#pragma once
#include <cmath>
#include <array>

namespace qwqdsp::filter {
class SvfTPTShelf {
public:
    void Reset() noexcept {
        s1_ = 0;
        s2_ = 0;
    }

    /**
     * @param w [0, pi]
     * @param r2 <0:不稳定 =0:无阻尼 0~1:复共轭极点 >1:分裂成两个单极点
     *        low shelf -> H(0)=-db*2 H(pi)= 1
     *       tilt shelf -> H(0)= -db  H(pi)= db
     *       high shelf -> H(0)=  1   H(pi)=2*db
     */
    void SetCoeffSVF(float w, float r2, float db) noexcept {
        float const m = std::pow(10.0f, db / 80.0f);
        m2_ = m * m;
        invm2_ = 1 / m2_;
        g_ = std::tan(w / 2) / m;
        R2_ = r2;
        g1_ = r2 + g_;
        d_ = 1 / (1 + r2 * g_ + g_ * g_);
    }

    void SetCoeffQ(float w, float Q, float db) noexcept {
        SetCoeffSVF(w, 1 / Q, db);
    }

    float TickLowshelf(float x) noexcept {
        auto[hp,bp,lp] = TickMultiMode(x);
        return lp * invm2_ * invm2_ + bp * invm2_ + hp;
    }

    float TickHighshelf(float x) noexcept {
        auto[hp,bp,lp] = TickMultiMode(x);
        return lp + bp * m2_ + hp * m2_ * m2_;
    }

    float TickTiltshelf(float x) noexcept {
        auto[hp,bp,lp] = TickMultiMode(x);
        return lp * invm2_ + bp + hp * m2_;
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
private:
    float m2_{};
    float invm2_{};
    float R2_{};
    float g_{};
    float g1_{};
    float d_{};
    float s1_{};
    float s2_{};
};
}