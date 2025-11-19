#pragma once
#include <cmath>

namespace qwqdsp::filter {
class Ladder {
public:
    void Reset() noexcept {
        s1_ = 0;
        s2_ = 0;
        s3_ = 0;
        s4_ = 0;
    }

    float Tick(float x) noexcept {
        float S = g2_ * s2_ + glp_ * (s3_ + s1_ * glp_) + s4_;
        S /= (1 + g_);
        float u = (x - k_ * S) / (1 + k_ * g4_);
        float y = TickLpTPT(u, s1_, glp_);
        y = TickLpTPT(y, s2_, glp_);
        y = TickLpTPT(y, s3_, glp_);
        y = TickLpTPT(y, s4_, glp_);
        return y;
    }

    auto TickMultiMode(float x) noexcept {
        struct Output {
            float hp;
            float lp1;
            float lp2;
            float lp3;
            float lp4;
        };
        Output r;

        float S = g2_ * s2_ + glp_ * (s3_ + s1_ * glp_) + s4_;
        S /= (1 + g_);
        r.hp = (x - k_ * S) / (1 + k_ * g4_);
        r.lp1 = TickLpTPT(r.hp, s1_, glp_);
        r.lp2 = TickLpTPT(r.lp1, s2_, glp_);
        r.lp3 = TickLpTPT(r.lp2, s3_, glp_);
        r.lp4 = TickLpTPT(r.lp3, s4_, glp_);
        return r;
    }

    float TickHighpass(float x) noexcept {
        auto r = TickMultiMode(x);
        return r.hp - 4 * (r.lp1 + r.lp3) + r.lp4 + 6 * r.lp2;
    }

    float TickBandpass(float x) noexcept {
        auto r = TickMultiMode(x);
        return r.lp2 + r.lp4 - 2 * r.lp3;
    }

    // -------------------- 'true' ladder --------------------
    auto TickTrueHighpass(float x) noexcept {
        struct Output {
            float hp1;
            float hp2;
            float hp3;
            float hp4;
        } output;

        float g = 1 / (1 + g_);
        float s = g * g * g * g * s1_ + g * g * g * s2_ + g * g * s3_ + g * s4_;
        s *= k_;
        s /= (1 + g * g * g * g * k_);
        float u = x + s;
        output.hp1 = TickHpTPT(u, s1_, glp_);
        output.hp2 = TickHpTPT(output.hp1, s1_, glp_);
        output.hp3 = TickHpTPT(output.hp2, s1_, glp_);
        output.hp4 = TickHpTPT(output.hp3, s1_, glp_);
        return output;
    }

    auto TickTrueBandpass(float x) noexcept {
        struct Output {
            float BPn1;
            float BPn2;
        } output;

        float g = g_;
        float B = 1 / (1 + 2 * R_ * g + g * g);
        float u = B * B * R_ * g * k_ * (g * s2_ - s1_) + B * k_ * (g * s4_ - s3_) - x;
        u /= (B * B * R_ * R_ * g * g * k_ - 1);

        output.BPn1 = TickHalfNormBandpass(u * R_, g, s1_, s2_, B);
        output.BPn2 = TickHalfNormBandpass(output.BPn1 * R_, g, s3_, s4_, B);

        return output;
    }

    /**
     * @param w [0, pi-0.1]
     * @param k [0, 3.99] => [实数极点, 自震荡]
     */
    void Set(float w, float k, float Q) noexcept {
        R_ = 0.5f / Q;
        g_ = std::tan(w / 2);
        glp_ = g_ / (1 + g_);
        g2_ = glp_ * glp_;
        g4_ = g2_ * g2_;
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

    static float TickHpTPT(float x, float& s, float geps) noexcept {
        float const delta = geps * (x - s);
        s += delta;
        float const y = s;
        s += delta;
        return x - y;
    }

    static float TickHalfNormBandpass(float x, float g, float& s1, float& s2, float B) noexcept {
        float bp = B * (g * (x - s2) + s1);
        float bp2 = bp + bp;
        s1 = bp2 - s1;
        float v22 = g * bp2;
        s2 += v22;
        return bp;
    }

    float R_{};
    float k_{};
    float g_{};
    float g2_{};
    float g4_{};
    float glp_{};
    float s1_{};
    float s2_{};
    float s3_{};
    float s4_{};
};
}