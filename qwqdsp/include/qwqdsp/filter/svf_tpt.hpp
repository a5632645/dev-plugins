#pragma once
#include <cmath>
#include <array>

namespace qwqdsp::filter {
class SvfTPT {
public:
    void Reset() noexcept {
        s1_ = 0;
        s2_ = 0;
    }

    /**
     * @param w [0, pi]
     * @param r2 <0:不稳定 =0:无阻尼 0~1:复共轭极点 >1:分裂成两个单极点
     */
    void SetCoeffSVF(float w, float r2) noexcept {
        g_ = std::tan(w / 2);
        R2_ = r2;
        g1_ = r2 + g_;
        d_ = 1 / (1 + r2 * g_ + g_ * g_);
    }

    void SetCoeffQ(float w, float Q) noexcept {
        SetCoeffSVF(w, 1 / Q);
    }

    /**
     * @return [hp, bp, lp]
     */
    std::array<float, 3> TickMultiMode(float x) noexcept {
        float hp = (x - g1_ * s1_ - s2_) * d_;
        float v1 = g_ * hp;
        float bp = v1 + s1_;
        float v2 = g_ * bp;
        float lp = v2 + s2_;
        s1_ = bp + v1;
        s2_ = lp + v2;
        return {hp, bp, lp};
    }

    /**
     * @return [bp, lp]
     */
    std::array<float, 2> TickBPLP(float x) noexcept {
        float bp = d_ * (g_ * (x - s2_) + s1_);
        float v1 = bp - s1_;
        float v2 = g_ * bp;
        float lp = v2 + s2_;
        s1_ = bp + v1;
        s2_ = lp + v2;
        return {bp, lp};
    }

    float TickBandpass(float x) noexcept {
        float bp = d_ * (g_ * (x - s2_) + s1_);
        float bp2 = bp + bp;
        float v22 = g_ * bp2;
        s1_ -= bp2;
        s2_ += v22;
        return bp;
    }

    float TickNormBandpass(float x) noexcept {
        return TickBandpass(x * R2_);
    }

    float TickNotch(float x) noexcept {
        return x - TickNormBandpass(x);
    }

    float TickAllpass(float x) noexcept {
        return x - 2 * TickNormBandpass(x);
    }
private:
    float R2_{};
    float g_{};
    float g1_{};
    float d_{};
    float s1_{};
    float s2_{};
};
}