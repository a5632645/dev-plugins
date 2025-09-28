#pragma once
#include <cassert>
#include <cstddef>
#include <span>
#include <cmath>

namespace qwqdsp::interpolation {
class Makima {
public:
    void Reset(std::span<const float> xs, std::span<const float> ys) noexcept {
        rpos_ = 0;
        xs_ = xs;
        ys_ = ys;
        // 计算边界外的斜率
        if (xs.size() > 2) {
            float e0 = (ys[1] - ys[0]) / (xs[1] - xs[0]);
            float e1 = (ys[2] - ys[1]) / (xs[2] - xs[0]);
            en1_ = 2 * e0 - e1;
            en2_ = 2 * en1_ - e0;
            size_t n = xs.size();
            float elast = (ys[n - 1] - ys[n - 2]) / (xs[n - 1] - xs[n - 2]);
            float elast2 = (ys[n - 2] - ys[n - 3]) / (xs[n - 2] - xs[n - 3]);
            e1_ = 2 * elast - elast2;
            e2_ = 2 * e1_ - elast;
        }
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
                return y0_ + s * d0_ + s * s * c0_ + s * s * s * b0_;
            }
        }
    }
    
    void StepNextPoint() noexcept {
        // 起点超出插值范围
        assert(rpos_ < xs_.size() - 1);

        float en2 = GetSlope(rpos_ - 2);
        float en1 = GetSlope(rpos_ - 1);
        float e0 = GetSlope(rpos_);
        float e1 = GetSlope(rpos_ + 1);
        float e2 = GetSlope(rpos_ + 2);
        float w1 = std::abs(e1 - e0) + std::abs(e1 + e0) * 0.5f;
        float w2 = std::abs(en1 - en2) + std::abs(en1 + en2) * 0.5f;
        float w1x = std::abs(e2 - e1) + std::abs(e2 + e1) * 0.5f;
        float w2x = std::abs(e0 - en1) + std::abs(e0 + en1) * 0.5f;
        float d0 = (w1 * en1 + w2 * e0) / (w1 + w2);
        float d1 = (w1x * e0 + w2x * e1) / (w1x + w2x);
        [[unlikely]]
        if (std::isnan(d0)) {
            d0 = 0.0f;
        }
        [[unlikely]]
        if (std::isnan(d1)) {
            d1 = 0.0f;
        }

        x0_ = xs_[rpos_];
        x1_ = xs_[rpos_ + 1];
        float h0 = x1_ - x0_;
        c0_ = (3 * e0 - 2 * d0 - d1) / h0;
        b0_ = (d0 - 2 * e0 + d1) / (h0 * h0);
        y0_ = ys_[rpos_];
        d0_ = d0;

        ++rpos_;
    }
private:
    float GetSlope(int idx) noexcept {
        if (idx == -1) {
            return en1_;
        }
        else if (idx == -2) {
            return en2_;
        }
        else if (idx == xs_.size() - 1) {
            return e1_;
        }
        else if (idx == xs_.size()) {
            return e2_;
        }
        else {
            return (ys_[idx + 1] - ys_[idx]) / (xs_[idx + 1] - xs_[idx]);
        }
    }

    // 边界之外的斜率
    float en2_{};
    float en1_{};
    float e1_{};
    float e2_{};

    float x0_{};
    float x1_{};
    float c0_{};
    float b0_{};
    float y0_{};
    float d0_{};

    int rpos_{};
    std::span<const float> xs_;
    std::span<const float> ys_;
};
}