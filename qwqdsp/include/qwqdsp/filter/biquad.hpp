#pragma once

namespace qwqdsp {
class Biquad {
public:
    float Tick(float x) {
        auto output = x * b0_ + latch1_;
        latch1_ = x * b1_ - output * a1_ + latch2_;
        latch2_ = x * b2_ - output * a2_;
        return output;
    }
private:
    float b0_{};
    float b1_{};
    float b2_{};
    float a1_{};
    float a2_{};
    float latch1_{};
    float latch2_{};
};
}