#pragma once
#include <algorithm>
#include <numbers>
#include <cmath>

namespace qwqdsp_filter {
class Ladder8Pole {
public:
    void Reset() noexcept {
        std::fill_n(s_, 8, 0.0f);
    }

    /**
     * @param k [0,1.884]
     */
    void Set(float w, float k) noexcept {
        g_ = std::tan(w / 2) * (1 + std::numbers::sqrt2_v<float>);
        k_ = k;
    }

    auto Tick(float x) noexcept {
        struct Output {
            float lp1;
            float lp2;
            float lp3;
            float lp4;
            float lp5;
            float lp6;
            float lp7;
            float lp8;
        } output;

        float B = 1 / (1 + g_);
        float A = g_ * B;
        float S = s_[7] + A * (s_[6] + A * (s_[5] + A * (s_[4] + A * (s_[3] + A * (s_[2] + A * (s_[1] + A * s_[0]))))));
        float u = x - B * k_ * S;
        u /= (1 + k_ * A * A * A * A * A * A * A * A);

        output.lp1 = TickLpTPT(u, s_[0], A);
        output.lp2 = TickLpTPT(output.lp1, s_[1], A);
        output.lp3 = TickLpTPT(output.lp2, s_[2], A);
        output.lp4 = TickLpTPT(output.lp3, s_[3], A);
        output.lp5 = TickLpTPT(output.lp4, s_[4], A);
        output.lp6 = TickLpTPT(output.lp5, s_[5], A);
        output.lp7 = TickLpTPT(output.lp6, s_[6], A);
        output.lp8 = TickLpTPT(output.lp7, s_[7], A);

        return output;
    }
private:
    static float TickLpTPT(float x, float& s, float geps) noexcept {
        float const delta = geps * (x - s);
        s += delta;
        float const y = s;
        s += delta;
        return y;
    }

    float s_[8]{};
    float g_{};
    float k_{};
};
}
