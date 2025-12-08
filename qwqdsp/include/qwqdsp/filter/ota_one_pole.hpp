#pragma once
#include <cmath>

namespace qwqdsp_filter {
class OTAOnePole {
public:
    void Reset() noexcept {
        s_ = 0;
    }

    void Set(float w) noexcept {
        g_ = std::tan(w / 2);
    }

    float Tick(float x) noexcept {
        float bias = x - s_;
        float g = g_;
        float u = 0;
        if (bias > 0) {
            // u > 0
            float t = -1 - g - s_ + x;
            u = t + std::sqrt(4*(x-s_)+t*t);
        }
        else {
            // u < 0
            float t = 1 + g - s_ + x;
            u = t - std::sqrt(4*(s_-x)+t*t);
        }
        u /= 2;

        u = u / (1 + std::abs(u));
        u *= g;
        s_ += u;
        float y = s_;
        s_ += u;
        
        return y;
    }
private:
    float s_{};
    float g_{};
};
}
