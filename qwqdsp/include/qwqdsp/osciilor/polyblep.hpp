#pragma once
#include <cmath>

namespace qwqdsp::oscillor {
/**
 * @ref https://ccrma.stanford.edu/~juhan/vas.html
 * @ref https://www.researchgate.net/publication/307990687_Rounding_Corners_with_BLAMP
 */
template<class T = float, bool kUseOrder4 = true>
class PolyBlep {
public:
    void SetFreq(T f, T fs) noexcept {
        phase_inc_ = f / fs;
    }

    void SetPWM(T width) noexcept {
        pwm_ = width;
    }

    T Sawtooth() noexcept {
        phase_ += phase_inc_;
        phase_ -= std::floor(phase_);
        T const t = phase_ * 2 - 1;
        return t - 2 * Blep(phase_, phase_inc_);
    }

    T Sqaure() noexcept {
        phase_ += phase_inc_;
        phase_ -= std::floor(phase_);
        T const t = phase_ < static_cast<T>(0.5) ? 1 : -1;
        T const phase2 = phase_ + static_cast<T>(0.5);
        T const phase2_wrap = phase2 - std::floor(phase2);
        return t + 2 * (Blep(phase_, phase_inc_) - Blep(phase2_wrap, phase_inc_));
    }

    T PWM() noexcept {
        phase_ += phase_inc_;
        phase_ -= std::floor(phase_);
        T const t = phase_ < pwm_ ? 1 : -1;
        T const phase2 = phase_ + 1 - pwm_;
        T const phase2_wrap = phase2 - std::floor(phase2);
        return t + 2 * (Blep(phase_, phase_inc_) - Blep(phase2_wrap, phase_inc_));
    }

    T Triangle() noexcept {
        phase_ += phase_inc_;
        phase_ -= std::floor(phase_);
        T t = 0;
        if (phase_ < static_cast<T>(0.5)) {
            t = 1 - 4 * phase_;
        }
        else {
            t = 4 * phase_ - 3;
        }
        T const phase2 = phase_ + static_cast<T>(0.5);
        T const phase2_wrap = phase2 - std::floor(phase2);
        return t + 8 * phase_inc_ * (-Blamp(phase_, phase_inc_) + Blamp(phase2_wrap, phase_inc_));
    }
private:
    static constexpr T x2(T x) noexcept {
        return x * x;
    }
    static constexpr T x3(T x) noexcept {
        return x2(x) * x;
    }
    static constexpr T x4(T x) noexcept {
        return x2(x) * x2(x);
    }
    static constexpr T x5(T x) noexcept {
        return x2(x) * x3(x);
    }

    static constexpr T Blep(T t, T dt) noexcept {
        if constexpr (kUseOrder4) {
            if (t < dt) {
                // 0~1
                T const x = t / dt;
                return x4(x) / 8 - x3(x) / 3 + 2 * x / 3 - static_cast<T>(0.5);
            }
            else if (t < 2 * dt) {
                // 1 ~ 2
                T const x = t / dt - 1;
                return -x4(x) / 24 + x3(x) / 6 - x2(x) / 4 + x / 6 - static_cast<T>(1.0 / 24);
            }
            else if (t > 1 - dt) {
                // -1 ~ 0
                T const x = (t - 1) / dt + 1;
                return -x4(x) / 8 + x3(x) / 6 + x2(x) / 4 + x / 6 + static_cast<T>(1.0 / 24);
            }
            else if (t > 1 - dt * 2) {
                // -2 ~ -1
                T const x = (t - 1) / dt + 2;
                return x4(x) / 24;
            }
            else {
                return 0;
            }
        }
        else {
            if (t < dt) {
                // 0 ~ 1
                T const x = t / dt;
                return -x2(x - 1) / 2;
            }
            else if (t > 1 - dt) {
                // -1 ~ 0
                T const x = (t - 1) / dt;
                return x2(x + 1) / 2;
            }
            else {
                return 0;
            }
        }
    }

    static constexpr T Blamp(T t, T dt) noexcept {
        if constexpr (kUseOrder4) {
            if (t < dt) {
                // 0~1
                T const x = t / dt;
                return x5(x) / 40 - x4(x) / 12 + x2(x) / 3 - x / 2 + static_cast<T>(7.0 / 30);
            }
            else if (t < 2 * dt) {
                // 1 ~ 2
                T const x = t / dt - 1;
                return -x5(x) / 120 + x4(x) / 24 - x3(x) / 12 + x2(x) / 12 - x / 24 + static_cast<T>(1.0 / 120);
            }
            else if (t > 1 - dt) {
                // -1 ~ 0
                T const x = (t - 1) / dt + 1;
                return -x5(x) / 40 + x4(x) / 24 + x3(x) / 12 + x2(x) / 12 + x / 24 + static_cast<T>(1.0 / 120);
            }
            else if (t > 1 - dt * 2) {
                // -2 ~ -1
                T const x = (t - 1) / dt + 2;
                return x5(x) / 120;
            }
            else {
                return 0;
            }
        }
        else {
            if (t < dt) {
                // 0 ~ 1
                T const x = t / dt;
                return -x3(x - 1) / 6;
            }
            else if (t > 1 - dt) {
                // -1 ~ 0
                T const x = (t - 1) / dt;
                return x3(x + 1) / 6;
            }
            else {
                return 0;
            }
        }
    }

    T phase_{};
    T phase_inc_{};
    T pwm_{};
};
}