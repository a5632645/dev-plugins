#pragma once

#include <algorithm>
#include <cmath>

#include <juce_audio_basics/juce_audio_basics.h>

#include "pluginshared/simd/simd.hpp"
#include "shortcircuit_sinc_delayline.hpp"

//==============================================================================
/** Kaiser window function — evaluates at a normalised position t in [0, 1].

    The Kaiser window is controlled by a single beta parameter; the window
    length is controlled separately by SttrProcessor::windowMul.
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
        simd::Float128 const computed = besselI0(beta_ * sqrtF(simd::Max(arg, zero))) * simd::BroadcastF128(invI0Beta_);

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

    static simd::Float128 sqrtF(simd::Float128 x) noexcept {
        return simd::Float128{std::sqrt(x[0]), std::sqrt(x[1]), std::sqrt(x[2]), std::sqrt(x[3])};
    }

    float beta_{8.0f};
    float invI0Beta_{1.0f / besselI0(8.0f)};
};

//==============================================================================
/** Delay-line based granular processor (STTR algorithm).

    Pure-DSP implementation of the STTR (Short time time reversal)
    effect — a granular delay with overlapping grains, dry/wet mix, and a
    Kaiser window.

    The window length (grain length = windowMul * hop) and the Kaiser beta are
    controlled by windowMul / windowBeta; the grain count is derived from
    windowMul.
 */
class SttrProcessor {
public:
    /** All parameters in one struct — set atomically via setParameters(). */
    struct Parameters {
        float mix{0.5f};
        float hopMs{16.0f};
        float dryDelay{0.0f};
        float stretch{1.0f};   // ratio = 2^(formant/12), formant in semitones
        int windowMul{2};        // grain length = windowMul * hop, integer [1, 4]
        float windowBeta{8.0f};  // Kaiser window beta
    };

    SttrProcessor() = default;

    /** Set all parameters atomically: copies into internal state, pulls
        smoother targets and re-syncs the grain count. Only needs to be
        called when a parameter changes. */
    void setParameters(Parameters const& params);

    /** Allocate stereo delay buffer and reset state. */
    void prepare(float sampleRate);

    /** Process one stereo audio block in-place. */
    void processBlock(float* left, float* right, int numSamples);

    /** Clear delay buffer and reset read/write positions. */
    void reset();
private:
    static constexpr float kMaxHopMs = 500.0f;
    static constexpr int kMaxGrains = 4;

    // Worst-case read depth: 2 * stretch_max(2^(10/12)) * hop_max * mul_max,
    // in milliseconds. The delay line is sized dynamically in prepare().
    static constexpr float kMaxDelayMs = 2.0f * 1.7818f * kMaxHopMs * static_cast<float>(kMaxGrains);

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
    simd::Float128 grainPhase_{};               // per-grain phase [0, 1) packed as 4 lanes, all share the same hop

    SlewedParam dryDelaySlw_{0.0f, 0.000167f};
    SlewedParam mixSlw_{0.5f, 0.15f};

    float lfoPhase_{};

    // Stereo sinc delay line (owns its sinc table)
    ShortcircuitSincDelayLine delayLine_;

    void pullTargets();
    void syncGrains();
};
