#pragma once
#include <cmath>

namespace qwqdsp::oscillor {
/**
 * @ref https://ccrma.stanford.edu/~juhan/vas.html
 * @ref https://www.researchgate.net/publication/307990687_Rounding_Corners_with_BLAMP
 */
class PolyBlep {
public:
    static constexpr bool kUseOrder4 = true;

    void SetFreq(float f, float fs) noexcept {
        phase_inc_ = f / fs;
    }

    void SetPWM(float width) noexcept {
        pwm_ = width;
    }

    float Sawtooth() noexcept {
        phase_ += phase_inc_;
        phase_ -= std::floor(phase_);
        float const t = phase_ * 2 - 1;
        return t - 2 * Blep(phase_, phase_inc_);
    }

    float Sqaure() noexcept {
        phase_ += phase_inc_;
        phase_ -= std::floor(phase_);
        float const t = phase_ < 0.5f ? 1.0f : -1.0f;
        float const phase2 = phase_ + 0.5f;
        float const phase2_wrap = phase2 - std::floor(phase2);
        return t + 2 * (Blep(phase_, phase_inc_) - Blep(phase2_wrap, phase_inc_));
    }

    float PWM() noexcept {
        phase_ += phase_inc_;
        phase_ -= std::floor(phase_);
        float const t = phase_ < pwm_ ? 1.0f : -1.0f;
        float const phase2 = phase_ + 1 - pwm_;
        float const phase2_wrap = phase2 - std::floor(phase2);
        return t + 2 * (Blep(phase_, phase_inc_) - Blep(phase2_wrap, phase_inc_));
    }

    float Triangle() noexcept {
        phase_ += phase_inc_;
        phase_ -= std::floor(phase_);
        float t = 0;
        if (phase_ < 0.5f) {
            t = 1 - 4 * phase_;
        }
        else {
            t = 4 * phase_ - 3;
        }
        float const phase2 = phase_ + 0.5f;
        float const phase2_wrap = phase2 - std::floor(phase2);
        return t + 8 * phase_inc_ * (-Blamp(phase_, phase_inc_) + Blamp(phase2_wrap, phase_inc_));
    }
private:
    static constexpr float x2(float x) noexcept {
        return x * x;
    }
    static constexpr float x3(float x) noexcept {
        return x2(x) * x;
    }
    static constexpr float x4(float x) noexcept {
        return x2(x) * x2(x);
    }
    static constexpr float x5(float x) noexcept {
        return x2(x) * x3(x);
    }

    static constexpr float Blep(float t, float dt) noexcept {
        // qwqfixme: 应该乘2
        if constexpr (kUseOrder4) {
            if (t < dt) {
                // 0~1
                float const x = t / dt;
                return x4(x) / 8 - x3(x) / 3 + 2 * x / 3 - 1.0f / 2;
            }
            else if (t < 2 * dt) {
                // 1 ~ 2
                float const x = t / dt - 1;
                return -x4(x) / 24 + x3(x) / 6 - x2(x) / 4 + x / 6 - 1.0f / 24;
            }
            else if (t > 1 - dt) {
                // -1 ~ 0
                float const x = (t - 1) / dt + 1;
                return -x4(x) / 8 + x3(x) / 6 + x2(x) / 4 + x / 6 + 1.0f / 24;
            }
            else if (t > 1 - dt * 2) {
                // -2 ~ -1
                float const x = (t - 1) / dt + 2;
                return x4(x) / 24;
            }
            else {
                return 0;
            }
        }
        else {
            if (t < dt) {
                // 0 ~ 1
                float const x = t / dt;
                return -x2(x - 1) / 2;
            }
            else if (t > 1 - dt) {
                // -1 ~ 0
                float const x = (t - 1) / dt;
                return x2(x + 1) / 2;
            }
            else {
                return 0;
            }
        }
    }

    static constexpr float Blamp(float t, float dt) noexcept {
        if constexpr (kUseOrder4) {
            if (t < dt) {
                // 0~1
                float const x = t / dt;
                return x5(x) / 40 - x4(x) / 12 + x2(x) / 3 - x / 2 + 7.0f / 30;
            }
            else if (t < 2 * dt) {
                // 1 ~ 2
                float const x = t / dt - 1;
                return -x5(x) / 120 + x4(x) / 24 - x3(x) / 12 + x2(x) / 12 - x / 24 + 1.0f / 120;
            }
            else if (t > 1 - dt) {
                // -1 ~ 0
                float const x = (t - 1) / dt + 1;
                return -x5(x) / 40 + x4(x) / 24 + x3(x) / 12 + x2(x) / 12 + x / 24 + 1.0f / 120;
            }
            else if (t > 1 - dt * 2) {
                // -2 ~ -1
                float const x = (t - 1) / dt + 2;
                return x5(x) / 120;
            }
            else {
                return 0;
            }
        }
        else {
            if (t < dt) {
                // 0 ~ 1
                float const x = t / dt;
                return -x3(x - 1) / 6;
            }
            else if (t > 1 - dt) {
                // -1 ~ 0
                float const x = (t - 1) / dt;
                return x3(x + 1) / 6;
            }
            else {
                return 0;
            }
        }
    }

    float phase_{};
    float phase_inc_{};
    float pwm_{};
};
}