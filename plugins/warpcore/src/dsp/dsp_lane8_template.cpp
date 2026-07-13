#include "dsp_state.hpp"

#include <algorithm>
#include <cassert>
#include <complex>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"

namespace warpcore {

struct ComplexPhase256 {
    simd::Complex256 pre_osc;
    simd::Complex256 post_osc;
    simd::Complex256 pre_osc_n_val;
    simd::Complex256 post_osc_n_val;
    simd::Float256 band_gain;
};
template <FreqDistrbution kFreqMode>
static ComplexPhase256 _GetComplexPhase256(std::complex<float> pre_osc_f32, std::complex<float> post_osc_f32) {
    ComplexPhase256 r;

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

template <FreqDistrbution kFreqMode, int kPoles, bool kSmooth>
static void ProcessInternal_Stereo(warpcore::ProcessorState& state, float* left, float* right,
                                   int num_samples) noexcept {
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
    simd::Float256 tail_gain = kBandGainLut[state.num_warps & 7] * 2.0f;

    constexpr float half_pi = std::numbers::pi_v<float> / 2.0f;
    float target_wet = std::sin(state.drywet * half_pi);
    float target_dry = std::cos(state.drywet * half_pi);
    float inv_samples = 1.0f / static_cast<float>(num_samples);
    float delta_wet = (target_wet - state.last_wet_) * inv_samples;
    float delta_dry = (target_dry - state.last_dry_) * inv_samples;
    float wet_mix = state.last_wet_;
    float dry_mix = state.last_dry_;
    int simd_loop_count = (state.num_warps + 7) / 8;

    if (state.num_warps <= 8) {
        if (kFreqMode == FreqDistrbution::k0_2n || kFreqMode == FreqDistrbution::k0_n) {
            tail_gain[0] = 1.0f;
        }
    }

    for (int i = 0; i < num_samples; i++) {
        if constexpr (kSmooth) {
            for (int j = 0; j < kPoles; ++j) {
                state.svf256.SetPole(j, state.last_w_[j], state.last_q_[j], state.analog_fmul);
            }
            for (int j = 0; j < kPoles; ++j) {
                state.last_w_[j] += state.w_inc_[j];
                state.last_q_[j] += state.q_inc_[j];
            }
            state.last_pre_osc_phase_inc_ += state.pre_osc_phase_inc_inc_;
            state.last_post_osc_phase_inc_ += state.post_osc_phase_inc_inc_;
        }
        const auto& svf_g = state.svf256.g;
        const auto& svf_d = state.svf256.d;

        // -------------------- tick complex sine generators --------------------
        state.pre_osc_phase += state.last_pre_osc_phase_inc_;
        state.pre_osc_phase -= std::floor(state.pre_osc_phase);

        state.post_osc_phase += state.last_post_osc_phase_inc_;
        state.post_osc_phase -= std::floor(state.post_osc_phase);

        // e^jwt
        constexpr float twopi = 2.0f * std::numbers::pi_v<float>;
        std::complex<float> pre_osc_f32 = {std::cos(state.pre_osc_phase * twopi),
                                           std::sin(state.pre_osc_phase * twopi)};
        std::complex<float> post_osc_f32 = {std::cos(state.post_osc_phase * twopi),
                                            std::sin(state.post_osc_phase * twopi)};

        simd::Complex256 pre_osc;
        simd::Complex256 post_osc;
        simd::Complex256 pre_osc_n_val;
        simd::Complex256 post_osc_n_val;
        simd::Float256 band_gain;
        auto r = _GetComplexPhase256<kFreqMode>(pre_osc_f32, post_osc_f32);
        pre_osc = r.pre_osc;
        post_osc = r.post_osc;
        pre_osc_n_val = r.pre_osc_n_val;
        post_osc_n_val = r.post_osc_n_val;
        band_gain = r.band_gain;

        float x_left = left[i];
        float x_right = right[i];

        // -------------------- process bands --------------------
        auto* svf_state = state.svf256.state.data();
        simd::Float256 y_l{};
        simd::Float256 y_r{};

        for (int j = 0; j < simd_loop_count - 1; ++j) {
            // std::complex<float> tmp = x * pre_osc_n_val;
            // pre_osc_n_val *= pre_osc;
            auto tmp_l = simd::BroadcastF256(x_left) * pre_osc_n_val;
            auto tmp_r = simd::BroadcastF256(x_right) * pre_osc_n_val;
            pre_osc_n_val *= pre_osc;

            for (int k = 0; k < kPoles; ++k) {
                const float gk = svf_g[k];
                const float dk = svf_d[k];

                auto s1_re_l = svf_state->s1_re_l;
                auto s1_im_l = svf_state->s1_im_l;
                auto s2_re_l = svf_state->s2_re_l;
                auto s2_im_l = svf_state->s2_im_l;

                auto s1_re_r = svf_state->s1_re_r;
                auto s1_im_r = svf_state->s1_im_r;
                auto s2_re_r = svf_state->s2_re_r;
                auto s2_im_r = svf_state->s2_im_r;

                auto bp_re_l = dk * (gk * (tmp_l.re - s2_re_l) + s1_re_l);
                auto bp_im_l = dk * (gk * (tmp_l.im - s2_im_l) + s1_im_l);
                auto v1_re_l = bp_re_l - s1_re_l;
                auto v1_im_l = bp_im_l - s1_im_l;
                auto v2_re_l = gk * bp_re_l;
                auto v2_im_l = gk * bp_im_l;
                auto lp_re_l = v2_re_l + s2_re_l;
                auto lp_im_l = v2_im_l + s2_im_l;

                auto bp_re_r = dk * (gk * (tmp_r.re - s2_re_r) + s1_re_r);
                auto bp_im_r = dk * (gk * (tmp_r.im - s2_im_r) + s1_im_r);
                auto v1_re_r = bp_re_r - s1_re_r;
                auto v1_im_r = bp_im_r - s1_im_r;
                auto v2_re_r = gk * bp_re_r;
                auto v2_im_r = gk * bp_im_r;
                auto lp_re_r = v2_re_r + s2_re_r;
                auto lp_im_r = v2_im_r + s2_im_r;

                svf_state->s1_re_l = bp_re_l + v1_re_l;
                svf_state->s1_im_l = bp_im_l + v1_im_l;
                svf_state->s2_re_l = lp_re_l + v2_re_l;
                svf_state->s2_im_l = lp_im_l + v2_im_l;
                svf_state->s1_re_r = bp_re_r + v1_re_r;
                svf_state->s1_im_r = bp_im_r + v1_im_r;
                svf_state->s2_re_r = lp_re_r + v2_re_r;
                svf_state->s2_im_r = lp_im_r + v2_im_r;

                ++svf_state;

                tmp_l.re = lp_re_l;
                tmp_l.im = lp_im_l;
                tmp_r.re = lp_re_r;
                tmp_r.im = lp_im_r;
            }

            // y += (tmp * post_osc_n_val).real();
            // post_osc_n_val *= post_osc;
            auto band_out_l = tmp_l * post_osc_n_val;
            auto band_out_r = tmp_r * post_osc_n_val;
            y_l += band_out_l.re * band_gain;
            y_r += band_out_r.re * band_gain;
            post_osc_n_val *= post_osc;

            band_gain = simd::BroadcastF256(2.0f);
        }

        // -------------------- here we have: 1/2/3/4/5/6/7/8 --------------------
        // std::complex<float> tmp = x * pre_osc_n_val;
        // pre_osc_n_val *= pre_osc;
        auto tmp_l = simd::BroadcastF256(x_left) * pre_osc_n_val;
        auto tmp_r = simd::BroadcastF256(x_right) * pre_osc_n_val;

        for (int k = 0; k < kPoles; ++k) {
            const float gk = svf_g[k];
            const float dk = svf_d[k];

            auto s1_re_l = svf_state->s1_re_l;
            auto s1_im_l = svf_state->s1_im_l;
            auto s2_re_l = svf_state->s2_re_l;
            auto s2_im_l = svf_state->s2_im_l;

            auto s1_re_r = svf_state->s1_re_r;
            auto s1_im_r = svf_state->s1_im_r;
            auto s2_re_r = svf_state->s2_re_r;
            auto s2_im_r = svf_state->s2_im_r;

            auto bp_re_l = dk * (gk * (tmp_l.re - s2_re_l) + s1_re_l);
            auto bp_im_l = dk * (gk * (tmp_l.im - s2_im_l) + s1_im_l);
            auto v1_re_l = bp_re_l - s1_re_l;
            auto v1_im_l = bp_im_l - s1_im_l;
            auto v2_re_l = gk * bp_re_l;
            auto v2_im_l = gk * bp_im_l;
            auto lp_re_l = v2_re_l + s2_re_l;
            auto lp_im_l = v2_im_l + s2_im_l;

            auto bp_re_r = dk * (gk * (tmp_r.re - s2_re_r) + s1_re_r);
            auto bp_im_r = dk * (gk * (tmp_r.im - s2_im_r) + s1_im_r);
            auto v1_re_r = bp_re_r - s1_re_r;
            auto v1_im_r = bp_im_r - s1_im_r;
            auto v2_re_r = gk * bp_re_r;
            auto v2_im_r = gk * bp_im_r;
            auto lp_re_r = v2_re_r + s2_re_r;
            auto lp_im_r = v2_im_r + s2_im_r;

            svf_state->s1_re_l = bp_re_l + v1_re_l;
            svf_state->s1_im_l = bp_im_l + v1_im_l;
            svf_state->s2_re_l = lp_re_l + v2_re_l;
            svf_state->s2_im_l = lp_im_l + v2_im_l;
            svf_state->s1_re_r = bp_re_r + v1_re_r;
            svf_state->s1_im_r = bp_im_r + v1_im_r;
            svf_state->s2_re_r = lp_re_r + v2_re_r;
            svf_state->s2_im_r = lp_im_r + v2_im_r;

            ++svf_state;

            tmp_l.re = lp_re_l;
            tmp_l.im = lp_im_l;
            tmp_r.re = lp_re_r;
            tmp_r.im = lp_im_r;
        }

        // y += (tmp * post_osc_n_val).real();
        // post_osc_n_val *= post_osc;
        auto band_out_l = tmp_l * post_osc_n_val;
        auto band_out_r = tmp_r * post_osc_n_val;
        y_l += band_out_l.re * tail_gain;
        y_r += band_out_r.re * tail_gain;

        left[i] = simd::ReduceAdd(y_l) * wet_mix + dry_mix * left[i];
        right[i] = simd::ReduceAdd(y_r) * wet_mix + dry_mix * right[i];
        wet_mix += delta_wet;
        dry_mix += delta_dry;
    }
    state.last_dry_ = target_dry;
    state.last_wet_ = target_wet;
}

template <FreqDistrbution kFreqMode, int kPoles, bool kSmooth>
static void ProcessInternal_Mono(warpcore::ProcessorState& state, float* left, int num_samples) noexcept {
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
    simd::Float256 tail_gain = kBandGainLut[state.num_warps & 7] * 2.0f;

    constexpr float half_pi = std::numbers::pi_v<float> / 2.0f;
    float target_wet = std::sin(state.drywet * half_pi);
    float target_dry = std::cos(state.drywet * half_pi);
    float inv_samples = 1.0f / static_cast<float>(num_samples);
    float delta_wet = (target_wet - state.last_wet_) * inv_samples;
    float delta_dry = (target_dry - state.last_dry_) * inv_samples;
    float wet_mix = state.last_wet_;
    float dry_mix = state.last_dry_;

    int simd_loop_count = (state.num_warps + 7) / 8;

    if (state.num_warps <= 8) {
        if (kFreqMode == FreqDistrbution::k0_2n || kFreqMode == FreqDistrbution::k0_n) {
            tail_gain[0] = 1.0f;
        }
    }

    for (int i = 0; i < num_samples; i++) {
        if constexpr (kSmooth) {
            for (int j = 0; j < kPoles; ++j) {
                state.svf256.SetPole(j, state.last_w_[j], state.last_q_[j], state.analog_fmul);
            }
            for (int j = 0; j < kPoles; ++j) {
                state.last_w_[j] += state.w_inc_[j];
                state.last_q_[j] += state.q_inc_[j];
            }
            state.last_pre_osc_phase_inc_ += state.pre_osc_phase_inc_inc_;
            state.last_post_osc_phase_inc_ += state.post_osc_phase_inc_inc_;
        }
        const auto& svf_g = state.svf256.g;
        const auto& svf_d = state.svf256.d;

        // -------------------- tick complex sine generators --------------------
        state.pre_osc_phase += state.last_pre_osc_phase_inc_;
        state.pre_osc_phase -= std::floor(state.pre_osc_phase);

        state.post_osc_phase += state.last_post_osc_phase_inc_;
        state.post_osc_phase -= std::floor(state.post_osc_phase);

        // e^jwt
        constexpr float twopi = 2.0f * std::numbers::pi_v<float>;
        std::complex<float> pre_osc_f32 = {std::cos(state.pre_osc_phase * twopi),
                                           std::sin(state.pre_osc_phase * twopi)};
        std::complex<float> post_osc_f32 = {std::cos(state.post_osc_phase * twopi),
                                            std::sin(state.post_osc_phase * twopi)};

        simd::Complex256 pre_osc;
        simd::Complex256 post_osc;
        simd::Complex256 pre_osc_n_val;
        simd::Complex256 post_osc_n_val;
        simd::Float256 band_gain;
        auto r = _GetComplexPhase256<kFreqMode>(pre_osc_f32, post_osc_f32);
        pre_osc = r.pre_osc;
        post_osc = r.post_osc;
        pre_osc_n_val = r.pre_osc_n_val;
        post_osc_n_val = r.post_osc_n_val;
        band_gain = r.band_gain;

        float x_left = left[i];
        // -------------------- process bands --------------------
        auto* svf_state = state.svf256.state.data();
        simd::Float256 y_l{};

        for (int j = 0; j < simd_loop_count - 1; ++j) {
            // std::complex<float> tmp = x * pre_osc_n_val;
            // pre_osc_n_val *= pre_osc;
            auto tmp_l = simd::BroadcastF256(x_left) * pre_osc_n_val;
            pre_osc_n_val *= pre_osc;

            for (int k = 0; k < kPoles; ++k) {
                const float gk = svf_g[k];
                const float dk = svf_d[k];

                auto s1_re_l = svf_state->s1_re_l;
                auto s1_im_l = svf_state->s1_im_l;
                auto s2_re_l = svf_state->s2_re_l;
                auto s2_im_l = svf_state->s2_im_l;

                auto bp_re_l = dk * (gk * (tmp_l.re - s2_re_l) + s1_re_l);
                auto bp_im_l = dk * (gk * (tmp_l.im - s2_im_l) + s1_im_l);
                auto v1_re_l = bp_re_l - s1_re_l;
                auto v1_im_l = bp_im_l - s1_im_l;
                auto v2_re_l = gk * bp_re_l;
                auto v2_im_l = gk * bp_im_l;
                auto lp_re_l = v2_re_l + s2_re_l;
                auto lp_im_l = v2_im_l + s2_im_l;

                svf_state->s1_re_l = bp_re_l + v1_re_l;
                svf_state->s1_im_l = bp_im_l + v1_im_l;
                svf_state->s2_re_l = lp_re_l + v2_re_l;
                svf_state->s2_im_l = lp_im_l + v2_im_l;

                ++svf_state;

                tmp_l.re = lp_re_l;
                tmp_l.im = lp_im_l;
            }

            // y += (tmp * post_osc_n_val).real();
            // post_osc_n_val *= post_osc;
            auto band_out_l = tmp_l * post_osc_n_val;
            y_l += band_out_l.re * band_gain;
            post_osc_n_val *= post_osc;

            band_gain = simd::BroadcastF256(2.0f);
        }

        // -------------------- here we have: 1/2/3/4/5/6/7/8 --------------------
        // std::complex<float> tmp = x * pre_osc_n_val;
        // pre_osc_n_val *= pre_osc;
        auto tmp_l = simd::BroadcastF256(x_left) * pre_osc_n_val;

        for (int k = 0; k < kPoles; ++k) {
            const float gk = svf_g[k];
            const float dk = svf_d[k];

            auto s1_re_l = svf_state->s1_re_l;
            auto s1_im_l = svf_state->s1_im_l;
            auto s2_re_l = svf_state->s2_re_l;
            auto s2_im_l = svf_state->s2_im_l;

            auto bp_re_l = dk * (gk * (tmp_l.re - s2_re_l) + s1_re_l);
            auto bp_im_l = dk * (gk * (tmp_l.im - s2_im_l) + s1_im_l);
            auto v1_re_l = bp_re_l - s1_re_l;
            auto v1_im_l = bp_im_l - s1_im_l;
            auto v2_re_l = gk * bp_re_l;
            auto v2_im_l = gk * bp_im_l;
            auto lp_re_l = v2_re_l + s2_re_l;
            auto lp_im_l = v2_im_l + s2_im_l;

            svf_state->s1_re_l = bp_re_l + v1_re_l;
            svf_state->s1_im_l = bp_im_l + v1_im_l;
            svf_state->s2_re_l = lp_re_l + v2_re_l;
            svf_state->s2_im_l = lp_im_l + v2_im_l;

            ++svf_state;

            tmp_l.re = lp_re_l;
            tmp_l.im = lp_im_l;
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
    state.last_dry_ = target_dry;
    state.last_wet_ = target_wet;
}

template <FreqDistrbution kFreqMode, int kPoles, bool kSmooth>
static void ProcessInternal(warpcore::ProcessorState& state, float* left, float* right, int num_samples) noexcept {
    if (right == nullptr) {
        // 这可能导致一些cache问题，不过不用重写DspState布局
        ProcessInternal_Mono<kFreqMode, kPoles, kSmooth>(state, left, num_samples);
    }
    else {
        ProcessInternal_Stereo<kFreqMode, kPoles, kSmooth>(state, left, right, num_samples);
    }
}

template <FreqDistrbution kFreqMode, bool kSmooth>
static void ProcessPoles(warpcore::ProcessorState& state, float* left, float* right, int num_samples) noexcept {
    switch (state.poles) {
        case 1:
            ProcessInternal<kFreqMode, 1, kSmooth>(state, left, right, num_samples);
            break;
        case 2:
            ProcessInternal<kFreqMode, 2, kSmooth>(state, left, right, num_samples);
            break;
        case 3:
            ProcessInternal<kFreqMode, 3, kSmooth>(state, left, right, num_samples);
            break;
        case 4:
            ProcessInternal<kFreqMode, 4, kSmooth>(state, left, right, num_samples);
            break;
        case 5:
            ProcessInternal<kFreqMode, 5, kSmooth>(state, left, right, num_samples);
            break;
        case 6:
            ProcessInternal<kFreqMode, 6, kSmooth>(state, left, right, num_samples);
            break;
        case 7:
            ProcessInternal<kFreqMode, 7, kSmooth>(state, left, right, num_samples);
            break;
        case 8:
            ProcessInternal<kFreqMode, 8, kSmooth>(state, left, right, num_samples);
            break;
        default:
            assert(false);
            break;
    }
}

// ----------------------------------------
// dsp processor
// ----------------------------------------

static void Init(warpcore::ProcessorState& state, float fs) noexcept {
    state.fs = fs;
    state.total_smooth_samples = static_cast<int>(fs * 10.0f / 1000.0f);
}

static void Reset(warpcore::ProcessorState& state) noexcept {
    state.svf256.Reset();
    state.pre_osc_phase = 0.0f;
    state.post_osc_phase = 0.0f;
    state.StopSmooth();
}

static void Update(warpcore::ProcessorState& state, const warpcore::Param& p) noexcept {
    state.Update<simd::Float256>(p);
}

template <bool kSmooth>
static void Process2(warpcore::ProcessorState& state, float* left, float* right, int num_samples) noexcept {
    switch (state.freq_distribution) {
        case FreqDistrbution::k0_n:
            ProcessPoles<FreqDistrbution::k0_n, kSmooth>(state, left, right, num_samples);
            break;
        case FreqDistrbution::k1_n:
            ProcessPoles<FreqDistrbution::k1_n, kSmooth>(state, left, right, num_samples);
            break;
        case FreqDistrbution::k0_2n:
            ProcessPoles<FreqDistrbution::k0_2n, kSmooth>(state, left, right, num_samples);
            break;
        case FreqDistrbution::k1_2n:
            ProcessPoles<FreqDistrbution::k1_2n, kSmooth>(state, left, right, num_samples);
            break;
    }
}

static void Process(warpcore::ProcessorState& state, float* left, float* right, int num_samples) noexcept {
    while (num_samples != 0) {
        int blocK_size = num_samples;
        if (state.smooth_samples != 0) {
            blocK_size = std::min(blocK_size, state.smooth_samples);
            state.smooth_samples -= blocK_size;
            Process2<true>(state, left, right, blocK_size);

            if (state.smooth_samples == 0) {
                state.StopSmooth();
                for (int i = 0; i < state.poles; ++i) {
                    state.svf256.SetPole(i, state.w_[i], state.q_[i], state.analog_fmul);
                }
            }
        }
        else {
            Process2<false>(state, left, right, blocK_size);
        }
        num_samples -= blocK_size;
        left += blocK_size;
        if (right != nullptr) {
            right += blocK_size;
        }
    }
}

// ----------------------------------------
// export
// ----------------------------------------

#ifndef DSP_EXPORT_NAME
#error "不应该编译这个文件,在其他cpp包含这个cpp并定义DSP_EXPORT_NAME=`dsp_dispatch.cpp里的变量`"
#endif

ProcessorDsp DSP_EXPORT_NAME{Init, Reset, Update, Process, DSP_INST_NAME};
} // namespace warpcore

#pragma GCC diagnostic pop
