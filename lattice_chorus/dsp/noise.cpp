#include "noise.hpp"

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

}