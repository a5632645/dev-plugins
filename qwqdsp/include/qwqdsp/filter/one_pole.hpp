#pragma once
#include <cmath>

namespace qwqdsp::filter {
class OnePoleFilter {
public:
    void Reset() noexcept {
        latch1_ = 0;
    }
    
    void SetLPF(float w) noexcept {
        auto k = std::tan(w / 2);
        b0_ = k / (1 + k);
        b1_ = b0_;
        a1_ = (k - 1) / (k + 1);
    }

    void SetHPF(float w) noexcept {
        auto k = std::tan(w / 2);
        b0_ = 1 / (1 + k);
        b1_ = -b0_;
        a1_ = (k - 1) / (k + 1);
    }

    void MakePass() noexcept {
        b0_ = 1;
        b1_ = 0;
        a1_ = 0;
    }

    void MakeHighShelf(float w, float amp) noexcept {
        auto k = std::tan(w / 2);
        b0_ = (k + amp) / (1 + k);
        b1_ = (k - amp) / (1 + k);
        a1_ = (k - 1) / (k + 1);
    }

    void CopyFrom(const OnePoleFilter& other) noexcept {
        b0_ = other.b0_;
        b1_ = other.b1_;
        a1_ = other.a1_;
    }

    float Tick(float in) noexcept {
        auto t = in - a1_ * latch1_;
        auto y = t * b0_ + b1_ * latch1_;
        latch1_ = t;
        return y;
    }
private:
    float b0_ = 0;
    float b1_ = 0;
    float a1_ = 0;
    float latch1_ = 0;
};
}
