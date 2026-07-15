#pragma once

#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

#include <juce_audio_basics/juce_audio_basics.h>
#include <qwqdsp/interpolation.hpp>

//==============================================================================
/** Window function — stores coefficients and evaluates at a normalised position.

    Grain count is tied to the window type (see numGrains()).
 */
class Window {
public:
    enum class Type {
        hann = 0,
        hamming,
        blackman,
        blackman_harris,
        nuttall,
        blackman_nuttall
    };

    Window() = default;

    void setType(Type type) noexcept {
        type_ = type;
        auto const& c = kCoeffs[static_cast<int>(type)];
        k0_ = c.k0;
        k1_ = c.k1;
        k2_ = c.k2;
        k3_ = c.k3;
    }

    /** Number of grains for the given window type. */
    static int numGrains(Type type) noexcept {
        return kNumGrains[static_cast<int>(type)];
    }

    Type type() const noexcept {
        return type_;
    }

    float value(float t) const noexcept {
        float const twopi = 2.0f * static_cast<float>(std::numbers::pi);
        return k0_ - k1_ * std::cos(1.0f * twopi * t) + k2_ * std::cos(2.0f * twopi * t)
             - k3_ * std::cos(3.0f * twopi * t);
    }
private:
    struct Coeffs {
        float k0, k1, k2, k3;
    };
    static constexpr Coeffs kCoeffs[] = {
        {0.5f,       0.5f,       0.0f,       0.0f      }, // hann
        {0.53836f,   0.46164f,   0.0f,       0.0f      }, // hamming
        {0.42659f,   0.496562f,  0.076849f,  0.0f      }, // blackman
        {0.35875f,   0.48829f,   0.14128f,   0.01168f  }, // blackman_harris
        {0.355768f,  0.487396f,  0.144232f,  0.012604f }, // nuttall
        {0.3635819f, 0.4891775f, 0.1365995f, 0.0106411f}, // blackman_nuttall
    };
    static constexpr int kNumGrains[] = {2, 2, 3, 4, 4, 4};

    Type type_{Type::hann};
    float k0_{}, k1_{}, k2_{}, k3_{};
};

//==============================================================================
/** Delay-line based granular processor (STTR algorithm).

    Pure-DSP implementation of the STTR (Short time time reversal)
    effect — a granular delay with overlapping grains, dry/wet mix, and
    window-function selection.

    The window type and grain count are linked — setParameters() updates both.
 */
class SttrProcessor {
public:
    /** All parameters in one struct — set atomically via setParameters(). */
    struct Parameters {
        float mix{0.5f};
        float hopMs{16.0f};
        float dryDelay{0.0f};
        float stretch{1.0f};
        Window::Type windowType{Window::Type::hann};
    };

    SttrProcessor() = default;

    /** Set all parameters atomically (copies into internal state). */
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

    // helpers
    static float millisecondsToSamples(float ms, float sr) {
        return ms / 1000.0f * sr;
    }

    static float wrap(float index, float length) {
        if (index < 0.0f) return index + length;
        if (index >= length) return index - length;
        return index;
    }

    static float interp(float a, float b, float d) {
        return a * (1.0f - d) + b * d;
    }

    static float interpSample(float* data, float pos, unsigned int wrapLen) {
        int i = static_cast<int>(pos);
        float d = pos - static_cast<float>(i);
        float y0 = data[static_cast<int>(wrap(static_cast<float>(i), static_cast<float>(wrapLen)))];
        float y1 = data[static_cast<int>(wrap(static_cast<float>(i + 1), static_cast<float>(wrapLen)))];
        float y2 = data[static_cast<int>(wrap(static_cast<float>(i + 2), static_cast<float>(wrapLen)))];
        float y3 = data[static_cast<int>(wrap(static_cast<float>(i + 3), static_cast<float>(wrapLen)))];
        return qwqdsp::Interpolation::Lagrange3rd(y0, y1, y2, y3, d);
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
    int numGrains_{2};
    int delayWriter_{};  // current write position in delay line
    float masterWPos_{}; // monotonically increasing write-sample counter
    Window windowFn_;    // current window coefficients

    float mix_{0.5f};
    float hopMs_{16.0f};
    float dryDelay_{0.0f};
    float stretch_{1.0f};
    Window::Type windowType_{Window::Type::hann};

    juce::SmoothedValue<float> hopSmoother_;     // juce linear ramp smoother
    juce::SmoothedValue<float> stretchSmoother_; // stretch ratio smoother
    float grainPhase_[kMaxGrains]{};             // per-grain phase [0, 1), all share same hop

    SlewedParam dryDelaySlw_{0.0f, 0.000167f};
    SlewedParam mixSlw_{0.5f, 0.15f};

    float lfoPhase_{};

    // Per-channel delay buffers
    std::vector<float> delayBuf_[2]; // stereo
    int delayCap_{};                 // samples per channel

    void pullTargets();
    void syncNumGrains();
};
