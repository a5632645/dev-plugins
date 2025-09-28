#pragma once
#include <cmath>

namespace qwqdsp::filter {
/**
 * @brief 它和biquad不能互换
 *                rsin * z^-1 * (b0 + b1*z^-1 + b2*z^-2)
 * H(z) = --------------------------------------------------------
 *         (z - 2rcos + r^2) or (z - rexp(w))(z - conj(rexp(w)))
 */
class GoldRader {
public:
    float Tick(float x) noexcept {
        float acc = b0_ * x + b1_ * x1_ + b2_ * x2_;
        x2_ = x1_;
        x1_ = x;
        acc += y1_ * rcos_;
        acc -= y2_ * rsin_;
        y2_ = y2_ * rcos_ + rsin_ * y1_;
        y1_ = acc;
        return y2_;
    }

    void Set(float b0, float b1, float b2, float pole_radius, float pole_omega) noexcept {
        rcos_ = pole_radius * std::cos(pole_omega);
        rsin_ = pole_radius * std::sin(pole_omega);
        b0_ = b0 / rsin_;
        b1_ = b1 / rsin_;
        b2_ = b2 / rsin_;
    }

    void Reset() noexcept {
        x1_ = 0;
        x2_ = 0;
        y1_ = 0;
        y2_ = 0;
    }

    void Copy(const GoldRader& other) noexcept {
        b0_ = other.b0_;
        b1_ = other.b1_;
        b2_ = other.b2_;
        rcos_ = other.rcos_;
        rsin_ = other.rsin_;
    }
private:
    float b0_{};
    float b1_{};
    float b2_{};
    float rcos_{};
    float rsin_{};
    float x1_{};
    float x2_{};
    float y1_{};
    float y2_{};
};
}