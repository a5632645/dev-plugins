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
    }

    void MakePass() noexcept {
        g_ = 1;
    }

    void CopyFrom(OnePoleTPT const& other) noexcept {
        g_ = other.g_;
    }

    /**
     * @note highpass = x - lowpass
     */
    float Tick(float x) noexcept {
        float const delta = g_ * (x - lag_);
        lag_ += delta;
        float const y = lag_;
        lag_ += delta;
        return y;
    }

    float TickHighpass(float x) noexcept {
        float const delta = g_ * (x - lag_);
        lag_ += delta;
        float const y = lag_;
        lag_ += delta;
        return x - y;
    }

    float TickHighshelf(float x, float gain) noexcept {
        float lp = Tick(x);
        return lp + gain * (x - lp);
    }
private:
    float g_{};
    float lag_{};
};
}
