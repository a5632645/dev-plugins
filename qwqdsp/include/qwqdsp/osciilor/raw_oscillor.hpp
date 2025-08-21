#pragma once
#include <cmath>
#include <numbers>

namespace qwqdsp::oscillor {
class RawOscillor {
public:
    void Init(float fs) {
        fs_ = fs;
    }

    void SetFreq(float freq) {
        inc_ = freq / fs_;
    }

    float Tick() {
        phase_ += inc_;
        phase_ -= static_cast<int>(phase_);
        return phase_;
    }

    float Saw() {
        return Tick() * 2.0f - 1.0f;
    }

    float Sine() {
        return std::sin(Tick() * std::numbers::pi_v<float> * 2.0f);
    }

    float Cosine() {
        return std::cos(Tick() * std::numbers::pi_v<float> * 2.0f);
    }

    float Triangle() {
        return 4.0f * std::abs(Tick() - 0.5f) - 1.0f;
    }

    float Square() {
        return Tick() > 0.5f ? 1.0f : -1.0f;
    }
private:
    float fs_{};
    float inc_{};
    float phase_{};
};
}