#pragma once
#include <algorithm>
#include <cassert>
#include <numbers>
#include "global.hpp"
#include "idsp.hpp"
#include "pluginshared/simd/simd.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"

namespace warpcore {

template <simd::Inst inst, class SimdT>
struct Tdf2LaneN {
    struct Coefficients {
        float b0;
        float a1;
        float a2;
    };
    std::array<Coefficients, global::kMaxPoles> coefficients{};

    // [pole pole], [pole, pole]
    // Each [] is a parallel simd::LaneSize<SimdT> filter.
    struct Tdf2State {
        SimdT z1_re_l;
        SimdT z1_re_r;
        SimdT z1_im_l;
        SimdT z1_im_r;
        SimdT z2_re_l;
        SimdT z2_re_r;
        SimdT z2_im_l;
        SimdT z2_im_r;
    };
    simd::Array256<Tdf2State, global::kMaxBands * global::kMaxPoles / simd::LaneSize<SimdT>> state{};

    void Reset() noexcept {
        state.fill(Tdf2State{});
    }

    void SetPole(int pole_idx, float w, float Q, float fmul) noexcept {
        const float g = std::tan(w / 2.0f) * fmul;
        const float inv_q = 1.0f / Q;
        const float norm = 1.0f / (1.0f + inv_q * g + g * g);
        const float b0 = g * g * norm;

        coefficients[pole_idx] = {
            .b0 = b0,
            .a1 = 2.0f * (g * g - 1.0f) * norm,
            .a2 = (1.0f - inv_q * g + g * g) * norm,
        };
    }
};

template <simd::Inst inst, class SimdT>
class DspImpl : public Idsp {
public:
    ~DspImpl() override = default;

    void Init(float fs) override {
        fs_ = fs;
        total_smooth_samples_ = static_cast<int>(fs * 10.0f / 1000.0f);
    }

    void Reset() override {
        tdf2_.Reset();
        pre_osc_phase_ = 0.0f;
        post_osc_phase_ = 0.0f;
        StopSmooth();
    }

    void Update(const warpcore::Param& p) override {
        num_warps_ = p.bands;
        drywet_ = p.drywet;
        pitch_affect_ = p.pitch_affect;
        freq_distribution_ = p.freq_distribution;

        float fhigh = p.f_high;
        float fshit = p.pitch_affect ? -p.pitch_shift : p.pitch_shift;
        fshit = std::exp2(fshit / 12.0f);

        if (fhigh > 20000.0f) {
            fhigh = fs_ / 2;
        }
        fhigh = std::min(fhigh, fs_ / 2);

        float f_first_band_stop = fhigh / static_cast<float>(p.bands);
        float f_first_band_center = f_first_band_stop / 2;

        bool pitch_alas = !pitch_affect_ && p.pitch_shift > 0.0f;
        bool formant_alas = pitch_affect_ && p.pitch_shift < 0.0f;

        if (pitch_alas || formant_alas) {
            f_first_band_stop *= fshit;
            int max_bands = static_cast<int>(fs_ / 2.0f / f_first_band_stop);
            max_bands = std::max(max_bands, 1);
            num_warps_ = std::clamp(num_warps_, 1, max_bands);
        }

        if (freq_distribution_ == FreqDistrbution::k0_n || freq_distribution_ == FreqDistrbution::k1_n) {
            f_first_band_center *= 2;
        }

        pre_osc_phase_inc_ = f_first_band_center / fs_;
        post_osc_phase_inc_ = pre_osc_phase_inc_ * fshit;

        if (p.pitch_affect) {
            std::swap(pre_osc_phase_inc_, post_osc_phase_inc_);
        }

        // butterworth lowpass
        float wbase = f_first_band_center * 2 * std::numbers::pi_v<float> / fs_;
        if (!p.fill_gap) {
            bool mul1 = p.pitch_affect && p.pitch_shift > 0.0f;
            bool mul2 = !p.pitch_affect && p.pitch_shift < 0.0f;
            if (mul1 || mul2) {
                wbase *= fshit;
            }
        }
        else {
            bool mul1 = p.pitch_affect && p.pitch_shift > 0.0f;
            bool mul2 = !p.pitch_affect && p.pitch_shift < 0.0f;
            if (!(mul1 || mul2)) {
                wbase *= fshit;
            }
        }

        float filter_w = wbase * p.filter_scale;
        filter_w = std::min(filter_w, std::numbers::pi_v<float> - 0.1f);

        bool stop_smooth = poles_ != p.filter_order;
        poles_ = p.filter_order;
        SetFreq(filter_w, p.filter_order);
        if (stop_smooth) {
            tdf2_.Reset();

            // 调整极点数量立刻赋值给滤波器，跳过所有平滑过程
            StopSmooth();
            for (int i = 0; i < poles_; ++i) {
                tdf2_.SetPole(i, w_[i], q_[i], analog_fmul_);
            }
        }
        else {
            smooth_samples_ = total_smooth_samples_;
            BeginSmooth();
        }
    }

    void Process(float* left, float* right, int num_samples) override {
        while (num_samples != 0) {
            int blocK_size = num_samples;
            if (smooth_samples_ != 0) {
                blocK_size = std::min(blocK_size, smooth_samples_);
                smooth_samples_ -= blocK_size;
                Process2<true>(left, right, blocK_size);

                if (smooth_samples_ == 0) {
                    StopSmooth();
                    for (int i = 0; i < poles_; ++i) {
                        tdf2_.SetPole(i, w_[i], q_[i], analog_fmul_);
                    }
                }
            }
            else {
                Process2<false>(left, right, blocK_size);
            }
            num_samples -= blocK_size;
            left += blocK_size;
            if (right != nullptr) {
                right += blocK_size;
            }
        }
    }

