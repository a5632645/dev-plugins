#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <complex>
#include <numbers>
#include <span>

#include "dsp_shared.hpp"
#include "global.hpp"
#include "params.hpp"
#include "pluginshared/dsp/delay_line_1ch_4time.hpp"
#include "pluginshared/dsp/one_pole_tpt.hpp"
#include "pluginshared/dsp/stereo_iir_hilbert_cpx.hpp"
#include "pluginshared/simd/simd.hpp"
#include "qwqdsp/convert.hpp"
#include "qwqdsp/filter/window_fir.hpp"
#include "qwqdsp/polymath.hpp"
#include "qwqdsp/spectral/complex_fft.hpp"
#include "qwqdsp/window/kaiser.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"

namespace steep_flanger {

// ------------------------------------------------------------
// FirDsp
// FIR 梳状滤波引擎：系数生成（窗函数/频谱整形/最小相位）、处理、系数显示
// ------------------------------------------------------------
template <simd::Inst inst, class SimdT>
class FirDsp {
public:
    explicit FirDsp(DspShared<inst, SimdT>& shared) : shared_(shared) {}

    void Init(float fs) {
        float const samples_need = fs * global::kInitMaxMs / 1000.0f;
        delay_left_.Init(static_cast<size_t>(samples_need * global::kMaxCoeffLen));
        delay_right_.Init(static_cast<size_t>(samples_need * global::kMaxCoeffLen));
        damp_.Reset();
        complex_fft_.Init(global::kFFTSize);
    }

    void Reset() {
        delay_left_.Reset();
        delay_right_.Reset();
        left_fb_ = 0;
        right_fb_ = 0;
        damp_.Reset();
        hilbert_complex_.Reset();
        last_damp_lowpass_coeff_ = damp_lowpass_coeff_;
    }

    void UpdateCoeff(const Params& p) noexcept {
        auto& param = shared_.param_;

        size_t coeff_len = static_cast<size_t>(param.fir_coeff_len);
        coeff_len_ = coeff_len;

        const juce::SpinLock::ScopedLockType coeffs_lock(coeffs_lock_);

        if (p.fir_source.load(std::memory_order_acquire) == Params::kWindowSinc) {
            std::span<float> kernel{coeffs_.data(), coeff_len};
            float const cutoff_w = param.fir_cutoff;
            if (param.fir_highpass) {
                qwqdsp_filter::WindowFIR::Highpass(kernel, std::numbers::pi_v<float> - cutoff_w);
            }
            else {
                qwqdsp_filter::WindowFIR::Lowpass(kernel, cutoff_w);
            }
            float const beta = qwqdsp_window::Kaiser::Beta(param.fir_side_lobe);
            qwqdsp_window::Kaiser::ApplyWindow(kernel, beta, false);
        }
        else {
            const juce::SpinLock::ScopedLockType custom_coeffs_lock(p.custom_coeffs_lock_);
            std::copy_n(p.custom_coeffs_.begin(), coeff_len, coeffs_.begin());
        }

        // zero pad to simd lane boundary
        size_t const coeff_len_div = (coeff_len + simd::LaneSize<SimdT> - 1) / simd::LaneSize<SimdT>;
        size_t const idxend = coeff_len_div * simd::LaneSize<SimdT>;
        for (size_t i = coeff_len; i < idxend; ++i) {
            coeffs_[i] = 0;
        }

        std::span<float> kernel{coeffs_.data(), coeff_len};
        constexpr size_t num_bins = global::kFFTSize;
        float pad[global::kFFTSize]{};
        float pad_im[global::kFFTSize]{};
        std::array<float, num_bins> gains{};
        std::array<float, num_bins> fft_re{};
        std::array<float, num_bins> fft_im{};
        std::copy(kernel.begin(), kernel.end(), pad);

        complex_fft_.FFT(pad, pad_im, fft_re.data(), fft_im.data());
        for (size_t i = 0; i < num_bins; ++i) {
            float g = std::sqrt(fft_re[i] * fft_re[i] + fft_im[i] * fft_im[i]);
            gains[i] = g;
        }

        if (param.fir_min_phase) {
            float log_gains[num_bins]{};
            for (size_t i = 0; i < num_bins; ++i) {
                float g = gains[i];
                log_gains[i] = std::log(g + 1e-18f);
            }

            float phases[num_bins]{};
            complex_fft_.IFFT(log_gains, phases, pad, pad_im);
            pad[0] = 0;
            pad[num_bins / 2] = 0;
            for (size_t i = num_bins / 2 + 1; i < num_bins; ++i) {
                pad[i] = -pad[i];
            }

            std::fill_n(pad_im, num_bins, 0.0f);
            complex_fft_.FFT(pad, pad_im, log_gains, phases);
            for (size_t i = 0; i < num_bins; ++i) {
                fft_re[i] = gains[i] * std::cos(phases[i]);
                fft_im[i] = gains[i] * std::sin(phases[i]);
            }
            complex_fft_.IFFT(fft_re.data(), fft_im.data(), pad, pad_im);

            for (size_t i = 0; i < kernel.size(); ++i) {
                kernel[i] = pad[i];
            }
        }

        float const max_spectral_gain = *std::max_element(gains.begin(), gains.end());
        float gain = 1.0f / (max_spectral_gain + 1e-10f);
        if (max_spectral_gain < 1e-10f) {
            gain = 1.0f;
        }
        for (auto& x : kernel) {
            x *= gain;
        }

        float energy = 0;
        for (auto x : kernel) {
            energy += x * x;
        }

        if (max_spectral_gain < 1e-10f) {
            fir_gain_ = 1.0f;
        }
        else {
            fir_gain_ = 1.0f / std::sqrt(energy + 1e-10f);
        }

        have_new_coeff_.store(true, std::memory_order_release);
    }

