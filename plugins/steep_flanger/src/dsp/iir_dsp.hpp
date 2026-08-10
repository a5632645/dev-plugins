#pragma once
#include <cmath>
#include <complex>
#include <numbers>

#include "dsp_shared.hpp"
#include "global.hpp"
#include "iirn_filter.hpp"
#include "pluginshared/dsp/stereo_iir_hilbert_cpx.hpp"
#include "pluginshared/simd/simd.hpp"
#include "qwqdsp/polymath.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"

namespace steep_flanger {

// ------------------------------------------------------------
// IirDsp
// IIR 相移滤波引擎：系数生成（切比雪夫/部分分式）、处理
// ------------------------------------------------------------
template <simd::Inst inst, class SimdT>
class IirDsp {
public:
    static constexpr size_t kIirNumContainers = global::kIirMaxNumFilters / simd::LaneSize<SimdT>;

    explicit IirDsp(DspShared<inst, SimdT>& shared) : shared_(shared) {}

    void Init(float fs) {
        for (size_t i = 0; i < kIirNumContainers; ++i) {
            iir_[i].Init(fs, global::kInitMaxMs);
        }
    }

    void Reset() {
        hilbert_complex_.Reset();
        for (size_t i = 0; i < kIirNumContainers; ++i) {
            iir_[i].Reset();
        }
    }

    void UpdateCoeff() noexcept {
        auto& param = shared_.param_;

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

    void Process(float* left, float* right, int num_samples) noexcept {
        auto& param = shared_.param_;

        // -------------------- 处理中 --------------------
        size_t cando = num_samples;
        while (cando != 0) {
            size_t num_process = std::min<size_t>(512, cando);
            cando -= num_process;

            float dry_mix = 1.0f - param.drywet;
            float wet_mix = param.drywet;

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
            target_delay_samples = simd::Max(target_delay_samples, simd::BroadcastF128(1.0f));
            float const delay_time_smooth_factor =
                1.0f - std::exp(-1.0f / (shared_.fs_ / static_cast<float>(num_process) * global::kDelaySmoothMs / 1000.0f));
            shared_.last_exp_delay_samples_ += delay_time_smooth_factor * (target_delay_samples - shared_.last_exp_delay_samples_);
            auto curr_num_notch = shared_.last_delay_samples_;
            auto delta_num_notch = (shared_.last_exp_delay_samples_ - curr_num_notch) / static_cast<float>(num_process);

            size_t num_simd_filter = (param.iir_num_filters + simd::LaneSize<SimdT> - 1) / simd::LaneSize<SimdT>;

            if (!param.barber_enable) {
                for (size_t j = 0; j < num_process; ++j) {
                    curr_num_notch += delta_num_notch;

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

                    float const left_in = *left;
                    float const right_in = *right;
                    float const left_num_notch = curr_num_notch[0];
                    float const right_num_notch = curr_num_notch[1];
                    auto& filters = iir_;

                    std::complex<float> right_sum = 0;
                    std::complex<float> left_sum = 0;

                    auto const addition_rotation =
                        std::polar(1.0f, shared_.barber_phase_smoother_.Tick() * std::numbers::pi_v<float> * 2);
                    shared_.barber_phase_ = std::fmod(shared_.barber_phase_ + barber_omega, std::numbers::pi_v<float> * 2);
                    auto const rotation_once = std::polar(1.0f, shared_.barber_phase_) * addition_rotation;
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
                    auto wet_x =
                        simd::Shuffle<simd::Float128, 0, 2, 1, 3>(remove_positive_spectrum, remove_positive_spectrum);

                    *left = wet_x[0] * wet_mix + left_in * dry_mix;
                    *right = wet_x[1] * wet_mix + right_in * dry_mix;
                    ++left;
                    ++right;
                }
            }
            shared_.last_delay_samples_ = shared_.last_exp_delay_samples_;
        }
    }
private:
    DspShared<inst, SimdT>& shared_;

    float iir_fir_k_{};
    bool last_iir_highpass_{false};
    steep_flanger::IirNFilter<SimdT> iir_[kIirNumContainers];

    pluginshared::dsp::StereoIIRHilbertCpx hilbert_complex_;
};

} // namespace steep_flanger

#pragma GCC diagnostic pop
