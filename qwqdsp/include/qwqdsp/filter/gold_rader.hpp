#pragma once
#include <cmath>

// TODO: fix pole coeffience cause volume error

/**
 * @brief Gold-Rader form biquad, also known as couple-form
 *        it seems that this filter is siutable for fast-modulation
 */
namespace qwqdsp::filter {
class GoldRader {
public:
    float Tick(float x) {
        float acc = b0_ * x + b1_ * x1_ + b2_ * x2_;
        x2_ = x1_;
        x1_ = x;
        acc += y1_ * rcos_ - y2_ * rsin_;
        y2_ = y2_ * rcos_ + rsin_ * y1_;
        y1_ = acc;
        return y2_;
    }

    void Set(float b0, float b1, float b2, float pole_radius, float pole_omega) {
        b0_ = b0;
        b1_ = b1;
        b2_ = b2;
        rcos_ = pole_radius * std::cos(pole_omega);
        rsin_ = pole_radius * std::sin(pole_omega);
    }

    void Reset() {
        x1_ = 0;
        x2_ = 0;
        y1_ = 0;
        y2_ = 0;
    }

    void Copy(const GoldRader& other) {
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