    std::string_view InstName() override {
        return INST_NAME;
    }
private:
    void SetFreq(float w_lowpass, int num_filters) noexcept {
        constexpr float atten = 0.5f;
        constexpr float square_epsi = (1.0f - atten * atten) / atten;
        analog_fmul_ = 1.0f / std::pow(square_epsi, 0.25f / static_cast<float>(num_filters));

        int n = 2 * num_filters;
        for (int k = 1; k <= num_filters; ++k) {
            float phi =
                (2.0f * static_cast<float>(k) - 1.0f) * std::numbers::pi_v<float> / (2.0f * static_cast<float>(n));
            float Q = 1.0f / (2.0f * std::sin(phi));
            w_[k - 1] = w_lowpass;
            q_[k - 1] = Q;
        }
    }

    void StopSmooth() noexcept {
        smooth_samples_ = 0;
        last_q_ = q_;
        last_w_ = w_;
        q_inc_.fill(0.0f);
        w_inc_.fill(0.0f);

        pre_osc_phase_inc_inc_ = 0.0f;
        post_osc_phase_inc_inc_ = 0.0f;
        last_pre_osc_phase_inc_ = pre_osc_phase_inc_;
        last_post_osc_phase_inc_ = post_osc_phase_inc_;
    }

    void BeginSmooth() noexcept {
        smooth_samples_ = total_smooth_samples_;
        float inv_samples = 1.0f / static_cast<float>(smooth_samples_);
        pre_osc_phase_inc_inc_ = (pre_osc_phase_inc_ - last_pre_osc_phase_inc_) * inv_samples;
        post_osc_phase_inc_inc_ = (post_osc_phase_inc_ - last_post_osc_phase_inc_) * inv_samples;
        for (int i = 0; i < poles_; ++i) {
            q_inc_[i] = (q_[i] - last_q_[i]) * inv_samples;
            w_inc_[i] = (w_[i] - last_w_[i]) * inv_samples;
        }
    }

    template <bool kSmooth>
    void Process2(float* left, float* right, int num_samples) noexcept {
        switch (freq_distribution_) {
            case FreqDistrbution::k0_n:
                ProcessPoles<FreqDistrbution::k0_n, kSmooth>(left, right, num_samples);
                break;
            case FreqDistrbution::k1_n:
                ProcessPoles<FreqDistrbution::k1_n, kSmooth>(left, right, num_samples);
                break;
            case FreqDistrbution::k0_2n:
                ProcessPoles<FreqDistrbution::k0_2n, kSmooth>(left, right, num_samples);
                break;
            case FreqDistrbution::k1_2n:
                ProcessPoles<FreqDistrbution::k1_2n, kSmooth>(left, right, num_samples);
                break;
        }
    }

    template <FreqDistrbution kFreqMode, bool kSmooth>
    void ProcessPoles(float* left, float* right, int num_samples) noexcept {
        switch (poles_) {
            case 1:
                ProcessInternal<kFreqMode, 1, kSmooth>(left, right, num_samples);
                break;
            case 2:
                ProcessInternal<kFreqMode, 2, kSmooth>(left, right, num_samples);
                break;
            case 3:
                ProcessInternal<kFreqMode, 3, kSmooth>(left, right, num_samples);
                break;
            case 4:
                ProcessInternal<kFreqMode, 4, kSmooth>(left, right, num_samples);
                break;
            case 5:
                ProcessInternal<kFreqMode, 5, kSmooth>(left, right, num_samples);
                break;
            case 6:
                ProcessInternal<kFreqMode, 6, kSmooth>(left, right, num_samples);
                break;
            case 7:
                ProcessInternal<kFreqMode, 7, kSmooth>(left, right, num_samples);
                break;
            case 8:
                ProcessInternal<kFreqMode, 8, kSmooth>(left, right, num_samples);
                break;
            default:
                assert(false);
                break;
        }
    }

    template <FreqDistrbution kFreqMode, int kPoles, bool kSmooth>
    void ProcessInternal(float* left, float* right, int num_samples) noexcept {
        if (right == nullptr) {
            ProcessInternal_Mono<kFreqMode, kPoles, kSmooth>(left, num_samples);
        }
        else {
            ProcessInternal_Stereo<kFreqMode, kPoles, kSmooth>(left, right, num_samples);
        }
    }

    static void ProcessTdf2(SimdT& re, SimdT& im, SimdT& z1_re, SimdT& z1_im, SimdT& z2_re, SimdT& z2_im,
                            const typename Tdf2LaneN<inst, SimdT>::Coefficients& c) noexcept {
        const SimdT b0_re = c.b0 * re;
        const SimdT b0_im = c.b0 * im;
        const SimdT y_re = b0_re + z1_re;
        const SimdT y_im = b0_im + z1_im;

        z1_re = b0_re + b0_re - c.a1 * y_re + z2_re;
        z1_im = b0_im + b0_im - c.a1 * y_im + z2_im;
        z2_re = b0_re - c.a2 * y_re;
        z2_im = b0_im - c.a2 * y_im;

        re = y_re;
        im = y_im;
    }

