#pragma once
#include <algorithm>
#include <cmath>
#include <numbers>
#include <complex>

namespace qwqdsp::oscillor {
namespace blep_coeff {
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

struct Triangle {
    static constexpr float kHalfLen = 1.0f;
    /**
     * @param x[0..kHalfLen]
     */
    static constexpr float GetHalf(float x) noexcept {
        float v = x - 1.0f;
        return -v * v * 0.5f;
    }

    /**
     * @param x[0..kHalfLen]
     */
    static constexpr float GetBlampHalf(float x) noexcept {
        float v = x - 1;
        return -v * v * v / 6;
    }
};

struct Hann {
    static constexpr float kHalfLen = 2.0f;
    static float GetHalf(float x) noexcept {
        constexpr float half_pi = std::numbers::pi_v<float> / 2;
        constexpr float inv_twopi = 1.0f / (std::numbers::pi_v<float> * 2);
        return x * 0.25f + std::sin(half_pi * x) * inv_twopi - 0.5f;
    }

    static float GetBlampHalf(float x) noexcept {
        constexpr float inv_pi2 = 1.0f / x2(std::numbers::pi_v<float>);
        constexpr float half_pi = std::numbers::pi_v<float> / 2;
        return (1.0f - x) / 2.0f + x2(x) / 8.0f - inv_pi2 * (1.0f + std::cos(half_pi * x));
    }
};

struct BSpline {
    static constexpr float kHalfLen = 2.0f;
    static constexpr float GetHalf(float x) noexcept {
        const float is_less_than_one_mask = x - 1.0f; 
        const float result_if_less = (x2(x) - 2.0f) * (6.0f - 8.0f * x + 3.0f * x2(x)) / 24.0f;
        const float result_if_greater = -x4(x - 2.0f) / 24.0f;
        return (is_less_than_one_mask < 0.0f) 
            ? result_if_less 
            : result_if_greater;
    }

    static constexpr float GetBlampHalf(float x) noexcept {
        const float is_less_than_one_mask = x - 1.0f; 
        const float result_if_less = (28.0f - 60.0f * x + 40.0f * x2(x) - 10.0f * x4(x) + 3.0f * x5(x)) / 120.0f;
        const float result_if_greater = -x5(x - 2.0f) / 120.0f;
        return (is_less_than_one_mask < 0.0f) 
            ? result_if_less 
            : result_if_greater;
    }
};

struct BlackmanNutall {
    static constexpr float kHalfLen = 3.0f;
    static float GetHalf(float x) noexcept {
        constexpr auto invpi = std::numbers::inv_pi_v<float>;
        constexpr auto pi = std::numbers::pi_v<float>;
        constexpr auto a1 = 0.166666666666667f;
        constexpr auto a2 = 0.672719819110907f * invpi;
        constexpr auto a3 = 0.0939262240502071f * invpi;
        constexpr auto a4 = 0.00487790142101867f * invpi;

        auto c = std::polar(1.0f, x * (pi / 3));
        auto r = c;

        float y = 0;
        y += a1 * x;
        y += a2 * r.imag();
        r *= c;
        y += a3 * r.imag();
        r *= c;
        y += a4 * r.imag();
        r *= c;
        y -= 0.5f;
        return y;
    }

    static float GetBlampHalf(float x) noexcept {
        const float is_less_than_one_mask = x - 1.0f; 
        const float result_if_less = (28.0f - 60.0f * x + 40.0f * x2(x) - 10.0f * x4(x) + 3.0f * x5(x)) / 120.0f;
        const float result_if_greater = -x5(x - 2.0f) / 120.0f;
        return (is_less_than_one_mask < 0.0f) 
            ? result_if_less 
            : result_if_greater;
    }
};

template<class T>
concept CBlepCoeff = requires (float x) {
    {T::GetHalf(x)} -> std::same_as<float>;
    {T::GetBlampHalf(x)} -> std::same_as<float>;
    T::kHalfLen;
};
}

/**
 * @ref https://ccrma.stanford.edu/~juhan/vas.html
 * @ref https://www.researchgate.net/publication/307990687_Rounding_Corners_with_BLAMP
 */
template<qwqdsp::oscillor::blep_coeff::CBlepCoeff TCoeff>
class PolyBlep {
public:
    static constexpr float kRangeMultiply = TCoeff::kHalfLen;

    void SetFreq(float f, float fs) noexcept {
        phase_inc_ = f / fs;
    }

    void SetPWM(float width) noexcept {
        pwm_ = width;
    }

    void SetHardSync(float sync_ratio) noexcept {
        sync_ = sync_ratio;
    }

