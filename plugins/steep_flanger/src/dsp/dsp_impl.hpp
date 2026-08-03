#pragma once
#include <algorithm>
#include <array>
#include <cassert>
#include <complex>
#include <numbers>
#include <span>

#include "com/iirn_filter.hpp"
#include "global.hpp"
#include "idsp.hpp"
#include "pluginshared/dsp/delay_line_1ch_4time.hpp"
#include "pluginshared/dsp/one_pole_tpt.hpp"
#include "pluginshared/dsp/stereo_iir_hilbert_cpx.hpp"
#include "pluginshared/simd/simd.hpp"
#include "qwqdsp/convert.hpp"
#include "qwqdsp/filter/window_fir.hpp"
#include "qwqdsp/misc/smoother.hpp"
#include "qwqdsp/oscillator/vic_sine_osc.hpp"
#include "qwqdsp/polymath.hpp"
#include "qwqdsp/spectral/complex_fft.hpp"
#include "qwqdsp/window/kaiser.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"

namespace steep_flanger {

template <simd::Inst inst, class SimdT>
class DspImpl : public Idsp {
public:
    static constexpr size_t kIirNumContainers = global::kIirMaxNumFilters / simd::LaneSize<SimdT>;

    ~DspImpl() override = default;

    void Init(float fs) override {
        for (size_t i = 0; i < kIirNumContainers; ++i) {
            iir_[i].Init(fs, DspParam::kInitMaxMs);
        }

        fs_ = fs;
        float const samples_need = fs * DspParam::kInitMaxMs / 1000.0f;
        delay_left_.Init(static_cast<size_t>(samples_need * global::kMaxCoeffLen));
        delay_right_.Init(static_cast<size_t>(samples_need * global::kMaxCoeffLen));
        barber_phase_smoother_.SetSmoothTime(20.0f, fs);
        damp_.Reset();
        barber_oscillator_.Reset();
        barber_osc_keep_amp_counter_ = 0;
        // VIC正交振荡器衰减非常慢，设定为5分钟保持一次
        barber_osc_keep_amp_need_ = static_cast<int>(fs * 60 * 5);

        complex_fft_.Init(global::kFFTSize);
    }

    void Reset() override {
        delay_left_.Reset();
        delay_right_.Reset();
        left_fb_ = 0;
        right_fb_ = 0;
        damp_.Reset();
        hilbert_complex_.Reset();
        for (size_t i = 0; i < kIirNumContainers; ++i) {
            iir_[i].Reset();
        }

        last_damp_lowpass_coeff_ = damp_lowpass_coeff_;
        phase_ = 0;
        barber_phase_smoother_.Reset();
        barber_oscillator_.Reset();
    }

    void Update(const DspParam& p, DspControl* control) override {
        param_ = p; // 复制一份参数（纯 POD）
        control_ = control;

        // 系数重建（由 Params::BeginListening 或 UI 设置的标志触发）
        if (control_->should_update_fir_.exchange(false)) {
            UpdateFirCoeff();
        }
        if (control_->should_update_iir_.exchange(false)) {
            UpdateIirCoeff();
        }
    }

    void Process(float* left, float* right, int num_samples) override {
        if (!param_.iir_mode) {
            ProcessFir(left, right, num_samples);
        }
        else {
            ProcessIir(left, right, num_samples);
        }
    }

    std::string_view InstName() override {
        return INST_NAME;
    }

    void GetCoeffs(float* out, int n) override {
        const juce::SpinLock::ScopedLockType lock(coeffs_lock_);
        std::copy_n(coeffs_.begin(), std::min<size_t>(static_cast<size_t>(n), coeffs_.size()), out);
    }

    bool ExchangeNewCoeff() override {
        return have_new_coeff_.exchange(false);
    }

    void SyncPhase(float phase) override {
        phase_ = phase;
    }