    template <class SimdT2>
    struct ComplexPhase {
        simd::SimdComplex<SimdT2> pre_osc;
        simd::SimdComplex<SimdT2> post_osc;
        simd::SimdComplex<SimdT2> pre_osc_n_val;
        simd::SimdComplex<SimdT2> post_osc_n_val;
        SimdT2 band_gain;
    };

    template <FreqDistrbution kFreqMode, class T> requires (simd::LaneSize<T> == 4)
    static ComplexPhase<simd::Float128> GetComplexPhase(std::complex<float> pre_osc_f32,
                                                        std::complex<float> post_osc_f32) {
        ComplexPhase<simd::Float128> r;

        const auto pre_osc_f32_0 = std::complex{1.0f, 0.0f};
        const auto pre_osc_f32_1 = pre_osc_f32;
        const auto pre_osc_f32_2 = pre_osc_f32 * pre_osc_f32;
        const auto pre_osc_f32_3 = pre_osc_f32 * pre_osc_f32 * pre_osc_f32;
        const auto pre_osc_f32_4 = pre_osc_f32 * pre_osc_f32 * pre_osc_f32 * pre_osc_f32;
        const auto pre_osc_f32_5 = pre_osc_f32_1 * pre_osc_f32_4;
        const auto pre_osc_f32_6 = pre_osc_f32_2 * pre_osc_f32_4;
        const auto pre_osc_f32_7 = pre_osc_f32_3 * pre_osc_f32_4;
        const auto pre_osc_f32_8 = pre_osc_f32_4 * pre_osc_f32_4;

        const auto post_osc_f32_0 = std::complex{1.0f, 0.0f};
        const auto post_osc_f32_1 = post_osc_f32;
        const auto post_osc_f32_2 = post_osc_f32 * post_osc_f32;
        const auto post_osc_f32_3 = post_osc_f32 * post_osc_f32 * post_osc_f32;
        const auto post_osc_f32_4 = post_osc_f32 * post_osc_f32 * post_osc_f32 * post_osc_f32;
        const auto post_osc_f32_5 = post_osc_f32_1 * post_osc_f32_4;
        const auto post_osc_f32_6 = post_osc_f32_2 * post_osc_f32_4;
        const auto post_osc_f32_7 = post_osc_f32_3 * post_osc_f32_4;
        const auto post_osc_f32_8 = post_osc_f32_4 * post_osc_f32_4;

        if constexpr (kFreqMode == FreqDistrbution::k0_n) {
            r.pre_osc = simd::Complex128{
                .re = simd::BroadcastF128(pre_osc_f32_4.real()),
                .im = simd::BroadcastF128(pre_osc_f32_4.imag()),
            };
            r.post_osc = simd::Complex128{
                .re = simd::BroadcastF128(post_osc_f32_4.real()),
                .im = simd::BroadcastF128(post_osc_f32_4.imag()),
            };

            r.pre_osc_n_val = simd::Complex128{
                .re = {pre_osc_f32_0.real(), pre_osc_f32_1.real(), pre_osc_f32_2.real(), pre_osc_f32_3.real()},
                .im = {pre_osc_f32_0.imag(), pre_osc_f32_1.imag(), pre_osc_f32_2.imag(), pre_osc_f32_3.imag()},
            };
            r.post_osc_n_val = simd::Complex128{
                .re = {post_osc_f32_0.real(), post_osc_f32_1.real(), post_osc_f32_2.real(), post_osc_f32_3.real()},
                .im = {post_osc_f32_0.imag(), post_osc_f32_1.imag(), post_osc_f32_2.imag(), post_osc_f32_3.imag()},
            };

            r.band_gain = simd::Float128{1.0f, 2.0f, 2.0f, 2.0f};
        }
        else if constexpr (kFreqMode == FreqDistrbution::k1_n) {
            r.pre_osc = simd::Complex128{
                .re = simd::BroadcastF128(pre_osc_f32_8.real()),
                .im = simd::BroadcastF128(pre_osc_f32_8.imag()),
            };
            r.post_osc = simd::Complex128{
                .re = simd::BroadcastF128(post_osc_f32_8.real()),
                .im = simd::BroadcastF128(post_osc_f32_8.imag()),
            };

            r.pre_osc_n_val = simd::Complex128{
                .re = {pre_osc_f32_1.real(), pre_osc_f32_2.real(), pre_osc_f32_3.real(), pre_osc_f32_4.real()},
                .im = {pre_osc_f32_1.imag(), pre_osc_f32_2.imag(), pre_osc_f32_3.imag(), pre_osc_f32_4.imag()},
            };
            r.post_osc_n_val = simd::Complex128{
                .re = {post_osc_f32_1.real(), post_osc_f32_2.real(), post_osc_f32_3.real(), post_osc_f32_4.real()},
                .im = {post_osc_f32_1.imag(), post_osc_f32_2.imag(), post_osc_f32_3.imag(), post_osc_f32_4.imag()},
            };

            r.band_gain = simd::Float128{1.0f, 1.0f, 1.0f, 1.0f} * 2.0f;
        }
        else if constexpr (kFreqMode == FreqDistrbution::k0_2n) {
            r.pre_osc = simd::Complex128{
                .re = simd::BroadcastF128(pre_osc_f32_8.real()),
                .im = simd::BroadcastF128(pre_osc_f32_8.imag()),
            };
            r.post_osc = simd::Complex128{
                .re = simd::BroadcastF128(post_osc_f32_8.real()),
                .im = simd::BroadcastF128(post_osc_f32_8.imag()),
            };

            r.pre_osc_n_val = simd::Complex128{
                .re = {pre_osc_f32_0.real(), pre_osc_f32_2.real(), pre_osc_f32_4.real(), pre_osc_f32_6.real()},
                .im = {pre_osc_f32_0.imag(), pre_osc_f32_2.imag(), pre_osc_f32_4.imag(), pre_osc_f32_6.imag()},
            };
            r.post_osc_n_val = simd::Complex128{
                .re = {post_osc_f32_0.real(), post_osc_f32_2.real(), post_osc_f32_4.real(), post_osc_f32_6.real()},
                .im = {post_osc_f32_0.imag(), post_osc_f32_2.imag(), post_osc_f32_4.imag(), post_osc_f32_6.imag()},
            };

            r.band_gain = simd::Float128{1.0f, 2.0f, 2.0f, 2.0f};
        }
        else if constexpr (kFreqMode == FreqDistrbution::k1_2n) {
            r.pre_osc = simd::Complex128{
                .re = simd::BroadcastF128(pre_osc_f32_8.real()),
                .im = simd::BroadcastF128(pre_osc_f32_8.imag()),
            };
            r.post_osc = simd::Complex128{
                .re = simd::BroadcastF128(post_osc_f32_8.real()),
                .im = simd::BroadcastF128(post_osc_f32_8.imag()),
            };

            r.pre_osc_n_val = simd::Complex128{
                .re = {pre_osc_f32_1.real(), pre_osc_f32_3.real(), pre_osc_f32_5.real(), pre_osc_f32_7.real()},
                .im = {pre_osc_f32_1.imag(), pre_osc_f32_3.imag(), pre_osc_f32_5.imag(), pre_osc_f32_7.imag()},
            };
            r.post_osc_n_val = simd::Complex128{
                .re = {post_osc_f32_1.real(), post_osc_f32_3.real(), post_osc_f32_5.real(), post_osc_f32_7.real()},
                .im = {post_osc_f32_1.imag(), post_osc_f32_3.imag(), post_osc_f32_5.imag(), post_osc_f32_7.imag()},
            };

            r.band_gain = simd::Float128{1.0f, 1.0f, 1.0f, 1.0f} * 2.0f;
        }

        return r;
    }

