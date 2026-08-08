#pragma once
#include <algorithm>
#include <cmath>

#include <juce_audio_basics/juce_audio_basics.h>

#include "idsp.hpp"
#include "pluginshared/simd/simd.hpp"
#include "shortcircuit_sinc_delayline.hpp"

namespace sttr {

//==============================================================================
/** Kaiser window function — evaluates at a normalised position t in [0, 1].

    The Kaiser window is controlled by a single beta parameter; the window
    length is controlled separately by DspImpl::windowMul.
 */
class Window {
public:
    Window() = default;

    void setBeta(float beta) noexcept {
        beta_ = beta;
        invI0Beta_ = 1.0f / besselI0(beta_);
    }

    float beta() const noexcept {
        return beta_;
    }

    /** Kaiser window value at normalised position t in [0, 1]. */
    float value(float t) const noexcept {
        // w(t) = I0(beta * sqrt(1 - (2t - 1)^2)) / I0(beta)
        float const arg = 1.0f - (2.0f * t - 1.0f) * (2.0f * t - 1.0f);
        if (arg <= 0.0f)
            return 1.0f;
        return besselI0(beta_ * std::sqrt(arg)) * invI0Beta_;
    }

    /** Kaiser window values at 4 normalised positions t in [0, 1]. */
    simd::Float128 value(simd::Float128 t) const noexcept {
        simd::Float128 const one  = simd::BroadcastF128(1.0f);
        simd::Float128 const two  = simd::BroadcastF128(2.0f);
        simd::Float128 const zero = simd::Float128{};

        // arg = 1 - (2t - 1)^2; clamp to >= 0 so sqrt stays finite
        simd::Float128 const arg = one - (two * t - one) * (two * t - one);
        simd::Float128 const computed =
            besselI0(beta_ * simd::Sqrt(simd::Max(arg, zero))) * simd::BroadcastF128(invI0Beta_);

        // arg <= 0 (outside [0, 1]) -> 1.0, otherwise computed
        simd::Float128 const maskf = simd::ToFloat(arg > zero); // 0.0 or -1.0 per lane
        return computed * (-maskf) + one * (maskf + one);
    }

private:
    // Padé (8,8) coefficients of exp(-x)*I0(x), ascending powers of x
    static constexpr float kNum[] = {
        1.0f,                    // x^0
        0.105679f,               // x^1
        0.232432f,               // x^2
        0.022871f,               // x^3
        0.0113535f,              // x^4
        0.000831911f,            // x^5
        0.000176505f,            // x^6
        7.559388122773327e-6f,   // x^7
        2.3196902640592364e-8f,  // x^8
    };
    static constexpr float kDen[] = {
        1.0f,                     // x^0
        1.1057f,                  // x^1
        0.588013f,                // x^2
        0.198596f,                // x^3
        0.0468095f,               // x^4
        0.00842545f,              // x^5
        0.000960233f,             // x^6
        0.000120437f,             // x^7
        1.612947555525283e-6f,    // x^8
    };

    /** Zeroth-order modified Bessel function of the first kind (scalar).
        Padé (8,8) rational approximation of exp(-x) * I0(x), valid for
        x in [0, 16] (max beta = 16, and beta * sqrt(1 - (2t-1)^2) <= beta);
        the result is multiplied by exp(x) to recover I0(x). */
    static float besselI0(float x) noexcept {
        // Horner evaluation of the Padé approximant of exp(-x) * I0(x)
        float num = kNum[8];
        float den = kDen[8];
        for (int i = 7; i >= 0; --i) {
            num = num * x + kNum[i];
            den = den * x + kDen[i];
        }
        return std::exp(x) * (num / den);
    }

    /** Zeroth-order modified Bessel function of the first kind (4-lane SIMD). */
    static simd::Float128 besselI0(simd::Float128 x) noexcept {
        simd::Float128 num = simd::BroadcastF128(kNum[8]);
        simd::Float128 den = simd::BroadcastF128(kDen[8]);
        for (int i = 7; i >= 0; --i) {
            num = num * x + simd::BroadcastF128(kNum[i]);
            den = den * x + simd::BroadcastF128(kDen[i]);
        }
        return expF(x) * (num / den);
    }