    void SyncBarberPhase(float phase) override {
        barber_oscillator_.Reset(phase);
    }
private:
    // ----------------------------------------
    // fir coefficient update (shared by all lanes)
    // ----------------------------------------
    void UpdateFirCoeff() noexcept {
        auto& param = param_;

        size_t coeff_len = static_cast<size_t>(param.fir_coeff_len);
        coeff_len_ = coeff_len;

        const juce::SpinLock::ScopedLockType coeffs_lock(coeffs_lock_);

        if (control_->fir_source == DspParam::kWindowSinc) {
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
            const juce::SpinLock::ScopedLockType custom_coeffs_lock(control_->custom_coeffs_lock_);
            std::copy_n(control_->custom_coeffs_.begin(), coeff_len, coeffs_.begin());
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

        have_new_coeff_ = true;
    }

    // ----------------------------------------
    // iir coefficient update
    // ----------------------------------------
    void UpdateIirCoeff() noexcept {
        auto& param = param_;

        int N = static_cast<int>(param.iir_num_filters);
        double eps = std::sqrt(std::pow(10.0, param.ripple / 10.0) - 1.0);
        double A = 1.0 / static_cast<double>(2 * N) * std::asinh(1.0 / eps);
        double k_re = std::sinh(A);
        double k_im = std::cosh(A);

        double cutoff = param.fir_cutoff;
        if (param.fir_highpass) {
            cutoff = std::numbers::pi_v<double> - cutoff;
        }
        cutoff = std::tan(cutoff / 2.0);

        if (last_iir_highpass_ != param.fir_highpass) {
            last_iir_highpass_ = param.fir_highpass;
            for (size_t i = 0; i < kIirNumContainers; ++i) {
                iir_[i].Reset();
            }
        }

        // s域
        double k = 1.0;
        std::complex<double> half_zpoles[global::kIirMaxNumFilters];
        {
            std::complex<double> half_spoles[global::kIirMaxNumFilters];
            if (!param.fir_highpass) {
                for (int i = 0; i < N; ++i) {
                    double phi = (2.0 * static_cast<double>(i + 1) - 1.0) * std::numbers::pi_v<double>
                               / static_cast<double>(4 * N);
                    half_spoles[i] = cutoff * std::complex{k_re * -std::sin(phi), k_im * std::cos(phi)};
                    k *= std::norm(half_spoles[i]);
                }
            }
            else {
                for (int i = 0; i < N; ++i) {
                    double phi = (2.0 * static_cast<double>(i + 1) - 1.0) * std::numbers::pi_v<double>
                               / static_cast<double>(4 * N);
                    auto pole = std::complex{k_re * -std::sin(phi), k_im * std::cos(phi)};
                    half_spoles[i] = cutoff / pole;
                }
            }

            float reduce_g = std::pow(10.0f, -param.ripple / 40.0f);
            k *= reduce_g;

            // 双线性变换
            for (int i = 0; i < N; ++i) {
                half_zpoles[i] = (1.0 + half_spoles[i]) / (1.0 - half_spoles[i]);
                k /= std::real((1.0 - half_spoles[i]) * (1.0 - std::conj(half_spoles[i])));
            }
        }

        // 部分分式分解
        std::complex<double> residual[global::kIirMaxNumFilters];
        double zero = -1.0;
        if (param.fir_highpass) {
            zero = 1.0;
        }
        for (int i = 0; i < N; ++i) {
            auto zpole = half_zpoles[i];

            std::complex<double> up = 1.0;
            for (int j = 0; j < N; ++j) {
                auto tmp_up = 1.0 - zero / zpole;
                up *= tmp_up;
                up *= tmp_up;
            }

            std::complex<double> down = 1.0;
            for (int j = 0; j < N; ++j) {
                if (i == j) {
                    down *= (1.0 - std::conj(half_zpoles[j]) / zpole);
                }
                else {
                    down *= (1.0 - half_zpoles[j] / zpole);
                    down *= (1.0 - std::conj(half_zpoles[j]) / zpole);
                }
            }

            residual[i] = k * up / down;
        }

        {
            std::complex<double> down = 1.0;
            for (int i = 0; i < N; ++i) {
                down *= (0.0 - half_zpoles[i]);
                down *= (0.0 - std::conj(half_zpoles[i]));
            }
            iir_fir_k_ = static_cast<float>(k * std::real(1.0 / down));
        }

        // 设定滤波器系数
        constexpr int kLane = simd::LaneSize<SimdT>;
        auto& filters = iir_;
        auto* residual_ptr = &residual[0];
        auto* pole_ptr = &half_zpoles[0];
        int full_num = N / kLane;
        int residual_num = N % kLane;

        for (int i = 0; i < full_num; ++i) {
            SimdT r_re{};
            SimdT r_im{};
            SimdT p_re{};
            SimdT p_im{};
            for (int lane = 0; lane < kLane; ++lane) {
                r_re[lane] = static_cast<float>(2 * residual_ptr[lane].real());
                r_im[lane] = static_cast<float>(2 * residual_ptr[lane].imag());
                p_re[lane] = static_cast<float>(std::real(pole_ptr[lane]));
                p_im[lane] = static_cast<float>(std::imag(pole_ptr[lane]));
            }
            residual_ptr += kLane;
            pole_ptr += kLane;

            filters[i].Set(simd::SimdComplex<SimdT>{r_re, r_im}, simd::SimdComplex<SimdT>{p_re, p_im});
        }

        if (residual_num != 0) {
            SimdT r_re{};
            SimdT r_im{};
            SimdT p_re{};
            SimdT p_im{};
            for (int lane = 0; lane < residual_num; ++lane) {
                r_re[lane] = static_cast<float>(2 * residual_ptr[lane].real());
                r_im[lane] = static_cast<float>(2 * residual_ptr[lane].imag());
                p_re[lane] = static_cast<float>(std::real(pole_ptr[lane]));
                p_im[lane] = static_cast<float>(std::imag(pole_ptr[lane]));
            }
            filters[full_num].Set(simd::SimdComplex<SimdT>{r_re, r_im}, simd::SimdComplex<SimdT>{p_re, p_im});
        }

        iir_fir_k_ = static_cast<float>(k);
    }

    // ----------------------------------------
    // fir processing
    // ----------------------------------------
    void ProcessFir(float* left, float* right, int num_samples) noexcept {
        auto& param = param_;

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
            float const damp_w = qwqdsp::convert::Freq2W(damp_freq, fs_);
            damp_lowpass_coeff_ = damp_.ComputeCoeff(damp_w);

            barber_phase_smoother_.SetTarget(param.barber_phase);
            barber_oscillator_.SetFreq(param.barber_speed, fs_);

            // update delay times
            phase_ += param.lfo_freq / fs_ * static_cast<float>(num_process);
            float right_phase = phase_ + param.lfo_phase;
            {
                float t;
                phase_ = std::modf(phase_, &t);
                right_phase = std::modf(right_phase, &t);
            }
            float left_phase = phase_;

            simd::Float128 lfo_modu;
            lfo_modu[0] = qwqdsp::polymath::SinPi(left_phase * std::numbers::pi_v<float>);
            lfo_modu[1] = qwqdsp::polymath::SinPi(right_phase * std::numbers::pi_v<float>);

            float const delay_samples = param.delay_ms * fs_ / 1000.0f;
            float const depth_samples = param.depth_ms * fs_ / 1000.0f;
            auto target_delay_samples = delay_samples + lfo_modu * depth_samples;
            target_delay_samples = simd::Max(target_delay_samples, simd::Float128{0.0f, 0.0f, 0.0f, 0.0f});
            float const delay_time_smooth_factor =
                1.0f - std::exp(-1.0f / (fs_ / static_cast<float>(num_process) * global::kDelaySmoothMs / 1000.0f));
            last_exp_delay_samples_ += delay_time_smooth_factor * (target_delay_samples - last_exp_delay_samples_);
            auto curr_num_notch = last_delay_samples_;
            auto delta_num_notch = (last_exp_delay_samples_ - curr_num_notch) / static_cast<float>(num_process);

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
                        std::polar(1.0f, barber_phase_smoother_.Tick() * std::numbers::pi_v<float> * 2);
                    barber_oscillator_.Tick();
                    auto const rotation_once = barber_oscillator_.GetCpx() * addition_rotation;
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

                barber_osc_keep_amp_counter_ += num_process;
                [[unlikely]]
                if (barber_osc_keep_amp_counter_ > barber_osc_keep_amp_need_) {
                    barber_osc_keep_amp_counter_ = 0;
                    barber_oscillator_.KeepAmp();
                }
            }
            last_delay_samples_ = last_exp_delay_samples_;
            last_damp_lowpass_coeff_ = damp_lowpass_coeff_;
        }
    }

