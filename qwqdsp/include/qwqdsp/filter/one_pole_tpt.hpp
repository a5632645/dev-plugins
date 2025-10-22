#pragma once
#include <cmath>

namespace qwqdsp::filter {
class OnePoleTPT {
public:
    void Reset() noexcept {
        lag_ = 0;
    }
    
    void MakeLowpass(float w) noexcept {
        auto k = std::tan(w / 2);
        g_ = k / (1 + k);
        G_ = 1 / (1 + k);
    }

    void MakePass() noexcept {
        g_ = 1;
        G_ = 0;
    }

    void CopyFrom(OnePoleTPT const& other) noexcept {
        g_ = other.g_;
        G_ = other.G_;
    }

    /**
     * @note 多模式输出
     *       hp = x - lp
     *       ap = lp - hp or ap = 2*lp - x
     */
    float TickLowpass(float x) noexcept {
        float const delta = g_ * (x - lag_);
        lag_ += delta;
        float const y = lag_;
        lag_ += delta;
        return y;
    }

    float TickHighpass(float x) noexcept {
        float const xs = x - lag_;
        float const y = xs * G_;
        lag_ += y * 2 * g_;
        return y;
    }

    float TickAllpass(float x) noexcept {
        float const xs = x - lag_;
        lag_ += xs * 2 * G_;
        return lag_ - xs;
    }
private:
    float g_{};
    float G_{};
    float lag_{};
};
}
