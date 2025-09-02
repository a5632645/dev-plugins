#pragma once
#include <cstddef>
#include <span>
#include <cmath>
#include <cassert>

namespace qwqdsp::interpolation {
class CatmullRomSpline {
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
                auto s = (x - x0_) / (x1_ - x0_);
                return d_ + s * (
                    c_ + s * (b_
                        + s * a_
                    )
                );
            }
        }
    }
    
    void StepNextPoint() noexcept {
        // 起点超出插值范围
        assert(rpos_ < xs_.size() - 1);

        if (rpos_ == 0) {
            float x0 = xs_[rpos_];
            float x1 = xs_[rpos_ + 1];
            float x2 = xs_[rpos_ + 2];
            float y0 = ys_[rpos_];
            float y1 = ys_[rpos_ + 1];
            float y2 = ys_[rpos_ + 2];
            float t01 = std::sqrt(x1 - x0);
            float t12 = std::sqrt(x2 - x1);
            float m1 = (1.0f - tension_) * (y2 - y1 + t12 * ((y1 - y0) / t01 - (y2 - y0) / (t01 + t12)));
            float m0 = (y1 - y0) / (x1 - x0);
            a_ = 2.0f * (y0 - y1) + m0 + m1;
            b_ = -3.0f * (y0 - y1) - 2.0f * m0 - m1;
            c_ = m0;
            d_ = y0;
            x0_ = x0;
            x1_ = x1;
        }
        else if (rpos_ == xs_.size() - 2) {
            float x1 = xs_[rpos_ - 1];
            float x2 = xs_[rpos_];
            float x3 = xs_[rpos_ + 1];
            float y1 = ys_[rpos_ - 1];
            float y2 = ys_[rpos_];
            float y3 = ys_[rpos_ + 1];
            float t12 = std::sqrt(x2 - x1);
            float t23 = std::sqrt(x3 - x2);
            float m2 = (1.0f - tension_) * (y2 - y1 + t12 * ((y3 - y2) / t23 - (y3 - y1) / (t12 + t23)));
            float m3 = (y3 - y2) / (x3 - x2);
            a_ = 2.0f * (y2 - y3) + m2 + m3;
            b_ = -3.0f * (y2 - y3) - 2.0f * m2 - m3;
            c_ = m2;
            d_ = y2;
            x0_ = x2;
            x1_ = x3;
        }
        else {
            float x0 = xs_[rpos_ - 1];
            float x1 = xs_[rpos_];
            float x2 = xs_[rpos_ + 1];
            float x3 = xs_[rpos_ + 2];
            float y0 = ys_[rpos_ - 1];
            float y1 = ys_[rpos_];
            float y2 = ys_[rpos_ + 1];
            float y3 = ys_[rpos_ + 2];
            float t01 = std::sqrt(x1 - x0);
            float t12 = std::sqrt(x2 - x1);
            float t23 = std::sqrt(x3 - x2);
            float m1 = (1.0f - tension_) * (y2 - y1 + t12 * ((y1 - y0) / t01 - (y2 - y0) / (t01 + t12)));
            float m2 = (1.0f - tension_) * (y2 - y1 + t12 * ((y3 - y2) / t23 - (y3 - y1) / (t12 + t23)));
            a_ = 2.0f * (y1 - y2) + m1 + m2;
            b_ = -3.0f * (y1 - y2) - 2.0f * m1 - m2;
            c_ = m1;
            d_ = y1;
            x0_ = x1;
            x1_ = x2;
        }
        ++rpos_;
    }

    void SetTension(float tension) noexcept {
        tension_ = tension;
    }
private:
    float a_{};
    float b_{};
    float c_{};
    float d_{};
    float x0_{};
    float x1_{};
    float tension_{};

    size_t rpos_{};
    std::span<const float> xs_;
    std::span<const float> ys_;
};
}