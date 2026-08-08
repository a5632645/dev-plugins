#pragma once
#include <algorithm>
#include <cmath>
#include <concepts>

#include <juce_audio_basics/juce_audio_basics.h>

#include "idsp.hpp"
#include "pluginshared/simd/simd.hpp"
#include "shortcircuit_sinc_delayline.hpp"
#include "window_kaiser.hpp"

namespace sttr {

//==============================================================================
/** Delay-line based granular processor (STTR algorithm).

    Pure-DSP implementation of the STTR (Short time time reversal) effect — a
    granular delay with overlapping grains, dry/wet mix, and a Kaiser window.

    The window length (grain length = windowMul * hop) and the Kaiser beta are
    controlled by windowMul / windowBeta; the grain count is derived from
    windowMul.

    SIMD-dispatched: instantiated per ISA in dsp_{sse2,sse4,neon}.cpp.
 */
template <simd::Inst inst, class SimdT>
class DspImpl : public Idsp {
public:
    static constexpr float kMaxHopMs = 500.0f;
    static constexpr int kMaxGrains = 4;

    // Worst-case read depth: 2 * stretch_max(2^(10/12)) * hop_max * mul_max,
    // in milliseconds. The delay line is sized dynamically in Prepare().
    static constexpr float kMaxDelayMs = 2.0f * 1.7818f * kMaxHopMs * static_cast<float>(kMaxGrains);

    DspImpl() = default;

    /** Allocate stereo delay buffer and reset state. */
    void Prepare(float sampleRate) override {
        sampleRate_ = sampleRate;

        // Size the delay line for the worst-case read depth
        delayLine_.Init(kMaxDelayMs, sampleRate);

        // Initial hop value and 40ms linear ramp
        float initHop = millisecondsToSamples(hopMs_, sampleRate_);
        hopSmoother_.reset(sampleRate_, 0.04);
        hopSmoother_.setCurrentAndTargetValue(initHop);

        stretchSmoother_.reset(sampleRate_, 0.04);
        stretchSmoother_.setCurrentAndTargetValue(stretch_);

        Reset();
    }

    /** Clear delay buffer and reset read/write positions. */
    void Reset() override {
        delayLine_.clear();

        // Initialise per-grain phases (staggered evenly)
        int const n = numGrains_ > 0 ? numGrains_ : 2;
        simd::Float128 init{};
        for (int g = 0; g < kMaxGrains; ++g)
            init[g] = (n > 1) ? static_cast<float>(g % n) / static_cast<float>(n) : 0.0f;
        grainPhase_ = init;
    }

    /** Set all parameters atomically: copies into internal state, pulls
        smoother targets and re-syncs the grain count. Only needs to be
        called when a parameter actually changes. */
    void SetParameters(SttrParam const& params) override {
        mix_ = params.mix;
        hopMs_ = params.hopMs;
        dryDelay_ = params.dryDelay;
        stretch_ = params.stretch;

        windowMul_ = params.windowMul;
        windowBeta_ = params.windowBeta;

        // pull smoother targets — SetParameters only runs when a parameter
        // changed, so the window is applied unconditionally.
        mixSlw_.target = mix_;
        hopSmoother_.setTargetValue(std::max(millisecondsToSamples(hopMs_, sampleRate_), 1.0f));
        dryDelaySlw_.target = dryDelay_;
        // stretch ratio = 2^(formant/12), formant in [-10, 10] st
        stretchSmoother_.setTargetValue(std::clamp(stretch_, 0.561231f, 1.781797f));

        windowFn_.setBeta(windowBeta_);

        // re-sync the grain count; only a windowMul change alters it, and only
        // then should the delay line be flushed.
        int const n = grainsForMul(windowMul_);
        if (n != numGrains_) {
            numGrains_ = n;
            Reset();
        }
    }

    /** Process one stereo audio block in-place. */
    void ProcessBlock(float* left, float* right, int numSamples) override {
        switch (numGrains_) {
            case 1:
                processGrains<1>(left, right, numSamples);
                break;
            case 2:
                processGrains<2>(left, right, numSamples);
                break;
            case 3:
                processGrains<3>(left, right, numSamples);
                break;
            default:
                processGrains<4>(left, right, numSamples);
                break;
        }
    }

