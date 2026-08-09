#pragma once
#include <algorithm>
#include <cmath>

#include "global.hpp"
#include "idsp.hpp"
#include "pluginshared/simd/simd.hpp"
#include "shortcircuit_sinc_delayline.hpp"
#include "window_kaiser.hpp"

namespace sttr {

// ------------------------------------------------------------
/** Delay-line based granular processor (STTR algorithm).

    Pure-DSP implementation of the STTR (Short time time reversal) effect — a
    granular delay with overlapping grains, dry/wet mix, and a Kaiser window.

    The window length (grain length = windowMul * hop) and the Kaiser beta are
    controlled by windowMul / windowBeta; the grain count is derived from
    windowMul.
*/
template <simd::Inst inst, class SimdT>
class DspImpl : public Idsp {
public:
    DspImpl() = default;

    /** Allocate stereo delay buffer and reset state. */
    void Prepare(float sample_rate) override {
        sample_rate_ = sample_rate;

        // size the delay line for the worst-case read depth
        delay_line_.Init(global::kMaxDelayMs, sample_rate);

        // initial hop value and one-pole slew
        float const init_hop = MillisecondsToSamples(hop_ms_, sample_rate_);
        hop_slw_.SetSlewTime(sample_rate_, global::kHopSlewMs);
        hop_slw_.target = init_hop;
        hop_slw_.value = init_hop;

        stretch_slw_.SetSlewTime(sample_rate_, global::kStretchSlewMs);
        stretch_slw_.target = stretch_;
        stretch_slw_.value = stretch_;

        dry_delay_slw_.SetSlewTime(sample_rate, global::kDryDelaySlewMs);
        dry_delay_slw_.target = dry_delay_;
        dry_delay_slw_.value = dry_delay_;

        mix_slw_.SetSlewTime(sample_rate, global::kMixSlewMs);
        mix_slw_.target = mix_;
        mix_slw_.value = mix_;

        Reset();
    }

    /** Clear delay buffer and reset read/write positions. */
    void Reset() override {
        delay_line_.Clear();

        // initialise per-grain phases, staggered evenly
        int const n = num_grains_ > 0 ? num_grains_ : 2;
        simd::Float128 init{};
        for (int g = 0; g < global::kMaxGrains; ++g)
            init[g] = (n > 1) ? static_cast<float>(g % n) / static_cast<float>(n) : 0.0f;
        grain_phase_ = init;
    }

    /** Set all parameters atomically: copies into internal state, pulls
        smoother targets and re-syncs the grain count. Only needs to be
        called when a parameter actually changes. */
    void SetParameters(SttrParam const& params) override {
        mix_ = params.mix;
        hop_ms_ = params.hop_ms;
        dry_delay_ = params.dry_delay;
        stretch_ = params.stretch;

        window_mul_ = params.window_mul;
        window_beta_ = params.window_beta;
        reverse_ = params.reverse;

        mix_slw_.target = mix_;
        hop_slw_.target = std::max(MillisecondsToSamples(hop_ms_, sample_rate_), 1.0f);
        dry_delay_slw_.target = dry_delay_;
        stretch_slw_.target = stretch_;

        window_fn_.SetBeta(window_beta_);

        // re-sync the grain count; only a windowMul change alters it, and only
        // then should the delay line be flushed
        int const n = GrainsForMul(window_mul_);
        if (n != num_grains_) {
            num_grains_ = n;
            Reset();
        }
    }

    /** Process one stereo audio block in-place. */
    void ProcessBlock(float* left, float* right, int numSamples) override {
        if (reverse_)
            ProcessGrainsBlock<true>(left, right, numSamples);
        else
            ProcessGrainsBlock<false>(left, right, numSamples);
    }

    std::string_view InstName() override {
        return INST_NAME;
    }
private:
    // one-pole exponential smoother: value += slew * (target - value); the
    // per-sample slew coefficient is derived from a time constant + sample rate
    struct SlewedParam {
        float target, value, slew;
        SlewedParam(float v, float s) noexcept
            : target(v)
            , value(v)
            , slew(s) {}

        /** Set the slew coefficient from a time constant in milliseconds. */
        void SetSlewTime(float sampleRate, float ms) noexcept {
            // slew = 1 - exp(-1 / (tau * fs))
            slew = 1.0f - std::exp(-1.0f / (ms * sampleRate * 0.001f));
        }

        float Step() noexcept {
            float diff = target - value;
            if (std::abs(diff) < 1.0e-20f)
                value = target;
            else
                value = value + diff * slew;
            return value;
        }
    };

