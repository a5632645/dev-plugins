#pragma once
#include "simd_pack.hpp"

namespace qwqdsp_simd_element {
template<size_t N>
class AlgebraicWaveshaper {
public:
    static inline PackFloat<N> Naive(PackFloatCRef<N> x) noexcept {
        return x / PackOps::Sqrt(1 + x*x);
    }

    /**
     * @note latency = 0.5 samples
     */
    PackFloat<N> ADAA(PackFloatCRef<N> x) noexcept {
        auto up = x + xn1_;
        auto F_x = PackOps::Sqrt(1 + x*x);
        auto down = F_x + F_xn1_;
        xn1_ = x;
        F_xn1_ = F_x;
        return up / down;
    }

    /**
     * @note latency = 1 samples
     */
    PackFloat<N> ADAA_MV(PackFloatCRef<N> x) noexcept {
        auto F12 = PackOps::Sqrt(1 + X2(0.5f * (x + xn1_)));
        auto F1 = PackOps::Sqrt(1 + X2(xn1_));
        auto F32 = F_xn2_;
        auto y = 0.25f * ((x + 3 * xn1_) / (F12 + F1) + (3 * xn1_ + xn2_) / (F1 + F32));
        xn2_ = xn1_;
        xn1_ = x;
        F_xn2_ = F12;
        return y;
    }

    /**
     * @note latency >= 1 samples
     */
    PackFloat<N> ADAA_MV_Compensation(PackFloatCRef<N> x) noexcept {
        constexpr auto kA = 0.171572875f;
        x = ADAA_MV(x);
        y_ = x + kA * (x - y_);
        return y_;
    }

    void Reset() noexcept {
        xn1_.Broadcast(0);
        xn2_.Broadcast(0);
        F_xn1_.Broadcast(1);
        F_xn2_.Broadcast(1);
        y_.Broadcast(0);
    }
private:
    static inline constexpr PackFloat<N> X2(PackFloatCRef<N> x) noexcept {
        return x * x;
    }

    PackFloat<N> xn1_{};
    PackFloat<N> F_xn1_{1};
    PackFloat<N> xn2_{};
    PackFloat<N> F_xn2_{1};
    PackFloat<N> y_{};
};
}