    float Sawtooth() noexcept {
        phase_ += phase_inc_;
        phase_ -= std::floor(phase_);
        // -------------------- high quality --------------------
        float const t = phase_ * 2 - 1;
        float const blep = 2 * Blep(phase_, phase_inc_);
        return t - blep;
        // -------------------- faster --------------------
        // float dt = std::min(phase_inc_, 0.5f / TCoeff::kHalfLen);
        // float naive = Frac(phase_ + 0.5f);
        // float blep = BlepOffset(phase_, dt, 0.5f);
        // return 2 * (naive - blep) - 1;
    }

    // qwqfixme: still have artifacts in extreme sync parameter
    float SawtoothSync() noexcept {
        phase_ += phase_inc_;
        phase_ -= std::floor(phase_);

        float frac_sync = sync_ - std::floor(sync_);
        if (frac_sync == static_cast<float>(0.0)) frac_sync = 1;

        float phase2 = phase_ * sync_;
        float const t = phase2 - std::floor(phase2);
        float dt = sync_ * phase_inc_;
        dt = std::min(dt, 0.5f);

        float blep = 0;

        float left_amount = 0;
        if (phase2 > 1) {
            left_amount = 1;
        }
        else {
            left_amount = frac_sync;
        }

        float right_amount = 0;
        float main_amount = 0;
        if (phase2 > std::floor(sync_)) {
            right_amount = 0;
            main_amount = frac_sync;
        }
        else {
            right_amount = 1;
            main_amount = 0;
        }

        auto t1 = std::min(t / dt, TCoeff::kHalfLen);
        auto t2 = std::min((1.0f - t) / dt, TCoeff::kHalfLen);
        blep = left_amount * TCoeff::GetHalf(t1) - right_amount * TCoeff::GetHalf(t2);
        t2 = std::min((1.0f - phase_) / dt, TCoeff::kHalfLen);
        blep -= main_amount * TCoeff::GetHalf(t2);
        blep += TCoeff::GetHalf(std::min((phase2 + frac_sync) / dt, TCoeff::kHalfLen));

        return (t - blep) * 2 - 1;
    }

    float Sqaure() noexcept {
        phase_ += phase_inc_;
        phase_ -= std::floor(phase_);
        float const t = phase_ < static_cast<float>(0.5) ? 1 : -1;
        // -------------------- high quality --------------------
        // float dt = phase_inc_;
        // float const blep = Blep(phase_, phase_inc_)
        //     - BlepOffset(phase_, dt, 0.5f) 
        //     - BlepOffset(phase_, dt, 1.5f) 
        //     - BlepOffset(phase_, dt, -0.5f);
        // -------------------- fast --------------------
        float dt = std::min(phase_inc_, 0.5f / TCoeff::kHalfLen);
        float const blep = Blep(phase_, phase_inc_) - BlepOffset(phase_, dt, 0.5f);
        return t + 2 * blep;
    }

