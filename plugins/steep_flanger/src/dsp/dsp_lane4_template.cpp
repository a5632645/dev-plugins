#include "dsp_state.hpp"

#include <span>

#include <qwqdsp/convert.hpp>
#include <qwqdsp/filter/window_fir.hpp>
#include <qwqdsp/polymath.hpp>
#include <qwqdsp/window/kaiser.hpp>

namespace dsp {

static void UpdateFirCoeff(dsp::DspState& state) noexcept {
    auto& self = state.lane4;
    auto& param = state.param;

    size_t coeff_len = static_cast<size_t>(param.fir_coeff_len);
    state.coeff_len_ = coeff_len;

    const juce::SpinLock::ScopedLockType coeffs_lock(state.coeffs_lock_);

    if (param.fir_source == DspParam::kWindowSinc) {
        std::span<float> kernel{state.coeffs_.data(), coeff_len};
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
        const juce::SpinLock::ScopedLockType custom_coeffs_lock(param.custom_coeffs_lock_);
        std::copy_n(param.custom_coeffs_.begin(), coeff_len, state.coeffs_.begin());
    }

    size_t const coeff_len_div_4 = (coeff_len + 3) / 4;
    size_t const idxend = coeff_len_div_4 * 4;
    for (size_t i = coeff_len; i < idxend; ++i) {
        state.coeffs_[i] = 0;
    }

    std::span<float> kernel{state.coeffs_.data(), coeff_len};
    constexpr size_t num_bins = global::kFFTSize;
    float pad[global::kFFTSize]{};
    float pad_im[global::kFFTSize]{};
    std::array<float, num_bins> gains{};
    std::array<float, num_bins> fft_re{};
    std::array<float, num_bins> fft_im{};
    std::copy(kernel.begin(), kernel.end(), pad);

    state.complex_fft_.FFT(pad, pad_im, fft_re.data(), fft_im.data());
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
        state.complex_fft_.IFFT(log_gains, phases, pad, pad_im);
        pad[0] = 0;
        pad[num_bins / 2] = 0;
        for (size_t i = num_bins / 2 + 1; i < num_bins; ++i) {
            pad[i] = -pad[i];
        }

        std::fill_n(pad_im, num_bins, 0.0f);
        state.complex_fft_.FFT(pad, pad_im, log_gains, phases);
        for (size_t i = 0; i < num_bins; ++i) {
            fft_re[i] = gains[i] * std::cos(phases[i]);
            fft_im[i] = gains[i] * std::sin(phases[i]);
        }
        state.complex_fft_.IFFT(fft_re.data(), fft_im.data(), pad, pad_im);

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
        state.fir_gain_ = 1.0f;
    }
    else {
        state.fir_gain_ = 1.0f / std::sqrt(energy + 1e-10f);
    }

    state.have_new_coeff_ = true;
}