    /** Number of active grains for the given window-length multiplier. */
    static int GrainsForMul(int mul) noexcept {
        return std::clamp(mul, 1, global::kMaxGrains);
    }

    static float MillisecondsToSamples(float ms, float sr) {
        return ms / 1000.0f * sr;
    }

    static float Interp(float a, float b, float d) {
        return a * (1.0f - d) + b * d;
    }

    template <int N, bool Reverse>
    void ProcessGrains(float* left, float* right, int numSamples);

    template <bool Reverse>
    void ProcessGrainsBlock(float* left, float* right, int numSamples) {
        switch (num_grains_) {
            case 1:
                ProcessGrains<1, Reverse>(left, right, numSamples);
                break;
            case 2:
                ProcessGrains<2, Reverse>(left, right, numSamples);
                break;
            case 3:
                ProcessGrains<3, Reverse>(left, right, numSamples);
                break;
            default:
                ProcessGrains<4, Reverse>(left, right, numSamples);
                break;
        }
    }

    // internal state
    float sample_rate_{44100.0f};
    int num_grains_{2};             // derived from window_mul_
    Window<inst, SimdT> window_fn_; // current Kaiser window

    float mix_{0.5f};
    float hop_ms_{16.0f};
    float dry_delay_{0.0f};
    float stretch_{1.0f};
    int window_mul_{2};       // grain length multiplier, integer [1, 4]
    float window_beta_{8.0f}; // Kaiser window beta
    bool reverse_{true};      // false = forward (sequential) grain playback

    SlewedParam hop_slw_{0.0f, 0.0f};     // hop length (samples), one-pole slew
    SlewedParam stretch_slw_{1.0f, 0.0f}; // stretch ratio, one-pole slew
    simd::Float128 grain_phase_{};        // per-grain phase [0, 1), 4 lanes (max 4 grains)

    SlewedParam dry_delay_slw_{0.0f, 0.000167f};
    SlewedParam mix_slw_{0.5f, 0.15f};

    // stereo sinc delay line (owns its sinc table)
    ShortcircuitSincDelayLine<inst, SimdT> delay_line_;
};

// ------------------------------------------------------------
template <simd::Inst inst, class SimdT>
template <int N, bool Reverse>
void DspImpl<inst, SimdT>::ProcessGrains(float* left, float* right, int numSamples) {
    // grains g >= N are inactive: read<N> returns zero for their lanes, so they
    // are automatically excluded from the wet sum below
    for (int n = 0; n < numSamples; ++n) {
        float const hop_samps = hop_slw_.Step();
        float const stretch = stretch_slw_.Step();
        float const mix = mix_slw_.Step();
        float const grain_len = hop_samps * static_cast<float>(window_mul_);
        float const phase_inc = 1.0f / grain_len; // fixed window-envelope rate

        delay_line_.Write(left[n], right[n]);

        simd::Float128 const phase = grain_phase_;
        simd::Float128 const win = window_fn_.Value(phase);

        // read delay (samples behind the write head):
        //   reverse -> read pointer walks backward at speed 1 - (1+stretch) = -stretch,
        //              so pitch = stretch (formant maps to correct semitones)
        //   forward -> read pointer speed = 1 - d(delay)/dn = stretch, pitch = stretch
        simd::Float128 read_delay;
        if constexpr (Reverse)
            read_delay = simd::BroadcastF128(1.0f + stretch) * phase * simd::BroadcastF128(grain_len);
        else
            read_delay = simd::BroadcastF128(grain_len) + simd::BroadcastF128((1.0f - stretch) * grain_len) * phase;

        simd::Float128 gl{};
        simd::Float128 gr{};
        delay_line_.template Read<N>(read_delay, gl, gr);
        float const sumL = simd::ReduceAdd(win * gl);
        float const sumR = simd::ReduceAdd(win * gr);

        grain_phase_ = simd::Frac(phase + simd::BroadcastF128(phase_inc));

        float const dryDelaySamps = grain_len * 1.5f * dry_delay_slw_.value;
        simd::Float128 dryL, dryR;
        delay_line_.template Read<1>(simd::Float128{dryDelaySamps, 0.0f, 0.0f, 0.0f}, dryL, dryR);
        left[n] = Interp(dryL[0], sumL, mix);
        right[n] = Interp(dryR[0], sumR, mix);

        dry_delay_slw_.Step();
    }
}

} // namespace sttr