    // ----------------------------------------
    // iir processing
    // ----------------------------------------
    void ProcessIir(float* left, float* right, int num_samples) noexcept {
        auto& param = param_;

        // -------------------- 处理中 --------------------
        size_t cando = num_samples;
        while (cando != 0) {
            size_t num_process = std::min<size_t>(512, cando);
            cando -= num_process;

            float dry_mix = 1.0f - param.drywet;
            float wet_mix = param.drywet;

            float const damp_pitch = param.damp_pitch;
            float const damp_freq = qwqdsp::convert::Pitch2Freq(damp_pitch);
            float const damp_w = qwqdsp::convert::Freq2W(damp_freq, fs_);
            damp_lowpass_coeff_ = damp_.ComputeCoeff(damp_w);

            barber_phase_smoother_.SetTarget(param.barber_phase);
            barber_oscillator_.SetFreq(param.barber_speed, fs_);

            // update delay times
            phase_ += param.lfo_freq / fs_ * static_cast<float>(num_process);
            float right_phase = phase_ + param.lfo_phase;
            {
                float t;
                phase_ = std::modf(phase_, &t);
                right_phase = std::modf(right_phase, &t);
            }
            float left_phase = phase_;

            simd::Float128 lfo_modu;
            lfo_modu[0] = qwqdsp::polymath::SinPi(left_phase * std::numbers::pi_v<float>);
            lfo_modu[1] = qwqdsp::polymath::SinPi(right_phase * std::numbers::pi_v<float>);

            float const delay_samples = param.delay_ms * fs_ / 1000.0f;
            float const depth_samples = param.depth_ms * fs_ / 1000.0f;
            auto target_delay_samples = delay_samples + lfo_modu * depth_samples;
            target_delay_samples = simd::Max(target_delay_samples, simd::BroadcastF128(1.0f));
            float const delay_time_smooth_factor =
                1.0f - std::exp(-1.0f / (fs_ / static_cast<float>(num_process) * global::kDelaySmoothMs / 1000.0f));
            last_exp_delay_samples_ += delay_time_smooth_factor * (target_delay_samples - last_exp_delay_samples_);
            auto curr_num_notch = last_delay_samples_;
            auto delta_num_notch = (last_exp_delay_samples_ - curr_num_notch) / static_cast<float>(num_process);

            float curr_damp_coeff = last_damp_lowpass_coeff_;
            float delta_damp_coeff = (damp_lowpass_coeff_ - curr_damp_coeff) / (static_cast<float>(num_process));

            size_t num_simd_filter = (param.iir_num_filters + simd::LaneSize<SimdT> - 1) / simd::LaneSize<SimdT>;

            if (!param.barber_enable) {
                for (size_t j = 0; j < num_process; ++j) {
                    curr_num_notch += delta_num_notch;
                    curr_damp_coeff += delta_damp_coeff;

                    float const left_in = *left;
                    float const right_in = *right;
                    float const left_num_notch = curr_num_notch[0];
                    float const right_num_notch = curr_num_notch[1];
                    auto& filters = iir_;

                    float right_sum = left_in * iir_fir_k_;
                    float left_sum = right_in * iir_fir_k_;
                    for (size_t i = 0; i < num_simd_filter; ++i) {
                        auto [l, r] = filters[i].Tick(left_in, right_in, left_num_notch, right_num_notch);
                        left_sum += l;
                        right_sum += r;
                    }

                    *left = left_sum * wet_mix + left_in * dry_mix;
                    *right = right_sum * wet_mix + right_in * dry_mix;
                    ++left;
                    ++right;
                }
            }
            else {
                for (size_t j = 0; j < num_process; ++j) {
                    curr_num_notch += delta_num_notch;
                    curr_damp_coeff += delta_damp_coeff;

                    float const left_in = *left;
                    float const right_in = *right;
                    float const left_num_notch = curr_num_notch[0];
                    float const right_num_notch = curr_num_notch[1];
                    auto& filters = iir_;

                    std::complex<float> right_sum = 0;
                    std::complex<float> left_sum = 0;

                    auto const addition_rotation =
                        std::polar(1.0f, barber_phase_smoother_.Tick() * std::numbers::pi_v<float> * 2);
                    barber_oscillator_.Tick();
                    auto const rotation_once = barber_oscillator_.GetCpx() * addition_rotation;
                    auto const right_channel_rotation = std::polar(1.0f, param.barber_stereo_phase);
                    auto left_rotate = rotation_once;
                    auto right_rotate = rotation_once * right_channel_rotation;

                    for (size_t i = 0; i < num_simd_filter; ++i) {
                        auto [l, r] = filters[i].TickCpx(left_in, right_in, left_num_notch, right_num_notch,
                                                         left_rotate, right_rotate);
                        left_sum += l;
                        right_sum += r;
                    }
                    left_sum = left_sum * 0.5f + left_in * iir_fir_k_;
                    right_sum = right_sum * 0.5f + right_in * iir_fir_k_;

                    auto remove_positive_spectrum = hilbert_complex_.Tick(
                        simd::Float128{left_sum.real(), left_sum.imag(), right_sum.real(), right_sum.imag()});
                    // this will mirror the positive spectrum to negative domain, forming a real value signal
                    auto damp_x =
                        simd::Shuffle<simd::Float128, 0, 2, 1, 3>(remove_positive_spectrum, remove_positive_spectrum);

                    *left = damp_x[0] * wet_mix + left_in * dry_mix;
                    *right = damp_x[1] * wet_mix + right_in * dry_mix;
                    ++left;
                    ++right;
                }

                barber_osc_keep_amp_counter_ += num_process;
                [[unlikely]]
                if (barber_osc_keep_amp_counter_ > barber_osc_keep_amp_need_) {
                    barber_osc_keep_amp_counter_ = 0;
                    barber_oscillator_.KeepAmp();
                }
            }
            last_delay_samples_ = last_exp_delay_samples_;
            last_damp_lowpass_coeff_ = damp_lowpass_coeff_;
        }
    }