static void UpdateIirCoeff(dsp::DspState& state) noexcept {
    auto& param = state.param;
    auto& self = state.lane4;

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

    if (state.last_iir_highpass_ != param.fir_highpass) {
        state.last_iir_highpass_ = param.fir_highpass;
        for (size_t i = 0; i < global::kIirMaxNumFilters / 4; ++i) {
            self.iir_[i].Reset();
        }
    }

    double k = 1.0;
    std::complex<double> half_zpoles[global::kIirMaxNumFilters];
    {
        std::complex<double> half_spoles[global::kIirMaxNumFilters];
        if (!param.fir_highpass) {
            for (int i = 0; i < N; ++i) {
                double phi =
                    (2.0 * static_cast<double>(i + 1) - 1.0) * std::numbers::pi_v<double> / static_cast<double>(4 * N);
                half_spoles[i] = cutoff * std::complex{k_re * -std::sin(phi), k_im * std::cos(phi)};
                k *= std::norm(half_spoles[i]);
            }
        }
        else {
            for (int i = 0; i < N; ++i) {
                double phi =
                    (2.0 * static_cast<double>(i + 1) - 1.0) * std::numbers::pi_v<double> / static_cast<double>(4 * N);
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
        state.iir_fir_k_ = static_cast<float>(k * std::real(1.0 / down));
    }

    // 设定滤波器系数
    auto& filters = self.iir_;
    auto* residual_ptr = &residual[0];
    auto* pole_ptr = &half_zpoles[0];
    int full_4_num = N / 4;
    int residual_4_num = N % 4;

    for (int i = 0; i < full_4_num; ++i) {
        simd::Float128 r_re{
            static_cast<float>(2 * residual_ptr[0].real()),
            static_cast<float>(2 * residual_ptr[1].real()),
            static_cast<float>(2 * residual_ptr[2].real()),
            static_cast<float>(2 * residual_ptr[3].real()),
        };
        simd::Float128 r_im{
            static_cast<float>(2 * residual_ptr[0].imag()),
            static_cast<float>(2 * residual_ptr[1].imag()),
            static_cast<float>(2 * residual_ptr[2].imag()),
            static_cast<float>(2 * residual_ptr[3].imag()),
        };
        simd::Float128 p_re{
            static_cast<float>(std::real(pole_ptr[0])),
            static_cast<float>(std::real(pole_ptr[1])),
            static_cast<float>(std::real(pole_ptr[2])),
            static_cast<float>(std::real(pole_ptr[3])),
        };
        simd::Float128 p_im{
            static_cast<float>(std::imag(pole_ptr[0])),
            static_cast<float>(std::imag(pole_ptr[1])),
            static_cast<float>(std::imag(pole_ptr[2])),
            static_cast<float>(std::imag(pole_ptr[3])),
        };
        residual_ptr += 4;
        pole_ptr += 4;

        filters[i].Set(simd::SimdComplex<simd::Float128>{r_re, r_im}, simd::SimdComplex<simd::Float128>{p_re, p_im});
    }

    if (residual_4_num != 0) {
        simd::Float128 r_re{};
        simd::Float128 r_im{};
        simd::Float128 p_re{};
        simd::Float128 p_im{};
        for (int i = 0; i < residual_4_num; ++i) {
            r_re[i] = static_cast<float>(2 * residual_ptr[i].real());
            r_im[i] = static_cast<float>(2 * residual_ptr[i].imag());
            p_re[i] = static_cast<float>(std::real(pole_ptr[i]));
            p_im[i] = static_cast<float>(std::imag(pole_ptr[i]));
        }
        filters[full_4_num].Set(simd::SimdComplex<simd::Float128>{r_re, r_im},
                                simd::SimdComplex<simd::Float128>{p_re, p_im});
    }

    state.iir_fir_k_ = static_cast<float>(k);
}

static void ProcessFir(dsp::DspState& state, float* left, float* right, int num_samples) noexcept {
    auto& self = state.lane4;
    auto& param = state.param;

    int cando = num_samples;
    while (cando != 0) {
        int num_process = std::min<int>(512, cando);
        cando -= num_process;

        if (param.should_update_fir_) {
            param.should_update_fir_ = false;
            UpdateFirCoeff(state);
        }

        constexpr float kWarpFactor = -0.8f;
        float warp_drywet = param.drywet;
        warp_drywet = 2.0f * warp_drywet - 1.0f;
        warp_drywet = (warp_drywet - kWarpFactor) / (1.0f - warp_drywet * kWarpFactor);
        warp_drywet = 0.5f * warp_drywet + 0.5f;
        warp_drywet = std::clamp(warp_drywet, 0.0f, 1.0f);
        float feedback_mul = warp_drywet * param.feedback;
        float fir_gain_lerp = std::lerp(1.0f, state.fir_gain_, param.drywet);

        float const damp_pitch = param.damp_pitch;
        float const damp_freq = qwqdsp::convert::Pitch2Freq(damp_pitch);
        float const damp_w = qwqdsp::convert::Freq2W(damp_freq, state.fs_);
        state.damp_lowpass_coeff_ = state.damp_.ComputeCoeff(damp_w);

        state.barber_phase_smoother_.SetTarget(param.barber_phase);
        state.barber_oscillator_.SetFreq(param.barber_speed, state.fs_);

        // update delay times
        state.phase_ += param.lfo_freq / state.fs_ * static_cast<float>(num_process);
        float right_phase = state.phase_ + param.lfo_phase;
        {
            float t;
            state.phase_ = std::modf(state.phase_, &t);
            right_phase = std::modf(right_phase, &t);
        }
        float left_phase = state.phase_;

        simd::Float128 lfo_modu;
        lfo_modu[0] = qwqdsp::polymath::SinPi(left_phase * std::numbers::pi_v<float>);
        lfo_modu[1] = qwqdsp::polymath::SinPi(right_phase * std::numbers::pi_v<float>);

        float const delay_samples = param.delay_ms * state.fs_ / 1000.0f;
        float const depth_samples = param.depth_ms * state.fs_ / 1000.0f;
        auto target_delay_samples = delay_samples + lfo_modu * depth_samples;
        target_delay_samples = simd::Max(target_delay_samples, simd::Float128{0.0f, 0.0f, 0.0f, 0.0f});
        float const delay_time_smooth_factor =
            1.0f - std::exp(-1.0f / (state.fs_ / static_cast<float>(num_process) * global::kDelaySmoothMs / 1000.0f));
        state.last_exp_delay_samples_ +=
            delay_time_smooth_factor * (target_delay_samples - state.last_exp_delay_samples_);
        auto curr_num_notch = state.last_delay_samples_;
        auto delta_num_notch = (state.last_exp_delay_samples_ - curr_num_notch) / static_cast<float>(num_process);

        float curr_damp_coeff = state.last_damp_lowpass_coeff_;
        float delta_damp_coeff = (state.damp_lowpass_coeff_ - curr_damp_coeff) / (static_cast<float>(num_process));

        float const inv_samples = 1.0f / static_cast<float>(num_process);
        size_t const coeff_len_div_4 = (state.coeff_len_ + 3) / 4;
        alignas(16) std::array<simd::Float128, global::kSIMDMaxCoeffLen / 4> delta_coeffs;
        auto* coeffs_ptr = (simd::Float128*)(state.coeffs_.data());
        auto* last_coeffs_ptr = (simd::Float128*)(state.last_coeffs_.data());
        float const wet_mix = param.drywet;
        simd::Float128 dry_coeff{1.0f - wet_mix, 0.0f, 0.0f, 0.0f};
        for (size_t i = 0; i < coeff_len_div_4; ++i) {
            auto target_wet_coeff = coeffs_ptr[i] * wet_mix + dry_coeff;
            delta_coeffs[i] = (target_wet_coeff - last_coeffs_ptr[i]) * inv_samples;
            dry_coeff = simd::Float128{};
        }

        // fir polyphase filtering
        if (!param.barber_enable) {
            for (int j = 0; j < num_process; ++j) {
                curr_num_notch += delta_num_notch;
                curr_damp_coeff += delta_damp_coeff;

                for (size_t i = 0; i < coeff_len_div_4; ++i) {
                    last_coeffs_ptr[i] += delta_coeffs[i];
                }

                simd::Float128 left_sum{};
                float const left_num_notch = curr_num_notch[0];
                simd::Float128 current_delay;
                current_delay[0] = 0;
                current_delay[1] = left_num_notch;
                current_delay[2] = left_num_notch * 2;
                current_delay[3] = left_num_notch * 3;
                simd::Float128 delay_inc = simd::BroadcastF128(left_num_notch * 4);
                self.delay_left_.Push(*left + state.left_fb_ * feedback_mul);
                for (size_t i = 0; i < coeff_len_div_4; ++i) {
                    auto taps_out = self.delay_left_.GetAfterPush(current_delay);
                    current_delay += delay_inc;
                    left_sum += last_coeffs_ptr[i] * taps_out;
                }

                simd::Float128 right_sum{};
                float const right_num_notch = curr_num_notch[1];
                current_delay[0] = 0;
                current_delay[1] = right_num_notch;
                current_delay[2] = right_num_notch * 2;
                current_delay[3] = right_num_notch * 3;
                delay_inc = simd::BroadcastF128(right_num_notch * 4);
                self.delay_right_.Push(*right + state.right_fb_ * feedback_mul);
                for (size_t i = 0; i < coeff_len_div_4; ++i) {
                    auto taps_out = self.delay_right_.GetAfterPush(current_delay);
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
                damp_x = state.damp_.TickLowpass(damp_x, simd::BroadcastF128(curr_damp_coeff));
                auto dc_remove = state.dc_.TickHighpass(damp_x, simd::BroadcastF128(0.0005f));
                state.left_fb_ = qwqdsp::polymath::ArctanPade(dc_remove[0]);
                state.right_fb_ = qwqdsp::polymath::ArctanPade(dc_remove[1]);
            }
        }
        else {
            for (int j = 0; j < num_process; ++j) {
                curr_damp_coeff += delta_damp_coeff;
                curr_num_notch += delta_num_notch;

                for (size_t i = 0; i < coeff_len_div_4; ++i) {
                    last_coeffs_ptr[i] += delta_coeffs[i];
                }

                self.delay_left_.Push(*left + state.left_fb_ * feedback_mul);
                self.delay_right_.Push(*right + state.right_fb_ * feedback_mul);

                float const left_num_notch = curr_num_notch[0];
                float const right_num_notch = curr_num_notch[1];
                simd::Float128 left_current_delay;
                simd::Float128 right_current_delay;
                left_current_delay[0] = 0;
                left_current_delay[1] = left_num_notch;
                left_current_delay[2] = left_num_notch * 2;
                left_current_delay[3] = left_num_notch * 3;
                right_current_delay[0] = 0;
                right_current_delay[1] = right_num_notch;
                right_current_delay[2] = right_num_notch * 2;
                right_current_delay[3] = right_num_notch * 3;
                auto left_delay_inc = simd::BroadcastF128(left_num_notch * 4);
                auto right_delay_inc = simd::BroadcastF128(right_num_notch * 4);

                auto const addition_rotation =
                    std::polar(1.0f, state.barber_phase_smoother_.Tick() * std::numbers::pi_v<float> * 2);
                state.barber_oscillator_.Tick();
                auto const rotation_once = state.barber_oscillator_.GetCpx() * addition_rotation;
                auto const rotation_2 = rotation_once * rotation_once;
                auto const rotation_3 = rotation_once * rotation_2;
                auto const rotation_4 = rotation_2 * rotation_2;
                auto const right_channel_rotation = std::polar(1.0f, param.barber_stereo_phase);
                simd::Complex128 left_rotation_coeff;
                left_rotation_coeff.re[0] = 1;
                left_rotation_coeff.re[1] = rotation_once.real();
                left_rotation_coeff.re[2] = rotation_2.real();
                left_rotation_coeff.re[3] = rotation_3.real();
                left_rotation_coeff.im[0] = 0;
                left_rotation_coeff.im[1] = rotation_once.imag();
                left_rotation_coeff.im[2] = rotation_2.imag();
                left_rotation_coeff.im[3] = rotation_3.imag();
                simd::Complex128 right_rotation_coeff = left_rotation_coeff;
                right_rotation_coeff *= simd::Complex128{.re = simd::BroadcastF128(right_channel_rotation.real()),
                                                         .im = simd::BroadcastF128(right_channel_rotation.imag())};
                simd::Complex128 left_rotation_mul{.re = simd::BroadcastF128(rotation_4.real()),
                                                   .im = simd::BroadcastF128(rotation_4.imag())};
                simd::Complex128 right_rotation_mul = left_rotation_mul;

                simd::Float128 left_re_sum{};
                simd::Float128 left_im_sum{};
                simd::Float128 right_re_sum{};
                simd::Float128 right_im_sum{};
                for (size_t i = 0; i < coeff_len_div_4; ++i) {
                    auto left_taps_out = self.delay_left_.GetAfterPush(left_current_delay);
                    auto right_taps_out = self.delay_right_.GetAfterPush(right_current_delay);
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

                auto remove_positive_spectrum = self.hilbert_complex_.Tick(
                    simd::Float128{simd::ReduceAdd(left_re_sum), simd::ReduceAdd(left_im_sum),
                                   simd::ReduceAdd(right_re_sum), simd::ReduceAdd(right_im_sum)});
                // this will mirror the positive spectrum to negative domain, forming a real value signal
                auto damp_x =
                    simd::Shuffle<simd::Float128, 0, 2, 1, 3>(remove_positive_spectrum, remove_positive_spectrum);
                *left = damp_x[0] * fir_gain_lerp;
                *right = damp_x[1] * fir_gain_lerp;
                ++left;
                ++right;
                damp_x = state.damp_.TickLowpass(damp_x, simd::BroadcastF128(curr_damp_coeff));
                auto dc_remove = state.dc_.TickHighpass(damp_x, simd::BroadcastF128(0.0005f));
                state.left_fb_ = qwqdsp::polymath::ArctanPade(dc_remove[0]);
                state.right_fb_ = qwqdsp::polymath::ArctanPade(dc_remove[1]);
            }

            state.barber_osc_keep_amp_counter_ += num_process;
            [[unlikely]]
            if (state.barber_osc_keep_amp_counter_ > state.barber_osc_keep_amp_need_) {
                state.barber_osc_keep_amp_counter_ = 0;
                state.barber_oscillator_.KeepAmp();
            }
        }
        state.last_delay_samples_ = state.last_exp_delay_samples_;
        state.last_damp_lowpass_coeff_ = state.damp_lowpass_coeff_;
    }
}

static void ProcessIir(dsp::DspState& state, float* left, float* right, int num_samples) noexcept {
    auto& self = state.lane4;
    auto& param = state.param;

    if (param.should_update_iir_) {
        param.should_update_iir_ = false;
        UpdateIirCoeff(state);
    }

    // -------------------- 处理中 --------------------
    size_t cando = num_samples;
    while (cando != 0) {
        size_t num_process = std::min<size_t>(512, cando);
        cando -= num_process;

        float dry_mix = 1.0f - param.drywet;
        float wet_mix = param.drywet;

        float const damp_pitch = param.damp_pitch;
        float const damp_freq = qwqdsp::convert::Pitch2Freq(damp_pitch);
        float const damp_w = qwqdsp::convert::Freq2W(damp_freq, state.fs_);
        state.damp_lowpass_coeff_ = state.damp_.ComputeCoeff(damp_w);

        state.barber_phase_smoother_.SetTarget(param.barber_phase);
        state.barber_oscillator_.SetFreq(param.barber_speed, state.fs_);

        // update delay times
        state.phase_ += param.lfo_freq / state.fs_ * static_cast<float>(num_process);
        float right_phase = state.phase_ + param.lfo_phase;
        {
            float t;
            state.phase_ = std::modf(state.phase_, &t);
            right_phase = std::modf(right_phase, &t);
        }
        float left_phase = state.phase_;

        simd::Float128 lfo_modu;
        lfo_modu[0] = qwqdsp::polymath::SinPi(left_phase * std::numbers::pi_v<float>);
        lfo_modu[1] = qwqdsp::polymath::SinPi(right_phase * std::numbers::pi_v<float>);

        float const delay_samples = param.delay_ms * state.fs_ / 1000.0f;
        float const depth_samples = param.depth_ms * state.fs_ / 1000.0f;
        auto target_delay_samples = delay_samples + lfo_modu * depth_samples;
        target_delay_samples = simd::Max(target_delay_samples, simd::BroadcastF128(1.0f));
        float const delay_time_smooth_factor =
            1.0f - std::exp(-1.0f / (state.fs_ / static_cast<float>(num_process) * global::kDelaySmoothMs / 1000.0f));
        state.last_exp_delay_samples_ +=
            delay_time_smooth_factor * (target_delay_samples - state.last_exp_delay_samples_);
        auto curr_num_notch = state.last_delay_samples_;
        auto delta_num_notch = (state.last_exp_delay_samples_ - curr_num_notch) / static_cast<float>(num_process);

        float curr_damp_coeff = state.last_damp_lowpass_coeff_;
        float delta_damp_coeff = (state.damp_lowpass_coeff_ - curr_damp_coeff) / (static_cast<float>(num_process));

        float const inv_samples = 1.0f / static_cast<float>(num_process);
        size_t num_simd_filter = (param.iir_num_filters + 3) / 4;

        if (!param.barber_enable) {
            for (size_t j = 0; j < num_process; ++j) {
                curr_num_notch += delta_num_notch;
                curr_damp_coeff += delta_damp_coeff;

                float const left_in = *left;
                float const right_in = *right;
                float const left_num_notch = curr_num_notch[0];
                float const right_num_notch = curr_num_notch[1];
                auto& filters = self.iir_;

                float right_sum = left_in * state.iir_fir_k_;
                float left_sum = right_in * state.iir_fir_k_;
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
                auto& filters = self.iir_;

                std::complex<float> right_sum = 0;
                std::complex<float> left_sum = 0;

                auto const addition_rotation =
                    std::polar(1.0f, state.barber_phase_smoother_.Tick() * std::numbers::pi_v<float> * 2);
                state.barber_oscillator_.Tick();
                auto const rotation_once = state.barber_oscillator_.GetCpx() * addition_rotation;
                auto const right_channel_rotation = std::polar(1.0f, param.barber_stereo_phase);
                auto left_rotate = rotation_once;
                auto right_rotate = rotation_once * right_channel_rotation;

                for (size_t i = 0; i < num_simd_filter; ++i) {
                    auto [l, r] = filters[i].TickCpx(left_in, right_in, left_num_notch, right_num_notch, left_rotate,
                                                     right_rotate);
                    left_sum += l;
                    right_sum += r;
                }
                left_sum = left_sum * 0.5f + left_in * state.iir_fir_k_;
                right_sum = right_sum * 0.5f + right_in * state.iir_fir_k_;

                auto remove_positive_spectrum = self.hilbert_complex_.Tick(
                    simd::Float128{left_sum.real(), left_sum.imag(), right_sum.real(), right_sum.imag()});
                // this will mirror the positive spectrum to negative domain, forming a real value signal
                auto damp_x =
                    simd::Shuffle<simd::Float128, 0, 2, 1, 3>(remove_positive_spectrum, remove_positive_spectrum);

                *left = damp_x[0] * wet_mix + left_in * dry_mix;
                *right = damp_x[1] * wet_mix + right_in * dry_mix;
                ++left;
                ++right;
            }

            state.barber_osc_keep_amp_counter_ += num_process;
            [[unlikely]]
            if (state.barber_osc_keep_amp_counter_ > state.barber_osc_keep_amp_need_) {
                state.barber_osc_keep_amp_counter_ = 0;
                state.barber_oscillator_.KeepAmp();
            }
        }
        state.last_delay_samples_ = state.last_exp_delay_samples_;
        state.last_damp_lowpass_coeff_ = state.damp_lowpass_coeff_;
    }
}

// ----------------------------------------
// dsp processor
// ----------------------------------------

static void Init(dsp::DspState& state, float fs) noexcept {
    auto& self = state.lane4;

    for (size_t i = 0; i < global::kIirMaxNumFilters / 4; ++i) {
        self.iir_[i].Init(fs, DspParam::kInitMaxMs);
    }

    state.fs_ = fs;
    float const samples_need = fs * DspParam::kInitMaxMs / 1000.0f;
    self.delay_left_.Init(static_cast<size_t>(samples_need * global::kMaxCoeffLen));
    self.delay_right_.Init(static_cast<size_t>(samples_need * global::kMaxCoeffLen));
    state.barber_phase_smoother_.SetSmoothTime(20.0f, fs);
    state.damp_.Reset();
    state.barber_oscillator_.Reset();
    state.barber_osc_keep_amp_counter_ = 0;
    // VIC正交振荡器衰减非常慢，设定为5分钟保持一次
    state.barber_osc_keep_amp_need_ = static_cast<size_t>(fs * 60 * 5);

    state.complex_fft_.Init(global::kFFTSize);
}

static void Reset(dsp::DspState& state) noexcept {
    auto& self = state.lane4;

    self.delay_left_.Reset();
    self.delay_right_.Reset();
    state.left_fb_ = 0;
    state.right_fb_ = 0;
    state.damp_.Reset();
    self.hilbert_complex_.Reset();
    for (size_t i = 0; i < global::kIirMaxNumFilters / 4; ++i) {
        self.iir_[i].Reset();
    }

    state.last_damp_lowpass_coeff_ = state.damp_lowpass_coeff_;
    state.phase_ = 0;
    state.barber_phase_smoother_.Reset();
    state.barber_oscillator_.Reset();
}

static void Update(dsp::DspState& state, const dsp::DspParam& param) noexcept {}

static void Process(dsp::DspState& state, float* left, float* right, int num_samples) noexcept {
    auto& param = state.param;

    if (!param.iir_mode) {
        ProcessFir(state, left, right, num_samples);
    }
    else {
        ProcessIir(state, left, right, num_samples);
    }
}

// ----------------------------------------
// export
// ----------------------------------------

#ifndef DSP_EXPORT_NAME
#error "不应该编译这个文件,在其他cpp包含这个cpp并定义DSP_EXPORT_NAME=`dsp_dispatch.cpp里的变量`"
#endif

DspProcessor DSP_EXPORT_NAME{Init, Reset, Update, Process, DSP_INST_NAME};
} // namespace dsp
