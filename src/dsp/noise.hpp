#pragma once
#include <cstdlib>

namespace dsp {

class Noise {
public:
    void Init(float fs);
    void SetRate(float rate);
    void Reset();
    inline float Tick() {
        float a = -a_ / 2.0f + 3.0f * b_ / 2.0f - 3.0f * c_ / 2.0f + d_ / 2.0f;
        float b = a_ - 5.0f * b_ / 2.0f + 2.0f * c_ - d_ / 2.0f;
        float c = -a_ / 2.0f + c_ / 2.0f;
        float d = b_;
        float t = phase_;
        float v = a * t * t * t + b * t * t + c * t + d;

        phase_ += inc_;
        if (phase_ > 1.0f) {
            phase_ -= 1.0f;
            a_ = b_;
            b_ = c_;
            c_ = d_;
            d_ = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        }

        return v;
    }
private:
    float fs_;
    float inc_;
    float phase_;
    float a_;
    float b_;
    float c_;
    float d_;
};

}