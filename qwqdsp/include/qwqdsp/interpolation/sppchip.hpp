#pragma once
#include <cassert>
#include <cstddef>
#include <span>

namespace qwqdsp::interpolation {
class SPPCHIP {
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
                return y0_ + s * (
                    d0_ + s * (
                        c0_ + s * b0_
                    )
                );
            }
        }
    }
    
    void StepNextPoint() noexcept {
        // 起点超出插值范围
        assert(rpos_ < xs_.size() - 1);
        
        float h0 = xs_[rpos_ + 1] - xs_[rpos_];
        float e0 = (ys_[rpos_ + 1] - ys_[rpos_]) / h0;
        float d0 = GetDerivative(rpos_);
        float d1 = GetDerivative(rpos_ + 1);
        d0_ = d0;
        x0_ = xs_[rpos_];
        y0_ = ys_[rpos_];
        c0_ = (3 * e0 - 2 * d0 - d1) / h0;
        b0_ = (d0 - 2 * e0 + d1) / (h0 * h0);
        ++rpos_;
    }
private:
    float GetDerivative(size_t i) const noexcept {
        if (i == 0) {
            auto y0 = ys_[i];
            auto y1 = ys_[i + 1];
            auto y2 = ys_[i + 2];
            auto x0 = xs_[i];
            auto x1 = xs_[i + 1];
            auto x2 = xs_[i + 2];
            auto e0 = (y1 - y0) / (x1 - x0);
            auto e1 = (y2 - y1) / (x2 - x1);
            auto h0 = x1 - x0;
            auto h1 = x2 - x1;
            auto d0 = ((2 * h0 + h1) * e0 - h0 * e1) / (h0 + h1);
            if (d0 * e0 < 0) {
                d0 = 0;
            }
            else if (e0 * e1 < 0 && std::abs(d0) > 3.0f * std::abs(e0)) {
                d0 = 3.0f * e0;
            }
            return d0;
        }
        else if (i == xs_.size() - 1) {
            auto y0 = ys_[i - 3];
            auto y1 = ys_[i - 2];
            auto y2 = ys_[i - 1];
            auto y3 = ys_[i];
            auto x0 = xs_[i - 3];
            auto x1 = xs_[i - 2];
            auto x2 = xs_[i - 1];
            auto x3 = xs_[i];
            auto e0 = (y1 - y0) / (x1 - x0);
            auto e1 = (y2 - y1) / (x2 - x1);
            auto e2 = (y3 - y2) / (x3 - x2);
            auto h0 = x1 - x0;
            auto h1 = x2 - x1;
            auto h2 = x3 - x2;
            auto d3 = ((2 * h1 + h1) * e2 - h2 * e1) / (h1 + h2);
            if (d3 * e2 < 0) {
                d3 = 0;
            }
            else if (e2 * e1 < 0 && std::abs(d3) > 3.0f * std::abs(e2)) {
                d3 = 3.0f * e2;
            }
            return d3;
        }
        else {
            auto y0 = ys_[i - 1];
            auto y1 = ys_[i];
            auto y2 = ys_[i + 1];
            auto x0 = xs_[i - 1];
            auto x1 = xs_[i];
            auto x2 = xs_[i + 1];
            auto e0 = (y1 - y0) / (x1 - x0);
            auto e1 = (y2 - y1) / (x2 - x1);
            auto h0 = x1 - x0;
            auto h1 = x2 - x1;
            auto d1 = 0.0f;
            if (e0 == 0.0f || e1 == 0.0f || e0 * e1 < 0) {
                d1 = 0;
            }
            else {
                auto w1 = 2 * h1 + h0;
                auto w2 = h1 + 2 * h0;
                d1 = (w1 + w2) * (e0 * e1) / (w1 * e1 + w2 * e0);
            }
            return d1;
        }
    }

    float d0_{};
    float y0_{};
    float x0_{};
    float c0_{};
    float b0_{};

    size_t rpos_{};
    std::span<const float> xs_;
    std::span<const float> ys_;
};
}