    static simd::Float128 expF(simd::Float128 x) noexcept {
        return simd::Float128{std::exp(x[0]), std::exp(x[1]), std::exp(x[2]), std::exp(x[3])};
    }

    float beta_{8.0f};
    float invI0Beta_{1.0f / besselI0(8.0f)};
};

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

        lfoPhase_ = 0.0f;

        // Initialise per-grain phases (staggered evenly)
        int const n = numGrains_ > 0 ? numGrains_ : 2;
        simd::Float128 init{};
        for (int g = 0; g < kMaxGrains; ++g)
            init[g] = (n > 1) ? static_cast<float>(g % n) / static_cast<float>(n) : 0.0f;
        grainPhase_ = init;
    }

    /** Set all parameters atomically: copies into internal state, pulls
        smoother targets and re-syncs the grain count. Only needs to be
        called when a parameter changes. */
    void SetParameters(SttrParam const& params) override {
        mix_ = params.mix;
        hopMs_ = params.hopMs;
        dryDelay_ = params.dryDelay;
        stretch_ = params.stretch;

        windowMul_ = params.windowMul;
        windowBeta_ = params.windowBeta;

        // SetParameters() is only called when a parameter actually changes, so
        // pull the smoother targets and re-sync the grain count here.
        pullTargets();
        syncGrains();
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

    // template dispatch
    template <int N>
    void processGrains(float* left, float* right, int numSamples);

    void pullTargets();
    void syncGrains();

    // internal state
    float sampleRate_{44100.0f};
    int numGrains_{2};   // derived from windowMul_
    Window windowFn_;    // current Kaiser window

    float mix_{0.5f};
    float hopMs_{16.0f};
    float dryDelay_{0.0f};
    float stretch_{1.0f};
    int windowMul_{2};        // grain length multiplier, integer [1, 4]
    float windowBeta_{8.0f};  // Kaiser window beta

    juce::SmoothedValue<float> hopSmoother_;     // juce linear ramp smoother
    juce::SmoothedValue<float> stretchSmoother_; // stretch ratio smoother
    simd::Float128 grainPhase_{};                // per-grain phase [0, 1) packed as 4 lanes

    SlewedParam dryDelaySlw_{0.0f, 0.000167f};
    SlewedParam mixSlw_{0.5f, 0.15f};

    float lfoPhase_{};

    // Stereo sinc delay line (owns its sinc table)
    ShortcircuitSincDelayLine delayLine_;
};

//------------------------------------------------------------------------------
template <simd::Inst inst, class SimdT>
void DspImpl<inst, SimdT>::pullTargets() {
    mixSlw_.target = mix_;
    hopSmoother_.setTargetValue(std::max(millisecondsToSamples(hopMs_, sampleRate_), 1.0f));
    dryDelaySlw_.target = dryDelay_;
    // stretch ratio = 2^(formant/12), formant in [-10, 10] st
    stretchSmoother_.setTargetValue(std::clamp(stretch_, 0.561231f, 1.781797f));
}

template <simd::Inst inst, class SimdT>
void DspImpl<inst, SimdT>::syncGrains() {
    if (std::abs(windowFn_.beta() - windowBeta_) > 1.0e-6f) windowFn_.setBeta(windowBeta_);

    int const n = grainsForMul(windowMul_);
    if (n != numGrains_) {
        numGrains_ = n;
        Reset();
    }
}

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
        delayLine_.read<N>(readDelay, gl, gr);
        float const sumL = simd::ReduceAdd(win * gl);
        float const sumR = simd::ReduceAdd(win * gr);

        // advance phases (same increment for all grains)
        grainPhase_ = simd::Frac(phase + simd::BroadcastF128(phaseInc));

        // mix dry signal from delay (single delay -> read<1>)
        float const dryDelaySamps = grainLen * 1.5f * dryDelaySlw_.value;
        simd::Float128 dryL, dryR;
        delayLine_.read<1>(simd::Float128{dryDelaySamps, 0.0f, 0.0f, 0.0f}, dryL, dryR);
        left[n] = interp(dryL[0], sumL, mixSlw_.value);
        right[n] = interp(dryR[0], sumR, mixSlw_.value);

        dryDelaySlw_.step();
    }

    mixSlw_.step();
}

} // namespace sttr
