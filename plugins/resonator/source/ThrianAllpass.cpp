#include "ThrianAllpass.hpp"

#include <complex>
#include <numbers>

static constexpr auto twopi = std::numbers::pi_v<float> * 2;

namespace mana {

float ThrianAllpass::Process(float in) {
    for (uint32_t i = 0; i < kMaxNumAPF; ++i) {
        in = ProcessFilter(in, i);
    }
    return in;
}

float ThrianAllpass::GetPhaseDelay(float omega) const {
    auto cpx = std::polar(1.0f, omega);
    auto up = a2_ * cpx * cpx + a1_ * cpx + 1.0f;
    auto down = cpx * cpx + a1_ * cpx + a2_;
    auto radius = -std::arg(up / down);
    if (radius < 0) {
        radius += twopi;
    }
    return radius * kMaxNumAPF / omega;
}

static constexpr float Something(float delay, int k) {
    float ret = 1;
    for (int n = 0; n <= 2; ++n) {
        ret *= (delay - 2 + n) / (delay - 2 + k + n);
    }
    return ret;
}

void ThrianAllpass::SetGroupDelay(float delay) {
    if (delay < 1.0f) {
        a1_ = 0.0f;
        a2_ = 0.0f;
        b0_ = 1.0f;
        b2_ = 0.0f;
    }
    else {
        a1_ = -2 * Something(delay, 1);
        a2_ = Something(delay, 2);
        b0_ = a2_;
        b2_ = 1.0f;
    }
}

void ThrianAllpass::Panic() {
    for (auto& s : latchs_) {
        s.latch1_ = 0;
        s.latch2_ = 0;
    }
}

std::complex<float> ThrianAllpass::GetResponce(float omega) const {
    auto z = std::polar(1.0f, omega);
    auto up = a2_ * z * z + a1_ * z + 1.0f;
    auto down = z * z + a1_ * z + a2_;
    return up / down;
}

float ThrianAllpass::ProcessFilter(float in, uint32_t i) {
    auto& latch1 = latchs_[i].latch1_;
    auto& latch2 = latchs_[i].latch2_;
    auto t = in - a1_ * latch1 - a2_ * latch2;
    // auto y = t * a2_ + a1_ * latch1 + latch2;
    auto y = t * b0_ + a1_ * latch1 + b2_ * latch2;
    latch2 = latch1;
    latch1 = t;
    return y;
}

}