    template <FreqDistrbution kFreqMode, class T> requires (simd::LaneSize<T> == 8)
    static ComplexPhase<simd::Float256> GetComplexPhase(std::complex<float> pre_osc_f32,
                                                        std::complex<float> post_osc_f32) {
        ComplexPhase<simd::Float256> r;

        const auto pre_osc_f32_0 = std::complex{1.0f, 0.0f};
        const auto pre_osc_f32_1 = pre_osc_f32;
        const auto pre_osc_f32_2 = pre_osc_f32 * pre_osc_f32;
        const auto pre_osc_f32_3 = pre_osc_f32 * pre_osc_f32 * pre_osc_f32;
        const auto pre_osc_f32_4 = pre_osc_f32 * pre_osc_f32 * pre_osc_f32 * pre_osc_f32;
        const auto pre_osc_f32_5 = pre_osc_f32_1 * pre_osc_f32_4;
        const auto pre_osc_f32_6 = pre_osc_f32_2 * pre_osc_f32_4;
        const auto pre_osc_f32_7 = pre_osc_f32_3 * pre_osc_f32_4;
        const auto pre_osc_f32_8 = pre_osc_f32_4 * pre_osc_f32_4;
        const auto pre_osc_f32_9 = pre_osc_f32_5 * pre_osc_f32_4;
        const auto pre_osc_f32_10 = pre_osc_f32_6 * pre_osc_f32_4;
        const auto pre_osc_f32_11 = pre_osc_f32_7 * pre_osc_f32_4;
        const auto pre_osc_f32_12 = pre_osc_f32_8 * pre_osc_f32_4;
        const auto pre_osc_f32_13 = pre_osc_f32_9 * pre_osc_f32_4;
        const auto pre_osc_f32_14 = pre_osc_f32_10 * pre_osc_f32_4;
        const auto pre_osc_f32_15 = pre_osc_f32_11 * pre_osc_f32_4;
        const auto pre_osc_f32_16 = pre_osc_f32_12 * pre_osc_f32_4;

        const auto post_osc_f32_0 = std::complex{1.0f, 0.0f};
        const auto post_osc_f32_1 = post_osc_f32;
        const auto post_osc_f32_2 = post_osc_f32 * post_osc_f32;
        const auto post_osc_f32_3 = post_osc_f32 * post_osc_f32 * post_osc_f32;
        const auto post_osc_f32_4 = post_osc_f32 * post_osc_f32 * post_osc_f32 * post_osc_f32;
        const auto post_osc_f32_5 = post_osc_f32_1 * post_osc_f32_4;
        const auto post_osc_f32_6 = post_osc_f32_2 * post_osc_f32_4;
        const auto post_osc_f32_7 = post_osc_f32_3 * post_osc_f32_4;
        const auto post_osc_f32_8 = post_osc_f32_4 * post_osc_f32_4;
        const auto post_osc_f32_9 = post_osc_f32_5 * post_osc_f32_4;
        const auto post_osc_f32_10 = post_osc_f32_6 * post_osc_f32_4;
        const auto post_osc_f32_11 = post_osc_f32_7 * post_osc_f32_4;
        const auto post_osc_f32_12 = post_osc_f32_8 * post_osc_f32_4;
        const auto post_osc_f32_13 = post_osc_f32_9 * post_osc_f32_4;
        const auto post_osc_f32_14 = post_osc_f32_10 * post_osc_f32_4;
        const auto post_osc_f32_15 = post_osc_f32_11 * post_osc_f32_4;
        const auto post_osc_f32_16 = post_osc_f32_12 * post_osc_f32_4;

        if constexpr (kFreqMode == FreqDistrbution::k0_n) {
            r.pre_osc = simd::Complex256{
                .re = simd::BroadcastF256(pre_osc_f32_8.real()),
                .im = simd::BroadcastF256(pre_osc_f32_8.imag()),
            };
            r.post_osc = simd::Complex256{
                .re = simd::BroadcastF256(post_osc_f32_8.real()),
                .im = simd::BroadcastF256(post_osc_f32_8.imag()),
            };

            r.pre_osc_n_val = simd::Complex256{
                .re = {pre_osc_f32_0.real(), pre_osc_f32_1.real(), pre_osc_f32_2.real(), pre_osc_f32_3.real(),
                       pre_osc_f32_4.real(), pre_osc_f32_5.real(), pre_osc_f32_6.real(), pre_osc_f32_7.real()},
                .im = {pre_osc_f32_0.imag(), pre_osc_f32_1.imag(), pre_osc_f32_2.imag(), pre_osc_f32_3.imag(),
                       pre_osc_f32_4.imag(), pre_osc_f32_5.imag(), pre_osc_f32_6.imag(), pre_osc_f32_7.imag()},
            };
            r.post_osc_n_val = simd::Complex256{
                .re = {post_osc_f32_0.real(), post_osc_f32_1.real(), post_osc_f32_2.real(), post_osc_f32_3.real(),
                       post_osc_f32_4.real(), post_osc_f32_5.real(), post_osc_f32_6.real(), post_osc_f32_7.real()},
                .im = {post_osc_f32_0.imag(), post_osc_f32_1.imag(), post_osc_f32_2.imag(), post_osc_f32_3.imag(),
                       post_osc_f32_4.imag(), post_osc_f32_5.imag(), post_osc_f32_6.imag(), post_osc_f32_7.imag()},
            };

            r.band_gain = simd::Float256{1.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f};
        }
        else if constexpr (kFreqMode == FreqDistrbution::k1_n) {
            r.pre_osc = simd::Complex256{
                .re = simd::BroadcastF256(pre_osc_f32_8.real()),
                .im = simd::BroadcastF256(pre_osc_f32_8.imag()),
            };
            r.post_osc = simd::Complex256{
                .re = simd::BroadcastF256(post_osc_f32_8.real()),
                .im = simd::BroadcastF256(post_osc_f32_8.imag()),
            };

            r.pre_osc_n_val = simd::Complex256{
                .re = {pre_osc_f32_1.real(), pre_osc_f32_2.real(), pre_osc_f32_3.real(), pre_osc_f32_4.real(),
                       pre_osc_f32_5.real(), pre_osc_f32_6.real(), pre_osc_f32_7.real(), pre_osc_f32_8.real()},
                .im = {pre_osc_f32_1.imag(), pre_osc_f32_2.imag(), pre_osc_f32_3.imag(), pre_osc_f32_4.imag(),
                       pre_osc_f32_5.imag(), pre_osc_f32_6.imag(), pre_osc_f32_7.imag(), pre_osc_f32_8.imag()},
            };
            r.post_osc_n_val = simd::Complex256{
                .re = {post_osc_f32_1.real(), post_osc_f32_2.real(), post_osc_f32_3.real(), post_osc_f32_4.real(),
                       post_osc_f32_5.real(), post_osc_f32_6.real(), post_osc_f32_7.real(), post_osc_f32_8.real()},
                .im = {post_osc_f32_1.imag(), post_osc_f32_2.imag(), post_osc_f32_3.imag(), post_osc_f32_4.imag(),
                       post_osc_f32_5.imag(), post_osc_f32_6.imag(), post_osc_f32_7.imag(), post_osc_f32_8.imag()},
            };

            r.band_gain = simd::Float256{1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f} * 2.0f;
        }
        else if constexpr (kFreqMode == FreqDistrbution::k0_2n) {
            r.pre_osc = simd::Complex256{
                .re = simd::BroadcastF256(pre_osc_f32_16.real()),
                .im = simd::BroadcastF256(pre_osc_f32_16.imag()),
            };
            r.post_osc = simd::Complex256{
                .re = simd::BroadcastF256(post_osc_f32_16.real()),
                .im = simd::BroadcastF256(post_osc_f32_16.imag()),
            };

            r.pre_osc_n_val = simd::Complex256{
                .re = {pre_osc_f32_0.real(), pre_osc_f32_2.real(), pre_osc_f32_4.real(), pre_osc_f32_6.real(),
                       pre_osc_f32_8.real(), pre_osc_f32_10.real(), pre_osc_f32_12.real(), pre_osc_f32_14.real()},
                .im = {pre_osc_f32_0.imag(), pre_osc_f32_2.imag(), pre_osc_f32_4.imag(), pre_osc_f32_6.imag(),
                       pre_osc_f32_8.imag(), pre_osc_f32_10.imag(), pre_osc_f32_12.imag(), pre_osc_f32_14.imag()},
            };
            r.post_osc_n_val = simd::Complex256{
                .re = {post_osc_f32_0.real(), post_osc_f32_2.real(), post_osc_f32_4.real(), post_osc_f32_6.real(),
                       post_osc_f32_8.real(), post_osc_f32_10.real(), post_osc_f32_12.real(), post_osc_f32_14.real()},
                .im = {post_osc_f32_0.imag(), post_osc_f32_2.imag(), post_osc_f32_4.imag(), post_osc_f32_6.imag(),
                       post_osc_f32_8.imag(), post_osc_f32_10.imag(), post_osc_f32_12.imag(), post_osc_f32_14.imag()},
            };

            r.band_gain = simd::Float256{1.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f};
        }
        else if constexpr (kFreqMode == FreqDistrbution::k1_2n) {
            r.pre_osc = simd::Complex256{
                .re = simd::BroadcastF256(pre_osc_f32_16.real()),
                .im = simd::BroadcastF256(pre_osc_f32_16.imag()),
            };
            r.post_osc = simd::Complex256{
                .re = simd::BroadcastF256(post_osc_f32_16.real()),
                .im = simd::BroadcastF256(post_osc_f32_16.imag()),
            };

            r.pre_osc_n_val = simd::Complex256{
                .re = {pre_osc_f32_1.real(), pre_osc_f32_3.real(), pre_osc_f32_5.real(), pre_osc_f32_7.real(),
                       pre_osc_f32_9.real(), pre_osc_f32_11.real(), pre_osc_f32_13.real(), pre_osc_f32_15.real()},
                .im = {pre_osc_f32_1.imag(), pre_osc_f32_3.imag(), pre_osc_f32_5.imag(), pre_osc_f32_7.imag(),
                       pre_osc_f32_9.imag(), pre_osc_f32_11.imag(), pre_osc_f32_13.imag(), pre_osc_f32_15.imag()},
            };
            r.post_osc_n_val = simd::Complex256{
                .re = {post_osc_f32_1.real(), post_osc_f32_3.real(), post_osc_f32_5.real(), post_osc_f32_7.real(),
                       post_osc_f32_9.real(), post_osc_f32_11.real(), post_osc_f32_13.real(), post_osc_f32_15.real()},
                .im = {post_osc_f32_1.imag(), post_osc_f32_3.imag(), post_osc_f32_5.imag(), post_osc_f32_7.imag(),
                       post_osc_f32_9.imag(), post_osc_f32_11.imag(), post_osc_f32_13.imag(), post_osc_f32_15.imag()},
            };

            r.band_gain = simd::Float256{1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f} * 2.0f;
        }

        return r;
    }

