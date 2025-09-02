#pragma once
#include <cmath>
#include <numbers>

namespace qwqdsp::oscillor {
class RawOscillor {
public:
    void Init(float fs) noexcept {
        fs_ = fs;
    }

    void SetFreq(float freq) noexcept {
        inc_ = freq / fs_;
    }

    float Tick() noexcept {
        phase_ += inc_;
        phase_ -= static_cast<int>(phase_);
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
    float fs_{};
    float inc_{};
    float phase_{};
};
}