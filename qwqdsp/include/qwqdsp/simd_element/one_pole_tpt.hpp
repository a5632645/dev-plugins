#pragma once
#include <numbers>
#include <cmath>
#include "simd_pack.hpp"

namespace qwqdsp_simd_element {

template<size_t N>
class OnePoleTPT {
public:
    void Reset() noexcept {
        lag_.Broadcast(0);
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
    static PackFloat<N> ComputeCoeffs(PackFloat<N> const& w) noexcept {
        PackFloat<N> r;
        for (size_t i = 0; i < N; ++i) {
            r.x[i] = ComputeCoeff(w.x[i]);
        }
        return r;
    }


    PackFloat<N> TickLowpass(PackFloat<N> const& x, PackFloat<N> const& coeff) noexcept {
        auto delta = coeff * (x - lag_);
        lag_ += delta;
        auto y = lag_;
        lag_ += delta;
        return y;
    }

    PackFloat<N> TickHighpass(PackFloat<N> const& x, PackFloat<N> const& coeff) noexcept {
        auto delta = coeff * (x - lag_);
        lag_ += delta;
        auto y = lag_;
        lag_ += delta;
        return x - y;
    }

    PackFloat<N> TickHighshelf(PackFloat<N> const& x, PackFloat<N> const& coeff, PackFloat<N> const& gain) noexcept {
        PackFloat<N> lp = TickLowpass(x, coeff);
        return lp + gain * (x - lp);
    }

    PackFloat<N> TickAllpass(PackFloat<N> const& x, PackFloat<N> const& coeff) noexcept {
        PackFloat<N> lp = TickLowpass(x, coeff);
        lp += lp;
        return lp - x;
    }
private:
    PackFloat<N> lag_{};
};

} // namespace qwqdsp_simd_element
