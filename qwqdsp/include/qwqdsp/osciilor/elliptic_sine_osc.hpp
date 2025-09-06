#pragma once
#include <cmath>

namespace qwqdsp::oscillor {
/**
 * @ref https://ccrma.stanford.edu/~juhan/pubs/jnam-emile05.pdf
 */
class EllipticSineOsc {
public:
    // 余弦波初始化
    void Reset() noexcept {
        x_ = 1;
        y_ = 0;
        backup_y_ = 0;
        w_old_ = 0.0f;
    }

    // 正弦波初始化
    void Reset(float w) noexcept {
        x_ = 0;
        y_ = std::sin(w);
        backup_y_ = y_;
        k_ = std::cos(w);
        w_old_ = w;
    }

    // 正弦波初始化
    void Reset(float w, float phase) noexcept {
        x_ = std::sin(phase);
        y_ = std::sin(w) * std::cos(phase);
        backup_y_ = y_;
        w_old_ = w;
        k_ = std::cos(w);
    }

    /**
     * @param w [0, pi)
     */
    void SetFreq(float w) noexcept {
        k_ = std::cos(w);
        [[unlikely]]
        if (w == 0.0f) {
            [[likely]]
            if (w_old_ != 0.0f) {
                backup_y_ = y_ / std::sin(w_old_);
            }
            y_ = 0;
        }
        else {
            [[unlikely]]
            if (w_old_ == 0.0f) {
                y_ = backup_y_ * std::sin(w);
            }
            else {
                y_ *= std::sin(w) / std::sin(w_old_);
            }
        }
        w_old_ = w;
    }

    float Tick() noexcept {
        float const newx = k_ * x_ + y_;
        y_ = k_ * newx - x_;
        x_ = newx;
        return Sine();
    }

    float Sine() const noexcept {
        return x_;
    }

    /**
     * @return 准确值 cos(wn) * sin(w)
     */
    float PCosine() const noexcept {
        return y_;
    }
private:
    float x_{};
    float y_{};
    float k_{};
    float w_old_{};
    float backup_y_{};
};
}