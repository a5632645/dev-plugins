#pragma once
#include <cmath>
#include <numbers>

namespace qwqdsp::oscillor {
class RawOscillor {
public:
    void Reset(float phase) noexcept {
        phase_ = phase - std::floor(phase);
    }

    void SetFreq(float freq, float fs) noexcept {
        inc_ = freq / fs;
    }

    float Tick() noexcept {
        phase_ += inc_;
        phase_ -= std::floor(phase_);
        return phase_;
    }

    float Saw() noexcept {
        return Tick() * 2.0f - 1.0f;
    }

    float Sine() noexcept {
        return std::sin(Tick() * std::numbers::pi_v<float> * 2.0f);
    }

    float Cosine() noexcept {
        return std::cos(Tick() * std::numbers::pi_v<float> * 2.0f);
    }

    float Triangle() noexcept {
        return 4.0f * std::abs(Tick() - 0.5f) - 1.0f;
    }

    float Square() noexcept {
        return Tick() > 0.5f ? 1.0f : -1.0f;
    }
private:
    float inc_{};
    float phase_{};
};
}