    // ----------------------------------------
    // members
    // ----------------------------------------
    DspParam param_;        // 参数副本（纯 POD）
    DspControl* control_{}; // 共享控制（Params 持有）

    juce::SpinLock coeffs_lock_;
    simd::Array256<float, global::kSIMDMaxCoeffLen> coeffs_{};
    simd::Array256<float, global::kSIMDMaxCoeffLen> last_coeffs_{};
    std::atomic<bool> have_new_coeff_{}; // dsp just updated its fir coeff
    qwqdsp_spectral::ComplexFFT complex_fft_;

    // -------------------- shared --------------------
    float fs_{};

    // fir
    float fir_gain_{1.0f};
    size_t coeff_len_{};

    // feedback
    float left_fb_{};
    float right_fb_{};
    pluginshared::dsp::OnePoleTPT<inst, simd::Float128> damp_;
    pluginshared::dsp::OnePoleTPT<inst, simd::Float128> dc_;
    float damp_lowpass_coeff_{1.0f};
    float last_damp_lowpass_coeff_{1.0f};

    // iir
    float iir_fir_k_{};
    bool last_iir_highpass_{false};

    // delay time lfo
    float phase_{};
    simd::Float128 last_exp_delay_samples_{};
    simd::Float128 last_delay_samples_{};

    // barberpole
    qwqdsp_misc::ExpSmoother barber_phase_smoother_;
    qwqdsp_oscillator::VicSineOsc barber_oscillator_;
    int barber_osc_keep_amp_counter_{};
    int barber_osc_keep_amp_need_{};

    // -------------------- simd part --------------------
    pluginshared::dsp::DelayLineSingleChannelMultiTime<inst, SimdT> delay_left_;
    pluginshared::dsp::DelayLineSingleChannelMultiTime<inst, SimdT> delay_right_;
    dsp::com::IirNFilter<SimdT> iir_[kIirNumContainers];
    pluginshared::dsp::StereoIIRHilbertCpx hilbert_complex_;
};

} // namespace steep_flanger

#pragma GCC diagnostic pop
