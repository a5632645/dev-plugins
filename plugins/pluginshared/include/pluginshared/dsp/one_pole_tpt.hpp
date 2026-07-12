#pragma once
#include <array>
#include <cmath>
#include <numbers>
#include "../simd.hpp"


namespace pluginshared::dsp {

template <simd::IsSimdFloat T>
class OnePoleTPT {
public:
    void Reset() noexcept {
        lag_ = T{};
    }

    /**
     * @brief 计算系数
     *
     * @param w 任何值，如果小于0，系数为0，如果大于pi，系数为1
     * @return 系数
     * @note 实际上，w的值域是[0, pi)，但这里也可以接受其他值，
     *        只是会被自动处理成特殊值
     */
    static float ComputeCoeff(float w) noexcept {
        constexpr float kMaxOmega = std::numbers::pi_v<float> - 1e-5f;
        [[unlikely]]
        if (w < 0.0f) {
            return 0.0f;
        }
        else if (w > kMaxOmega) {
            return 1.0f;
        }
        else [[likely]] {
            auto k = std::tan(w / 2);
            return k / (1 + k);
        }
    }

    /**
     * @brief 计算系数
     *
     * @param w 数字角频率，任意值，过低过高会自动处理特殊值
     * @return PackFloat<N> 系数
     */
    static T ComputeCoeffs(T w) noexcept {
        T r;
        for (size_t i = 0; i < simd::LaneSize<T>; ++i) {
            r[i] = ComputeCoeff(w[i]);
        }
        return r;
    }

    T TickLowpass(T x, T coeff) noexcept {
        T lag = lag_;
        auto delta = coeff * (x - lag);
        lag += delta;
        auto y = lag;
        lag += delta;
        lag_ = lag;
        return y;
    }

    T TickHighpass(T x, T coeff) noexcept {
        T lag = lag_;
        auto delta = coeff * (x - lag);
        lag += delta;
        auto y = lag;
        lag += delta;
        lag_ = lag;
        return x - y;
    }

    T TickHighshelf(T x, T coeff, T gain) noexcept {
        T lp = TickLowpass(x, coeff);
        return lp + gain * (x - lp);
    }

    T TickAllpass(T x, T coeff) noexcept {
        T lp = TickLowpass(x, coeff);
        lp += lp;
        return lp - x;
    }
private:
    T lag_{};
};

} // namespace pluginshared::dsp