    std::string_view InstName() override {
        return INST_NAME;
    }

private:
    // helpers
    /** Number of active grains for the given window-length multiplier. */
    static int grainsForMul(int mul) noexcept {
        return std::clamp(mul, 1, kMaxGrains);
    }

    static float millisecondsToSamples(float ms, float sr) {
        return ms / 1000.0f * sr;
    }

    static float interp(float a, float b, float d) {
        return a * (1.0f - d) + b * d;
    }

    // slewed parameter (for dryDelay/mix)
    struct SlewedParam {
        float target, value, slew;
        SlewedParam(float v, float s) noexcept
            : target(v)
            , value(v)
            , slew(s) {}
        void step() noexcept {
            float diff = target - value;
            if (std::abs(diff) < 1.0e-20f) {
                value = target;
                return;
            }
            value = value + diff * slew;
        }
    };

    // template dispatch — the delay-line read<N> selects the 128-bit or 256-bit
    // dot path internally via requires on SimdT.
    template <int N>
    void processGrains(float* left, float* right, int numSamples);

    // internal state
    float sampleRate_{44100.0f};
    int numGrains_{2};   // derived from windowMul_
    Window<inst, SimdT> windowFn_; // current Kaiser window

    float mix_{0.5f};
    float hopMs_{16.0f};
    float dryDelay_{0.0f};
    float stretch_{1.0f};
    int windowMul_{2};        // grain length multiplier, integer [1, 4]
    float windowBeta_{8.0f};  // Kaiser window beta

    juce::SmoothedValue<float> hopSmoother_;     // juce linear ramp smoother
    juce::SmoothedValue<float> stretchSmoother_; // stretch ratio smoother
    simd::Float128 grainPhase_{};                // per-grain phase [0, 1), packed as 4 lanes (max 4 grains)

    SlewedParam dryDelaySlw_{0.0f, 0.000167f};
    SlewedParam mixSlw_{0.5f, 0.15f};

    // Stereo sinc delay line (owns its sinc table)
    ShortcircuitSincDelayLine<inst, SimdT> delayLine_;
};

//------------------------------------------------------------------------------
template <simd::Inst inst, class SimdT>
template <int N>
void DspImpl<inst, SimdT>::processGrains(float* left, float* right, int numSamples) {
    // Grains g >= N are inactive: read<N> returns zero for their lanes, so they
    // are automatically excluded from the wet sum below.
    for (int n = 0; n < numSamples; ++n) {
        // linear ramp step
        float const hopSamps = hopSmoother_.getNextValue();
        float const stretch = stretchSmoother_.getNextValue();
        float const grainLen = hopSamps * static_cast<float>(windowMul_);
        float const phaseInc = 1.0f / grainLen; // fixed window-envelope rate

        // write input (stereo)
        delayLine_.write(left[n], right[n]);

        // wet signal: all grains advance in lockstep with the same phaseInc, so pack
        // the per-grain phases into one 4-lane vector and process them together.
        simd::Float128 const phase = grainPhase_;

        // Kaiser window — SIMD overload, bit-identical per lane to the scalar version
        simd::Float128 const win = windowFn_.value(phase);

        // read delay = 2 * (stretch * phase) * grainLen, samples behind the write head
        simd::Float128 const readDelay = simd::BroadcastF128(2.0f) * (stretch * phase) * simd::BroadcastF128(grainLen);

        // wet reads: the templated read<N> only reads the first N lanes and leaves
        // lanes >= N at zero, so the vector reduce below stays valid for any N.
        simd::Float128 gl{};
        simd::Float128 gr{};
        delayLine_.template read<N>(readDelay, gl, gr);
        float const sumL = simd::ReduceAdd(win * gl);
        float const sumR = simd::ReduceAdd(win * gr);

        // advance phases (same increment for all grains)
        grainPhase_ = simd::Frac(phase + simd::BroadcastF128(phaseInc));

        // mix dry signal from delay (single delay -> read<1>)
        float const dryDelaySamps = grainLen * 1.5f * dryDelaySlw_.value;
        simd::Float128 dryL, dryR;
        delayLine_.template read<1>(simd::Float128{dryDelaySamps, 0.0f, 0.0f, 0.0f}, dryL, dryR);
        left[n] = interp(dryL[0], sumL, mixSlw_.value);
        right[n] = interp(dryR[0], sumR, mixSlw_.value);

        dryDelaySlw_.step();
    }

    mixSlw_.step();
}

} // namespace sttr
