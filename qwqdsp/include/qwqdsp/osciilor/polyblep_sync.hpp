#pragma once
#include <algorithm>
#include <cmath>
#include <numbers>
#include <complex>

namespace qwqdsp::oscillor {
namespace blep_coeff2 {
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

// qwqfixme: 增强
template<qwqdsp::oscillor::blep_coeff2::CBlepCoeff TCoeff>
class PolyBlepSync {
public:
    static constexpr size_t kDelay = static_cast<size_t>(TCoeff::kHalfLen);
    static constexpr size_t kDelaySize = 16;
    static constexpr size_t kDelayMask = kDelaySize - 1;

    void SetFreq(float f, float fs) noexcept {
        phase_inc_ = f / fs;
    }

    void SetPWM(float width) noexcept {
        pwm_ = width;
    }

    float GetPhase() noexcept {
        return phase_;
    }

    float Sawtooth(bool reset, float sync_samples_before) noexcept {
        // float last_phase = phase_;
        // phase_ += phase_inc_;
        // if (phase_ > 1) {
        //     phase_ -= 1;
        //     float happen_before = phase_ / phase_inc_;
        //     if (reset) {
        //         if (happen_before > sync_samples_before) {
        //             AddBlep(wpos_, happen_before, -1.0f);
        //             AddBlep(wpos_, sync_samples_before, -phase_inc_ * (happen_before - sync_samples_before));
        //             phase_ = sync_samples_before * phase_inc_;
        //         }
        //         else {
        //             AddBlep(wpos_, sync_samples_before, -(last_phase + (1-sync_samples_before)*phase_inc_));
        //             phase_ = sync_samples_before * phase_inc_;
        //         }
        //     }
        //     else {
        //         AddBlep(wpos_, happen_before, -1.0f);
        //     }
        // }
        // else if (reset) {
        //     AddBlep(wpos_, sync_samples_before, -phase_);
        //     phase_ = sync_samples_before * phase_inc_;
        // }

        if (reset) {
            AddBlep(wpos_, sync_samples_before, -phase_);
            phase_ = sync_samples_before * phase_inc_;
        }
        if (phase_ > 1) {
            phase_ -= 1;
            float happen_before = phase_ / phase_inc_;
            AddBlep(wpos_, happen_before, -1);
        }
        buffer_[wpos_] += phase_;
        naive_buffer_[wpos_] = phase_;
        float out = buffer_[rpos_];
        buffer_[rpos_] = 0;
        rpos_ = (rpos_ + 1) & kDelayMask;
        wpos_ = (wpos_ + 1) & kDelayMask;
        phase_ += phase_inc_;
        return 2 * out - 1;
    }
private:
    /**
     * @brief 在buffer_idx前frac_samples处(采样点)插入一个scale大小的blep
     * @param buffer_idx 缓冲区绝对索引
     * @param frac_samples_before 分数采样点
     * @param scale blep大小
     */
    void AddBlep(size_t buffer_idx, float frac_samples_before, float scale) noexcept {
        size_t begin_idx = buffer_idx - kDelay;
        float x = frac_samples_before - kDelay;
        for (size_t i = 0; i < 2 * kDelay; ++i) {
            begin_idx &= kDelayMask;
            float t = std::clamp(x, -TCoeff::kHalfLen, TCoeff::kHalfLen);
            buffer_[begin_idx] -= scale * std::copysign(TCoeff::GetHalf(std::abs(t)), t);
            blep_buffer_[begin_idx] = -scale * std::copysign(TCoeff::GetHalf(std::abs(t)), t);
            ++begin_idx;
            x += 1.0f;
        }
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
    }

    static constexpr float Blamp(float t, float dt) noexcept {
        auto t1 = std::min(t / dt, TCoeff::kHalfLen);
        auto t2 = std::min((1.0f - t) / dt, TCoeff::kHalfLen);
        return TCoeff::GetBlampHalf(t1) + TCoeff::GetBlampHalf(t2);
    }

    float phase_{};
    float phase_inc_{0.011f};
    float pwm_{};

    float buffer_[kDelaySize]{};
    float naive_buffer_[kDelaySize]{};
    float blep_buffer_[kDelaySize]{};
    size_t wpos_{kDelay};
    size_t rpos_{};
};
}