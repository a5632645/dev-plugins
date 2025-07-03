#include "filter.hpp"
#include <cmath>
#include <numbers>

namespace dsp {

void Filter::Init(float sample_rate) {
    sample_rate_ = sample_rate;
}

float Filter::ProcessSingle(float x) {
    auto t = x - a1_ * latch1_ - a2_ * latch2_;
    auto y = t * b0_ + b1_ * latch1_ + b2_ * latch2_;
    latch2_ = latch1_;
    latch1_ = t;
    return y;
}

void Filter::Process(std::span<float> block) {
    for (float& s : block) {
        s = ProcessSingle(s);
    }
}

void Filter::MakeHighShelf(float db_gain, float freq, float s) {
    constexpr float pi = std::numbers::pi_v<float>;
    float A = std::pow(10.0f, db_gain / 40.0f);
    float omega = 2 * pi * freq / sample_rate_;
    float cos_omega = std::cos(omega);
    float sin_omega = std::sin(omega);
    float alpha = sin_omega / 2 * std::sqrt((A + 1 / A) * (1 / s - 1) + 2);

    float b0 = A * ((A + 1) + (A - 1) * cos_omega + 2 * std::sqrt(A) * alpha);
    float b1 = -2 * A * ((A - 1) + (A + 1) * cos_omega);
    float b2 = A * ((A + 1) + (A - 1) * cos_omega - 2 * std::sqrt(A) * alpha);
    float a0 = (A + 1) - (A - 1) * cos_omega + 2 * std::sqrt(A) * alpha;
    float a1 = 2 * ((A - 1) - (A + 1) * cos_omega);
    float a2 = (A + 1) - (A - 1) * cos_omega - 2 * std::sqrt(A) * alpha;

    b0_ = b0 / a0;
    b1_ = b1 / a0;
    b2_ = b2 / a0;
    a1_ = a1 / a0;
    a2_ = a2 / a0;
}

void Filter::MakeHighPass(float pitch) {
    float freq = std::exp2((pitch - 69.0f) / 12.0f) * 440.0f;
    float omega = 2 * std::numbers::pi_v<float> * freq / sample_rate_;
    auto k = std::tan(omega / 2);
    b0_ = 1 / (1 + k);
    b1_ = -b0_;
    b2_ = 0;
    a1_ = (k - 1) / (k + 1);
    a2_ = 0;
}

void Filter::MakeLowpassDirect(float omega) {
    auto k = std::tan(omega / 2);
    constexpr auto Q = 1.0f / std::numbers::sqrt2_v<float>;
    auto down = k * k * Q + k + Q;
    b0_ = k * k * Q / down;
    b1_ = 2 * b0_;
    b2_ = b0_;
    a1_ = 2 * Q * (k * k - 1) / down;
    a2_ = (k * k * Q - k + Q) / down;
}

void Filter::MakeDownSample(int dicimate) {
    if (dicimate == 1) {
        b0_ = 1.0f;
        b1_ = 0.0f;
        b2_ = 0.0f;
        a1_ = 0.0f;
        a2_ = 0.0f;
    }
    else {
        auto omega = std::numbers::pi_v<float> / dicimate;
        auto k = std::tan(omega / 2);
        constexpr auto Q = 1.0f / std::numbers::sqrt2_v<float>;
        auto down = k * k * Q + k + Q;
        b0_ = k * k * Q / down;
        b1_ = 2 * b0_;
        b2_ = b0_;
        a1_ = 2 * Q * (k * k - 1) / down;
        a2_ = (k * k * Q - k + Q) / down;
    }
}

void Filter::ResetLatch() {
    latch1_ = 0.0f;
    latch2_ = 0.0f;
}

void Filter::MakeLowpass(float freq, float Q) {
    
}

}