    // qwqfixme: still have artifacts in extreme sync parameter
    float SqaureSync() noexcept {
        phase_ += phase_inc_;
        phase_ -= std::floor(phase_);
        
        float const frac_sync = sync_ - std::floor(sync_);
        float const phase_sync = phase_ * sync_;
        float const frac_phase_sync = phase_sync - std::floor(phase_sync);
        float const native = frac_phase_sync < 0.5f ? 1 : -1;
        float const dt = sync_ * phase_inc_;
        float blep = 0;
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

    float PWM_NoDC() noexcept {
        phase_ += phase_inc_;
        phase_ -= std::floor(phase_);
        float const t = phase_ * 2 - 1;
        float const saw1 = t - 2 * Blep(phase_, phase_inc_);
        float const phase2 = phase_ + pwm_;
        float const phase2_wrap = phase2 - std::floor(phase2);
        float const t2 = phase2_wrap * 2 - 1;
        float const saw2 = t2 - 2 * Blep(phase2_wrap, phase_inc_);
        return saw1 - saw2;
    }

    /**
     * @note 如果是faster代码,该波形需要限制pwm在[phase_inc*blep_len,1-phase_inc*blep_len]之间
     */
    float PWM_Classic() noexcept {
        phase_ += phase_inc_;
        phase_ -= std::floor(phase_);
        // -------------------- faster --------------------
        // float const t = phase_ < static_cast<float>(pwm_) ? 1 : -1;
        // float const blep = Blep(phase_, phase_inc_) - BlepOffset(phase_, phase_inc_, pwm_);
        // -------------------- faster auto clamp --------------------
        float dt = std::min(phase_inc_, 0.5f / TCoeff::kHalfLen);
        auto pwm = std::clamp(pwm_, TCoeff::kHalfLen * dt, 1 - TCoeff::kHalfLen * dt);
        float const t = phase_ < static_cast<float>(pwm) ? 1 : -1;
        float const blep = Blep(phase_, phase_inc_) - BlepOffset(phase_, dt, pwm);
        // -------------------- high quality --------------------
        // float const t = phase_ < static_cast<float>(pwm_) ? 1 : -1;
        // float const blep = Blep(phase_, phase_inc_) 
        //     - BlepOffset(phase_, phase_inc_, pwm_) 
        //     - BlepOffset(phase_, phase_inc_, pwm_ + 1) 
        //     - BlepOffset(phase_, phase_inc_, pwm_ - 1);
        auto v = t + 2 * blep;
        return v;
    }

    // qwqfixme: still have artifacts in extreme sync parameter
    float PWMSync() noexcept {
        phase_ += phase_inc_;
        phase_ -= std::floor(phase_);
        
        float const frac_sync = sync_ - std::floor(sync_);
        float const phase_sync = phase_ * sync_;
        float const frac_phase_sync = phase_sync - std::floor(phase_sync);
        float const native = frac_phase_sync < pwm_ ? 1 : -1;
        float const dt = sync_ * phase_inc_;
        float blep = 0;
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

    // qwqfixme: triganle hardsync
    float Triangle() noexcept {
        phase_ += phase_inc_;
        phase_ -= std::floor(phase_);
        float t = 0;
        if (phase_ < static_cast<float>(0.5)) {
            t = 1 - 4 * phase_;
        }
        else {
            t = 4 * phase_ - 3;
        }
        float const phase2 = phase_ + static_cast<float>(0.5);
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

    static float Frac(float x) noexcept {
        return x - std::floor(x);
    }

    /**
     * @param t [0..1]
     * @param dt [0..1]
     */
    static constexpr float Blep(float t, float dt) noexcept {
        auto t1 = std::min(t / dt, TCoeff::kHalfLen);
        auto t2 = std::min((1.0f - t) / dt, TCoeff::kHalfLen);
        return TCoeff::GetHalf(t1) - TCoeff::GetHalf(t2);
    }

    /**
     * @brief 以offset为0的blep
     */
    static float BlepOffset(float t, float dt, float offset) noexcept {
        float x = (t - offset) / dt;
        x = std::clamp(x, -TCoeff::kHalfLen, TCoeff::kHalfLen);
        return -std::copysign(TCoeff::GetHalf(std::abs(x)), x);
    }

    // 此Blep使用sync作为右边界
    static constexpr float BlepSync(float t, float dt, float sync) noexcept {
        auto t1 = std::min(t / dt, TCoeff::kHalfLen);
        auto t2 = std::min((sync - t) / dt, TCoeff::kHalfLen);
        return TCoeff::GetHalf(t1) - TCoeff::GetHalf(t2);
        // if constexpr (std::is_same_v<TCoeff, qwqdsp::oscillor::blep_coeff::BSpline>) {
        //     if (t < dt) {
        //         // 0~1
        //         float const x = t / dt;
        //         return x4(x) / 8 - x3(x) / 3 + 2 * x / 3 - static_cast<float>(0.5);
        //     }
        //     else if (t < 2 * dt) {
        //         // 1 ~ 2
        //         float const x = t / dt - 1;
        //         return -x4(x) / 24 + x3(x) / 6 - x2(x) / 4 + x / 6 - static_cast<float>(1.0 / 24);
        //     }
        //     else if (t > sync - dt) {
        //         // -1 ~ 0
        //         float const x = (t - sync) / dt + 1;
        //         return -x4(x) / 8 + x3(x) / 6 + x2(x) / 4 + x / 6 + static_cast<float>(1.0 / 24);
        //     }
        //     else if (t > sync - dt * 2) {
        //         // -2 ~ -1
        //         float const x = (t - sync) / dt + 2;
        //         return x4(x) / 24;
        //     }
        //     else {
        //         return 0;
        //     }
        // }
        // else if (std::is_same_v<TCoeff, qwqdsp::oscillor::blep_coeff::Triangle>) {
        //     if (t < dt) {
        //         // 0 ~ 1
        //         float const x = t / dt;
        //         return -x2(x - 1) / 2;
        //     }
        //     else if (t > sync - dt) {
        //         // -1 ~ 0
        //         float const x = (t - sync) / dt;
        //         return x2(x + 1) / 2;
        //     }
        //     else {
        //         return 0;
        //     }
        // }
        // else {
        //     static_assert((std::is_same_v<TCoeff, qwqdsp::oscillor::blep_coeff::Triangle>
        //         || std::is_same_v<TCoeff, qwqdsp::oscillor::blep_coeff::BSpline>), "unsupport blep type");
        //     return 0;
        // }
    }

    static constexpr float Blamp(float t, float dt) noexcept {
        auto t1 = std::min(t / dt, TCoeff::kHalfLen);
        auto t2 = std::min((1.0f - t) / dt, TCoeff::kHalfLen);
        return TCoeff::GetBlampHalf(t1) + TCoeff::GetBlampHalf(t2);
    }

    float phase_{};
    float phase_inc_{0.00001f};
    float pwm_{};
    float sync_{1.0f};
};
}