#pragma once
#include <numbers>
#include <cmath>

namespace qwqdsp::oscillor {
class PolySineOsc {
public:
    void Reset() noexcept {
        phase_ = 0;
    }

    void Reset(float phase) noexcept {
        phase_ = phase;
    }

    void SetW(float w) noexcept {
        phase_inc_ = w / (std::numbers::pi_v<float> * 2);
    }

    void Tick() noexcept {
        float t;
        phase_ += phase_inc_;
        phase_ = std::modf(phase_, &t);
    }

    float Sine() const noexcept {
        float x = 2 * phase_ - 1;
        x *= std::numbers::pi_v<float>;
        auto x2 = x * x;
        auto numerator = -x * (static_cast<float>(-11511339840) +
                            x2 * (static_cast<float>(1640635920) + x2 * (static_cast<float>(-52785432) + x2 * static_cast<float>(479249))));
        auto denominator =
            static_cast<float>(11511339840) + x2 * (static_cast<float>(277920720) + x2 * (static_cast<float>(3177720) + x2 * static_cast<float>(18361)));
        return numerator / denominator;
    }

    float Cosine() const noexcept {
        float x = 2 * phase_ - 1;
        x *= std::numbers::pi_v<float>;
        auto x2 = x * x;
        auto numerator = -(static_cast<float>(-39251520) + x2 * (static_cast<float>(18471600) + x2 * (static_cast<float>(-1075032) + static_cast<float>(14615) * x2)));
        auto denominator = static_cast<float>(39251520) + x2 * (static_cast<float>(1154160) + x2 * (static_cast<float>(16632) + x2 * static_cast<float>(127)));
        return numerator / denominator;
    }
private:
    float phase_;
    float phase_inc_;
};
}