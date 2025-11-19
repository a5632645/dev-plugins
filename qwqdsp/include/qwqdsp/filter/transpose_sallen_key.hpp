#pragma once
#include <cmath>

namespace qwqdsp::filter {
class TransposeSallenKey {
public:
    void Reset() noexcept {
        s1_ = 0;
        s2_ = 0;
    }

    auto Tick(float x) noexcept {
        struct Output {
            float lp2;
            float bp2;
            float hp2;
        } output;

        float G = (1 + g_);
        float u = x * G * G - G * k_ * s2_ + k_ * s1_;
        u /= (G * G - g_ * k_);
        float lp1 = TickLpTPT(u, s1_, glp_);
        float hp1 = u - lp1;
        output.lp2 = TickLpTPT(lp1, s2_, glp_);
        output.bp2 = lp1 - output.lp2;
        output.hp2 = hp1 - output.bp2;
        return output;
    }

    /**
     * @param w [0,pi]
     * @param k [0,2] => [实数极点,自震荡]
     */
    void Set(float w, float k) noexcept {
        g_ = std::tan(w / 2);
        glp_ = g_ / (1 + g_);
        k_ = k;
    }
private:
    static float TickLpTPT(float x, float& s, float geps) noexcept {
        float const delta = geps * (x - s);
        s += delta;
        float const y = s;
        s += delta;
        return y;
    }

    float g_{};
    float glp_{};
    float k_{};
    float s1_{};
    float s2_{};
};
}