    void Process(float* left, float* right, int num_samples) noexcept {
        auto& param = shared_.param_;

        int cando = num_samples;
        while (cando != 0) {
            int num_process = std::min<int>(512, cando);
            cando -= num_process;

            constexpr float kWarpFactor = -0.8f;
            float warp_drywet = param.drywet;
            warp_drywet = 2.0f * warp_drywet - 1.0f;
            warp_drywet = (warp_drywet - kWarpFactor) / (1.0f - warp_drywet * kWarpFactor);
            warp_drywet = 0.5f * warp_drywet + 0.5f;
            warp_drywet = std::clamp(warp_drywet, 0.0f, 1.0f);
            float feedback_mul = warp_drywet * param.feedback;
            float fir_gain_lerp = std::lerp(1.0f, fir_gain_, param.drywet);

            float const damp_pitch = param.damp_pitch;
            float const damp_freq = qwqdsp::convert::Pitch2Freq(damp_pitch);
            float const damp_w = qwqdsp::convert::Freq2W(damp_freq, shared_.fs_);
            damp_lowpass_coeff_ = damp_.ComputeCoeff(damp_w);

            shared_.barber_phase_smoother_.SetTarget(param.barber_phase);
            float const barber_omega = param.barber_speed / shared_.fs_ * std::numbers::pi_v<float> * 2;

            // update delay times
            shared_.phase_ += param.lfo_freq / shared_.fs_ * static_cast<float>(num_process);
            float right_phase = shared_.phase_ + param.lfo_phase;
            {
                float t;
                shared_.phase_ = std::modf(shared_.phase_, &t);
                right_phase = std::modf(right_phase, &t);
            }
            float left_phase = shared_.phase_;

            simd::Float128 lfo_modu;
            lfo_modu[0] = qwqdsp::polymath::SinPi(left_phase * std::numbers::pi_v<float>);
            lfo_modu[1] = qwqdsp::polymath::SinPi(right_phase * std::numbers::pi_v<float>);

            float const delay_samples = param.delay_ms * shared_.fs_ / 1000.0f;
            float const depth_samples = param.depth_ms * shared_.fs_ / 1000.0f;
            auto target_delay_samples = delay_samples + lfo_modu * depth_samples;
            target_delay_samples = simd::Max(target_delay_samples, simd::Float128{0.0f, 0.0f, 0.0f, 0.0f});
            float const delay_time_smooth_factor =
                1.0f - std::exp(-1.0f / (shared_.fs_ / static_cast<float>(num_process) * global::kDelaySmoothMs / 1000.0f));
            shared_.last_exp_delay_samples_ += delay_time_smooth_factor * (target_delay_samples - shared_.last_exp_delay_samples_);
            auto curr_num_notch = shared_.last_delay_samples_;
            auto delta_num_notch = (shared_.last_exp_delay_samples_ - curr_num_notch) / static_cast<float>(num_process);

            float curr_damp_coeff = last_damp_lowpass_coeff_;
            float delta_damp_coeff = (damp_lowpass_coeff_ - curr_damp_coeff) / (static_cast<float>(num_process));

            float const inv_samples = 1.0f / static_cast<float>(num_process);
            size_t const coeff_len_div = (coeff_len_ + simd::LaneSize<SimdT> - 1) / simd::LaneSize<SimdT>;
            alignas(alignof(SimdT)) std::array<SimdT, global::kSIMDMaxCoeffLen / simd::LaneSize<SimdT>> delta_coeffs;
            auto* coeffs_ptr = reinterpret_cast<SimdT*>(coeffs_.data());
            auto* last_coeffs_ptr = reinterpret_cast<SimdT*>(last_coeffs_.data());
            float const wet_mix = param.drywet;
            SimdT dry_coeff{};
            dry_coeff[0] = 1.0f - wet_mix;
            for (size_t i = 0; i < coeff_len_div; ++i) {
                auto target_wet_coeff = coeffs_ptr[i] * wet_mix + dry_coeff;
                delta_coeffs[i] = (target_wet_coeff - last_coeffs_ptr[i]) * inv_samples;
                dry_coeff = SimdT{};
            }

            // fir polyphase filtering
            if (!param.barber_enable) {
                for (int j = 0; j < num_process; ++j) {
                    curr_num_notch += delta_num_notch;
                    curr_damp_coeff += delta_damp_coeff;

                    for (size_t i = 0; i < coeff_len_div; ++i) {
                        last_coeffs_ptr[i] += delta_coeffs[i];
                    }

                    SimdT left_sum{};
                    float const left_num_notch = curr_num_notch[0];
                    SimdT current_delay{};
                    for (int lane = 0; lane < simd::LaneSize<SimdT>; ++lane) {
                        current_delay[lane] = left_num_notch * static_cast<float>(lane);
                    }
                    auto delay_inc = simd::Broadcast<SimdT>(left_num_notch * simd::LaneSize<SimdT>);
                    delay_left_.Push(*left + left_fb_ * feedback_mul);
                    for (size_t i = 0; i < coeff_len_div; ++i) {
                        auto taps_out = delay_left_.GetAfterPush(current_delay);
                        current_delay += delay_inc;
                        left_sum += last_coeffs_ptr[i] * taps_out;
                    }

                    SimdT right_sum{};
                    float const right_num_notch = curr_num_notch[1];
                    for (int lane = 0; lane < simd::LaneSize<SimdT>; ++lane) {
                        current_delay[lane] = right_num_notch * static_cast<float>(lane);
                    }
                    delay_inc = simd::Broadcast<SimdT>(right_num_notch * simd::LaneSize<SimdT>);
                    delay_right_.Push(*right + right_fb_ * feedback_mul);
                    for (size_t i = 0; i < coeff_len_div; ++i) {
                        auto taps_out = delay_right_.GetAfterPush(current_delay);
                        current_delay += delay_inc;
                        right_sum += last_coeffs_ptr[i] * taps_out;
                    }

                    simd::Float128 damp_x;
                    damp_x[0] = simd::ReduceAdd(left_sum);
                    damp_x[1] = simd::ReduceAdd(right_sum);
                    *left = damp_x[0] * fir_gain_lerp;
                    *right = damp_x[1] * fir_gain_lerp;
                    ++left;
                    ++right;
                    damp_x = damp_.TickLowpass(damp_x, simd::BroadcastF128(curr_damp_coeff));
                    auto dc_remove = dc_.TickHighpass(damp_x, simd::BroadcastF128(0.0005f));
                    left_fb_ = qwqdsp::polymath::ArctanPade(dc_remove[0]);
                    right_fb_ = qwqdsp::polymath::ArctanPade(dc_remove[1]);
                }
            }
            else {
                for (int j = 0; j < num_process; ++j) {
                    curr_damp_coeff += delta_damp_coeff;
                    curr_num_notch += delta_num_notch;

                    for (size_t i = 0; i < coeff_len_div; ++i) {
                        last_coeffs_ptr[i] += delta_coeffs[i];
                    }

                    delay_left_.Push(*left + left_fb_ * feedback_mul);
                    delay_right_.Push(*right + right_fb_ * feedback_mul);

                    float const left_num_notch = curr_num_notch[0];
                    float const right_num_notch = curr_num_notch[1];
                    SimdT left_current_delay{};
                    SimdT right_current_delay{};
                    for (int lane = 0; lane < simd::LaneSize<SimdT>; ++lane) {
                        left_current_delay[lane] = left_num_notch * static_cast<float>(lane);
                        right_current_delay[lane] = right_num_notch * static_cast<float>(lane);
                    }
                    auto left_delay_inc = simd::Broadcast<SimdT>(left_num_notch * simd::LaneSize<SimdT>);
                    auto right_delay_inc = simd::Broadcast<SimdT>(right_num_notch * simd::LaneSize<SimdT>);

                    auto const addition_rotation =
                        std::polar(1.0f, shared_.barber_phase_smoother_.Tick() * std::numbers::pi_v<float> * 2);
                    shared_.barber_phase_ = std::fmod(shared_.barber_phase_ + barber_omega, std::numbers::pi_v<float> * 2);
                    auto const rotation_once = std::polar(1.0f, shared_.barber_phase_) * addition_rotation;
                    auto const right_channel_rotation = std::polar(1.0f, param.barber_stereo_phase);

                    // rotations[lane] = rotation_once^lane (right channel rotated by stereo phase)
                    std::array<std::complex<float>, simd::LaneSize<SimdT>> left_rotations{};
                    std::array<std::complex<float>, simd::LaneSize<SimdT>> right_rotations{};
                    left_rotations[0] = std::complex<float>{1.0f, 0.0f};
                    right_rotations[0] = std::complex<float>{1.0f, 0.0f};
                    for (int lane = 1; lane < simd::LaneSize<SimdT>; ++lane) {
                        left_rotations[lane] = left_rotations[lane - 1] * rotation_once;
                        right_rotations[lane] = right_rotations[lane - 1] * rotation_once;
                    }
                    for (auto& r : right_rotations) {
                        r *= right_channel_rotation;
                    }

                    SimdT left_rot_re{};
                    SimdT left_rot_im{};
                    SimdT right_rot_re{};
                    SimdT right_rot_im{};
                    for (int lane = 0; lane < simd::LaneSize<SimdT>; ++lane) {
                        left_rot_re[lane] = left_rotations[lane].real();
                        left_rot_im[lane] = left_rotations[lane].imag();
                        right_rot_re[lane] = right_rotations[lane].real();
                        right_rot_im[lane] = right_rotations[lane].imag();
                    }
                    simd::SimdComplex<SimdT> left_rotation_coeff{left_rot_re, left_rot_im};
                    simd::SimdComplex<SimdT> right_rotation_coeff{right_rot_re, right_rot_im};

                    // rotation^LaneSize multiplier for each tap block
                    auto const rotation_mul_c = left_rotations[simd::LaneSize<SimdT> - 1] * rotation_once;
                    simd::SimdComplex<SimdT> left_rotation_mul{simd::Broadcast<SimdT>(rotation_mul_c.real()),
                                                               simd::Broadcast<SimdT>(rotation_mul_c.imag())};
                    simd::SimdComplex<SimdT> right_rotation_mul = left_rotation_mul;

                    SimdT left_re_sum{};
                    SimdT left_im_sum{};
                    SimdT right_re_sum{};
                    SimdT right_im_sum{};
                    for (size_t i = 0; i < coeff_len_div; ++i) {
                        auto left_taps_out = delay_left_.GetAfterPush(left_current_delay);
                        auto right_taps_out = delay_right_.GetAfterPush(right_current_delay);
                        left_current_delay += left_delay_inc;
                        right_current_delay += right_delay_inc;

                        left_taps_out *= last_coeffs_ptr[i];
                        auto temp = left_taps_out * left_rotation_coeff.re;
                        left_re_sum += temp;
                        temp = left_taps_out * left_rotation_coeff.im;
                        left_im_sum += temp;

                        right_taps_out *= last_coeffs_ptr[i];
                        temp = right_taps_out * right_rotation_coeff.re;
                        right_re_sum += temp;
                        temp = right_taps_out * right_rotation_coeff.im;
                        right_im_sum += temp;

                        left_rotation_coeff *= left_rotation_mul;
                        right_rotation_coeff *= right_rotation_mul;
                    }

                    auto remove_positive_spectrum = hilbert_complex_.Tick(
                        simd::Float128{simd::ReduceAdd(left_re_sum), simd::ReduceAdd(left_im_sum),
                                       simd::ReduceAdd(right_re_sum), simd::ReduceAdd(right_im_sum)});
                    // this will mirror the positive spectrum to negative domain, forming a real value signal
                    auto damp_x =
                        simd::Shuffle<simd::Float128, 0, 2, 1, 3>(remove_positive_spectrum, remove_positive_spectrum);
                    *left = damp_x[0] * fir_gain_lerp;
                    *right = damp_x[1] * fir_gain_lerp;
                    ++left;
                    ++right;
                    damp_x = damp_.TickLowpass(damp_x, simd::BroadcastF128(curr_damp_coeff));
                    auto dc_remove = dc_.TickHighpass(damp_x, simd::BroadcastF128(0.0005f));
                    left_fb_ = qwqdsp::polymath::ArctanPade(dc_remove[0]);
                    right_fb_ = qwqdsp::polymath::ArctanPade(dc_remove[1]);
                }
            }
            shared_.last_delay_samples_ = shared_.last_exp_delay_samples_;
            last_damp_lowpass_coeff_ = damp_lowpass_coeff_;
        }
    }

