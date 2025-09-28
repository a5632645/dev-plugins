#pragma once
#include <cassert>
#include <cstddef>
#include <span>

namespace qwqdsp::interpolation {
class Linear {
public:
    void Reset(std::span<const float> xs, std::span<const float> ys) noexcept {
        rpos_ = 0;
        xs_ = xs;
        ys_ = ys;
        StepNextPoint();
    }

    float Next(float x) noexcept {
        [[unlikely]]
        if (xs_.size() == 2) {
            float e0 = (ys_[1] - ys_[0]) / (xs_[1] - xs_[0]);
            return ys_[0] + e0 * (x - xs_[0]);
        }
        else {
            [[unlikely]]
            if (x >= xs_.back()) {
                return ys_.back();
            }
            else [[unlikely]] if (x < xs_.front()) {
                return ys_.front();
            }
            else {
                while (x > xs_[rpos_]) {
                    StepNextPoint();
                }
                float s = x - x0_;
                return m0_ * s + y0_;
            }
        }
    }
    
    void StepNextPoint() noexcept {
        // 起点超出插值范围
        assert(rpos_ < xs_.size() - 1);

        x0_ = xs_[rpos_];
        y0_ = ys_[rpos_];
        m0_ = (ys_[rpos_ + 1] - ys_[rpos_]) / (xs_[rpos_ + 1] - xs_[rpos_]);
        ++rpos_;
    }
private:
    float m0_{};
    float x0_{};
    float y0_{};

    size_t rpos_{};
    std::span<const float> xs_;
    std::span<const float> ys_;
};
}