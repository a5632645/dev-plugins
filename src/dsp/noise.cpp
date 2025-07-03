#include "noise.hpp"
#include <cstdlib>

namespace dsp {

void Noise::Init(float fs) {
    fs_ = fs;
}

void Noise::SetRate(float rate) {
    inc_ = rate / fs_;
}

void Noise::Reset() {
    a_ = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    b_ = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    c_ = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    d_ = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    phase_ = 0.0f;
}

float Noise::Tick() {
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

}