    static simd::Float128 GetTailGain(simd::Float128, int num_warp) {
        constexpr simd::Array128<simd::Float128, 4> kBandGainLut{
            simd::Float128{1.0f, 1.0f, 1.0f, 1.0f},
            simd::Float128{1.0f, 0.0f, 0.0f, 0.0f},
            simd::Float128{1.0f, 1.0f, 0.0f, 0.0f},
            simd::Float128{1.0f, 1.0f, 1.0f, 0.0f},
        };
        return kBandGainLut[num_warp & 3] * 2.0f;
    }

    static simd::Float256 GetTailGain(simd::Float256, int num_warp) {
        constexpr simd::Array256<simd::Float256, 8> kBandGainLut{
            simd::Float256{1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
            simd::Float256{1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
            simd::Float256{1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
            simd::Float256{1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
            simd::Float256{1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f},
            simd::Float256{1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f},
            simd::Float256{1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f},
            simd::Float256{1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f},
        };
        return kBandGainLut[num_warp & 7] * 2.0f;
    }

    template <FreqDistrbution kFreqMode, int kPoles, bool kSmooth>
    void ProcessInternal_Stereo(float* left, float* right, int num_samples) noexcept {
        SimdT tail_gain = GetTailGain(SimdT{}, num_warps_);

        constexpr float half_pi = std::numbers::pi_v<float> / 2.0f;
        float target_wet = std::sin(drywet_ * half_pi);
        float target_dry = std::cos(drywet_ * half_pi);
        float inv_samples = 1.0f / static_cast<float>(num_samples);
        float delta_wet = (target_wet - last_wet_) * inv_samples;
        float delta_dry = (target_dry - last_dry_) * inv_samples;
        float wet_mix = last_wet_;
        float dry_mix = last_dry_;
        int simd_loop_count = (num_warps_ + (simd::LaneSize<SimdT> - 1)) / simd::LaneSize<SimdT>;

        if (num_warps_ <= simd::LaneSize<SimdT>) {
            if (kFreqMode == FreqDistrbution::k0_2n || kFreqMode == FreqDistrbution::k0_n) {
                tail_gain[0] = 1.0f;
            }
        }

        for (int i = 0; i < num_samples; i++) {
            if constexpr (kSmooth) {
                for (int j = 0; j < kPoles; ++j) {
                    tdf2_.SetPole(j, last_w_[j], last_q_[j], analog_fmul_);
                }
                for (int j = 0; j < kPoles; ++j) {
                    last_w_[j] += w_inc_[j];
                    last_q_[j] += q_inc_[j];
                }
                last_pre_osc_phase_inc_ += pre_osc_phase_inc_inc_;
                last_post_osc_phase_inc_ += post_osc_phase_inc_inc_;
            }
            const auto& coefficients = tdf2_.coefficients;

            // -------------------- tick complex sine generators --------------------
            pre_osc_phase_ += last_pre_osc_phase_inc_;
            pre_osc_phase_ -= std::floor(pre_osc_phase_);

            post_osc_phase_ += last_post_osc_phase_inc_;
            post_osc_phase_ -= std::floor(post_osc_phase_);

            // e^jwt
            constexpr float twopi = 2.0f * std::numbers::pi_v<float>;
            std::complex<float> pre_osc_f32 = {std::cos(pre_osc_phase_ * twopi), std::sin(pre_osc_phase_ * twopi)};
            std::complex<float> post_osc_f32 = {std::cos(post_osc_phase_ * twopi), std::sin(post_osc_phase_ * twopi)};

            auto r = GetComplexPhase<kFreqMode, SimdT>(pre_osc_f32, post_osc_f32);
            simd::SimdComplex<SimdT> pre_osc = r.pre_osc;
            simd::SimdComplex<SimdT> post_osc = r.post_osc;
            simd::SimdComplex<SimdT> pre_osc_n_val = r.pre_osc_n_val;
            simd::SimdComplex<SimdT> post_osc_n_val = r.post_osc_n_val;
            SimdT band_gain = r.band_gain;

            float x_left = left[i];
            float x_right = right[i];
            // -------------------- process bands --------------------
            auto* tdf2_state = tdf2_.state.data();
            SimdT y_l{};
            SimdT y_r{};

            for (int j = 0; j < simd_loop_count - 1; ++j) {
                // std::complex<float> tmp = x * pre_osc_n_val;
                // pre_osc_n_val *= pre_osc;
                auto tmp_l = simd::Broadcast<SimdT>(x_left) * pre_osc_n_val;
                auto tmp_r = simd::Broadcast<SimdT>(x_right) * pre_osc_n_val;
                pre_osc_n_val *= pre_osc;

                for (int k = 0; k < kPoles; ++k) {
                    const auto& c = coefficients[k];
                    ProcessTdf2(tmp_l.re, tmp_l.im, tdf2_state->z1_re_l, tdf2_state->z1_im_l,
                                tdf2_state->z2_re_l, tdf2_state->z2_im_l, c);
                    ProcessTdf2(tmp_r.re, tmp_r.im, tdf2_state->z1_re_r, tdf2_state->z1_im_r,
                                tdf2_state->z2_re_r, tdf2_state->z2_im_r, c);
                    ++tdf2_state;
                }

                // y += (tmp * post_osc_n_val).real();
                // post_osc_n_val *= post_osc;
                auto band_out_l = tmp_l * post_osc_n_val;
                auto band_out_r = tmp_r * post_osc_n_val;
                y_l += band_out_l.re * band_gain;
                y_r += band_out_r.re * band_gain;
                post_osc_n_val *= post_osc;

                band_gain = simd::Broadcast<SimdT>(2.0f);
            }

            // -------------------- here we have: 1/2/3/4 --------------------
            // std::complex<float> tmp = x * pre_osc_n_val;
            // pre_osc_n_val *= pre_osc;
            auto tmp_l = simd::Broadcast<SimdT>(x_left) * pre_osc_n_val;
            auto tmp_r = simd::Broadcast<SimdT>(x_right) * pre_osc_n_val;

            for (int k = 0; k < kPoles; ++k) {
                const auto& c = coefficients[k];
                ProcessTdf2(tmp_l.re, tmp_l.im, tdf2_state->z1_re_l, tdf2_state->z1_im_l, tdf2_state->z2_re_l,
                            tdf2_state->z2_im_l, c);
                ProcessTdf2(tmp_r.re, tmp_r.im, tdf2_state->z1_re_r, tdf2_state->z1_im_r, tdf2_state->z2_re_r,
                            tdf2_state->z2_im_r, c);
                ++tdf2_state;
            }

            // y += (tmp * post_osc_n_val).real();
            // post_osc_n_val *= post_osc;
            auto band_out_l = tmp_l * post_osc_n_val;
            auto band_out_r = tmp_r * post_osc_n_val;
            y_l += band_out_l.re * tail_gain;
            y_r += band_out_r.re * tail_gain;

            float wet_out_l = simd::ReduceAdd(y_l);
            float wet_out_r = simd::ReduceAdd(y_r);
            left[i] = wet_out_l * wet_mix + dry_mix * left[i];
            right[i] = wet_out_r * wet_mix + dry_mix * right[i];
            wet_mix += delta_wet;
            dry_mix += delta_dry;
        }
        last_dry_ = target_dry;
        last_wet_ = target_wet;
    }

    template <FreqDistrbution kFreqMode, int kPoles, bool kSmooth>
    void ProcessInternal_Mono(float* left, int num_samples) noexcept {
        SimdT tail_gain = GetTailGain(SimdT{}, num_warps_);

        constexpr float half_pi = std::numbers::pi_v<float> / 2.0f;
        float target_wet = std::sin(drywet_ * half_pi);
        float target_dry = std::cos(drywet_ * half_pi);
        float inv_samples = 1.0f / static_cast<float>(num_samples);
        float delta_wet = (target_wet - last_wet_) * inv_samples;
        float delta_dry = (target_dry - last_dry_) * inv_samples;
        float wet_mix = last_wet_;
        float dry_mix = last_dry_;

        int simd_loop_count = (num_warps_ + (simd::LaneSize<SimdT> - 1)) / simd::LaneSize<SimdT>;

        if (num_warps_ <= simd::LaneSize<SimdT>) {
            if (kFreqMode == FreqDistrbution::k0_2n || kFreqMode == FreqDistrbution::k0_n) {
                tail_gain[0] = 1.0f;
            }
        }

        for (int i = 0; i < num_samples; i++) {
            if constexpr (kSmooth) {
                for (int j = 0; j < kPoles; ++j) {
                    tdf2_.SetPole(j, last_w_[j], last_q_[j], analog_fmul_);
                }
                for (int j = 0; j < kPoles; ++j) {
                    last_w_[j] += w_inc_[j];
                    last_q_[j] += q_inc_[j];
                }
                last_pre_osc_phase_inc_ += pre_osc_phase_inc_inc_;
                last_post_osc_phase_inc_ += post_osc_phase_inc_inc_;
            }
            const auto& coefficients = tdf2_.coefficients;

            // -------------------- tick complex sine generators --------------------
            pre_osc_phase_ += last_pre_osc_phase_inc_;
            pre_osc_phase_ -= std::floor(pre_osc_phase_);

            post_osc_phase_ += last_post_osc_phase_inc_;
            post_osc_phase_ -= std::floor(post_osc_phase_);

            // e^jwt
            constexpr float twopi = 2.0f * std::numbers::pi_v<float>;
            std::complex<float> pre_osc_f32 = {std::cos(pre_osc_phase_ * twopi), std::sin(pre_osc_phase_ * twopi)};
            std::complex<float> post_osc_f32 = {std::cos(post_osc_phase_ * twopi), std::sin(post_osc_phase_ * twopi)};

            auto r = GetComplexPhase<kFreqMode, SimdT>(pre_osc_f32, post_osc_f32);
            simd::SimdComplex<SimdT> pre_osc = r.pre_osc;
            simd::SimdComplex<SimdT> post_osc = r.post_osc;
            simd::SimdComplex<SimdT> pre_osc_n_val = r.pre_osc_n_val;
            simd::SimdComplex<SimdT> post_osc_n_val = r.post_osc_n_val;
            SimdT band_gain = r.band_gain;

            float x_left = left[i];
            // -------------------- process bands --------------------
            auto* tdf2_state = tdf2_.state.data();
            SimdT y_l{};

            for (int j = 0; j < simd_loop_count - 1; ++j) {
                // std::complex<float> tmp = x * pre_osc_n_val;
                // pre_osc_n_val *= pre_osc;
                auto tmp_l = simd::Broadcast<SimdT>(x_left) * pre_osc_n_val;
                pre_osc_n_val *= pre_osc;

                for (int k = 0; k < kPoles; ++k) {
                    const auto& c = coefficients[k];
                    ProcessTdf2(tmp_l.re, tmp_l.im, tdf2_state->z1_re_l, tdf2_state->z1_im_l,
                                tdf2_state->z2_re_l, tdf2_state->z2_im_l, c);
                    ++tdf2_state;
                }

                // y += (tmp * post_osc_n_val).real();
                // post_osc_n_val *= post_osc;
                auto band_out_l = tmp_l * post_osc_n_val;
                y_l += band_out_l.re * band_gain;
                post_osc_n_val *= post_osc;

                band_gain = simd::Broadcast<SimdT>(2.0f);
            }

            // -------------------- here we have: 1/2/3/4 --------------------
            // std::complex<float> tmp = x * pre_osc_n_val;
            // pre_osc_n_val *= pre_osc;
            auto tmp_l = simd::Broadcast<SimdT>(x_left) * pre_osc_n_val;

            for (int k = 0; k < kPoles; ++k) {
                const auto& c = coefficients[k];
                ProcessTdf2(tmp_l.re, tmp_l.im, tdf2_state->z1_re_l, tdf2_state->z1_im_l, tdf2_state->z2_re_l,
                            tdf2_state->z2_im_l, c);
                ++tdf2_state;
            }

            // y += (tmp * post_osc_n_val).real();
            // post_osc_n_val *= post_osc;
            auto band_out_l = tmp_l * post_osc_n_val;
            y_l += band_out_l.re * tail_gain;

            float wet_out = simd::ReduceAdd(y_l);
            left[i] = wet_out * wet_mix + dry_mix * left[i];
            wet_mix += delta_wet;
            dry_mix += delta_dry;
        }
        last_dry_ = target_dry;
        last_wet_ = target_wet;
    }

    float fs_{};
    int num_warps_{};
    int poles_{};
    float drywet_{};
    bool pitch_affect_{};
    FreqDistrbution freq_distribution_{};

    // smooth
    int smooth_samples_{};
    int total_smooth_samples_{};
    std::array<float, global::kMaxPoles> w_{};
    std::array<float, global::kMaxPoles> q_{};
    std::array<float, global::kMaxPoles> w_inc_{};
    std::array<float, global::kMaxPoles> q_inc_{};
    std::array<float, global::kMaxPoles> last_w_{};
    std::array<float, global::kMaxPoles> last_q_{};
    float pre_osc_phase_inc_inc_{};
    float post_osc_phase_inc_inc_{};
    float last_pre_osc_phase_inc_{};
    float last_post_osc_phase_inc_{};
    float last_dry_{};
    float last_wet_{};

    // complex sine generator
    float pre_osc_phase_{};
    float pre_osc_phase_inc_{};
    float post_osc_phase_{};
    float post_osc_phase_inc_{};

    // complex lowpass filter
    float analog_fmul_{};
    Tdf2LaneN<inst, SimdT> tdf2_;
};

} // namespace warpcore

#pragma GCC diagnostic pop
