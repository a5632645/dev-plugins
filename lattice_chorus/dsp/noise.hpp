#pragma once
#include <cmath>
#include <cstdlib>

namespace dsp {

class Noise {
public:
    void Init(float fs);
    void SetRate(float rate);
    void Reset();
    // 0~1
    inline float Tick() {
        phase_ += inc_;
        if (phase_ > 1.0f) {
            phase_ -= 1.0f;
            a_ = b_;
            b_ = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        }

        return std::lerp(a_, b_, phase_);
    }
private:
    float fs_;
    float inc_;
    float phase_;
    float a_;
    float b_;
};

}