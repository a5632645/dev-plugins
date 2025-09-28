#pragma once

namespace qwqdsp::filter {
class Biquad {
public:
    void Reset() noexcept {
        latch1_ = 0;
        latch2_ = 0;
    }

    float Tick(float x) noexcept {
        auto output = x * b0_ + latch1_;
        latch1_ = x * b1_ - output * a1_ + latch2_;
        latch2_ = x * b2_ - output * a2_;
        return output;
    }

    void Set(float b0, float b1, float b2, float a1, float a2) noexcept {
        b0_ = b0;
        b1_ = b1;
        b2_ = b2;
        a1_ = a1;
        a2_ = a2;
    }

    void Copy(const Biquad& other) noexcept {
        b0_ = other.b0_;
        b1_ = other.b1_;
        b2_ = other.b2_;
        a1_ = other.a1_;
        a2_ = other.a2_;
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