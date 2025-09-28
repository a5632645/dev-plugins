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
    static constexpr float kRangeMultiply = kUseOrder4 ? 2.0f : 1.0f;

    void SetFreq(T f, T fs) noexcept {
        phase_inc_ = f / fs;
    }

    void SetPWM(T width) noexcept {
        pwm_ = width;
    }

    void SetHardSync(T sync_ratio) noexcept {
        sync_ = sync_ratio;
    }

    T Sawtooth() noexcept {
        phase_ += phase_inc_;
        phase_ -= std::floor(phase_);
        T const t = phase_ * 2 - 1;
        return t - 2 * Blep(phase_, phase_inc_);
    }

    // 在N+0.1左右会有一些混叠
    T SawtoothSync() noexcept {
        phase_ += phase_inc_;
        phase_ -= std::floor(phase_);
        T phase2 = phase_ * sync_;
        T const t = phase2 - std::floor(phase2);
        T const dt = sync_ * phase_inc_;
        T frac_sync = sync_ - std::floor(sync_);
        if (frac_sync == static_cast<T>(0.0)) frac_sync = 1;
        T blep = 0;
        // 最右侧
        if (phase2 > std::floor(sync_) && phase2 > sync_ - kRangeMultiply * dt) {
            blep = frac_sync * BlepSync(t, dt, frac_sync);
        }
        // 最左侧
        else if (phase2 < kRangeMultiply * dt) {
            blep = Blep(t, dt) * frac_sync;
        }
        else {
            blep = Blep(t, dt);
        }
        return (t - blep) * 2 - 1;
    }

    T Sqaure() noexcept {
        phase_ += phase_inc_;
        phase_ -= std::floor(phase_);
        T const t = phase_ < static_cast<T>(0.5) ? 1 : -1;
        T const blep = Blep(phase_, phase_inc_) - BlepOffset(phase_, phase_inc_, 0.5f);
        return t + 2 * blep;
    }

    // 在N+0.3左右会有一些混叠
    T SqaureSync() noexcept {
        phase_ += phase_inc_;
        phase_ -= std::floor(phase_);
        
        T const frac_sync = sync_ - std::floor(sync_);
        T const phase_sync = phase_ * sync_;
        T const frac_phase_sync = phase_sync - std::floor(phase_sync);
        T const native = frac_phase_sync < 0.5f ? 1 : -1;
        T const dt = sync_ * phase_inc_;
        T blep = 0;
        if (frac_sync == 0.0f) {
            blep = Blep(frac_phase_sync, dt) - BlepOffset(frac_phase_sync, dt, 0.5f);
        }
        else if (frac_sync < 0.5f) {
            if (phase_sync < dt * kRangeMultiply) {
                blep = BlepOffset(phase_sync, dt, 0.5f);
            }
            else if (phase_sync > sync_ - dt * kRangeMultiply) {
                blep = Blep(frac_phase_sync, dt);
            }
            else {
                blep = Blep(frac_phase_sync, dt) - BlepOffset(frac_phase_sync, dt, 0.5f);
            }
        }
        else {
            if (phase_sync > std::floor(sync_)) {
                blep = BlepSync(frac_phase_sync, dt, frac_sync) - BlepOffset(frac_phase_sync, dt, 0.5f);
            }
            else {
                blep = Blep(frac_phase_sync, dt) - BlepOffset(frac_phase_sync, dt, 0.5f);
            }
        }

        return native + blep * 2;
    }

    T PWM() noexcept {
        phase_ += phase_inc_;
        phase_ -= std::floor(phase_);
        T const t = phase_ * 2 - 1;
        T const saw1 = t - 2 * Blep(phase_, phase_inc_);
        T const phase2 = phase_ + pwm_;
        T const phase2_wrap = phase2 - std::floor(phase2);
        T const t2 = phase2_wrap * 2 - 1;
        T const saw2 = t2 - 2 * Blep(phase2_wrap, phase_inc_);
        return saw1 - saw2;
    }

    // 在N+0.3左右会有一些混叠
    T PWMSync() noexcept {
        phase_ += phase_inc_;
        phase_ -= std::floor(phase_);
        
        T const frac_sync = sync_ - std::floor(sync_);
        T const phase_sync = phase_ * sync_;
        T const frac_phase_sync = phase_sync - std::floor(phase_sync);
        T const native = frac_phase_sync < pwm_ ? 1 : -1;
        T const dt = sync_ * phase_inc_;
        T blep = 0;
        if (frac_sync == 0.0f) {
            blep = Blep(frac_phase_sync, dt) - BlepOffset(frac_phase_sync, dt, pwm_);
        }
        else if (frac_sync < pwm_) {
            if (phase_sync < dt * kRangeMultiply) {
                blep = BlepOffset(phase_sync, dt, pwm_);
            }
            else if (phase_sync > sync_ - dt * kRangeMultiply) {
                blep = Blep(frac_phase_sync, dt);
            }
            else {
                blep = Blep(frac_phase_sync, dt) - BlepOffset(frac_phase_sync, dt, pwm_);
            }
        }
        else {
            if (phase_sync > std::floor(sync_)) {
                blep = BlepSync(frac_phase_sync, dt, frac_sync) - BlepOffset(frac_phase_sync, dt, pwm_);
            }
            else {
                blep = Blep(frac_phase_sync, dt) - BlepOffset(frac_phase_sync, dt, pwm_);
            }
        }

        return native + blep * 2;
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

    static T Frac(T x) noexcept {
        return x - std::floor(x);
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

    // 以offset为中心
    static constexpr T BlepOffset(T t, T dt, T offset) noexcept {
        if constexpr (kUseOrder4) {
            if (t - offset < -2 * dt) {
                return 0;
            }
            else if (t - offset < -dt) {
                // -2 ~ -1
                T const x = (t - offset) / dt + 2;
                return x4(x) / 24;
            }
            else if (t - offset < 0) {
                // -1 ~ 0
                T const x = (t - offset) / dt + 1;
                return -x4(x) / 8 + x3(x) / 6 + x2(x) / 4 + x / 6 + static_cast<T>(1.0 / 24);
            }
            else if (t - offset < dt) {
                // 0 ~ 1
                T const x = (t - offset) / dt;
                return x4(x) / 8 - x3(x) / 3 + 2 * x / 3 - static_cast<T>(0.5);
            }
            else if (t - offset < 2 * dt) {
                // 1 ~ 2
                T const x = (t - offset) / dt - 1;
                return -x4(x) / 24 + x3(x) / 6 - x2(x) / 4 + x / 6 - static_cast<T>(1.0 / 24);
            }
            else {
                return 0;
            }
        }
        else {
            if (t - offset < -dt) {
                return 0;
            }
            else if (t - offset < 0) {
                // -1 ~ 0
                T const x = (t - offset) / dt;
                return x2(x + 1) / 2;
            }
            else if (t - offset < dt) {
                // 0 ~ 1
                T const x = (t - offset) / dt;
                return -x2(x - 1) / 2;
            }
            else {
                return 0;
            }
        }
    }

    // 此Blep使用sync作为右边界
    static constexpr T BlepSync(T t, T dt, T sync) noexcept {
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
            else if (t > sync - dt) {
                // -1 ~ 0
                T const x = (t - sync) / dt + 1;
                return -x4(x) / 8 + x3(x) / 6 + x2(x) / 4 + x / 6 + static_cast<T>(1.0 / 24);
            }
            else if (t > sync - dt * 2) {
                // -2 ~ -1
                T const x = (t - sync) / dt + 2;
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
            else if (t > sync - dt) {
                // -1 ~ 0
                T const x = (t - sync) / dt;
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
    T sync_{};
};
}