    void GetCoeffs(float* out, int n) {
        const juce::SpinLock::ScopedLockType lock(coeffs_lock_);
        std::copy_n(coeffs_.begin(), std::min<size_t>(static_cast<size_t>(n), coeffs_.size()), out);
    }

    bool ExchangeNewCoeff() {
        return have_new_coeff_.exchange(false, std::memory_order_acq_rel);
    }
private:
    DspShared<inst, SimdT>& shared_;

    juce::SpinLock coeffs_lock_;
    simd::Array256<float, global::kSIMDMaxCoeffLen> coeffs_{};
    simd::Array256<float, global::kSIMDMaxCoeffLen> last_coeffs_{};
    std::atomic<bool> have_new_coeff_{}; // dsp just updated its fir coeff
    qwqdsp_spectral::ComplexFFT complex_fft_;
    float fir_gain_{1.0f};
    size_t coeff_len_{};
    pluginshared::dsp::OnePoleTPT<inst, simd::Float128> dc_;
    float left_fb_{};
    float right_fb_{};

    // 梳状延迟线（FIR 专用）
    pluginshared::dsp::DelayLineSingleChannelMultiTime<inst, SimdT> delay_left_;
    pluginshared::dsp::DelayLineSingleChannelMultiTime<inst, SimdT> delay_right_;
    pluginshared::dsp::StereoIIRHilbertCpx hilbert_complex_;

    // 反馈低通
    pluginshared::dsp::OnePoleTPT<inst, simd::Float128> damp_;
    float damp_lowpass_coeff_{1.0f};
    float last_damp_lowpass_coeff_{1.0f};
};

} // namespace steep_flanger

#pragma GCC diagnostic pop
