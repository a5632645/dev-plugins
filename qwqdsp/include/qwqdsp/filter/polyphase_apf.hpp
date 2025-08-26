#pragma once
#include <cstddef>
#include <complex>
#include "qwqdsp/fx/delay_line.hpp"

namespace qwqdsp::filter {
class PolyphaseAPF {
public:
    void Init(size_t max_samples) {
        buffer_.Init(max_samples);
    }

    void Deinit() {
        buffer_.Deinit();
    }

    float Tick(float x) {
        float z1 = buffer_.GetBeforePush(n_latch_);
        float in = x + alpha_ * z1;
        float out = -alpha_ * in + z1;
        buffer_.Push(in);
        return out;
    }

    void SetAlpha(float a) {
        alpha_ = a;
    }

    void SetNLatch(size_t n) {
        n_latch_ = n;
    }

    std::complex<float> GetResponce(std::complex<float> z) const {
        auto zpow = std::pow(z, -static_cast<float>(n_latch_));
        auto up = zpow - alpha_;
        auto down = 1.0f - alpha_ * zpow;
        return up / down;
    }
private:
    float alpha_{};
    fx::DelayLine<fx::DelayLineInterp::None> buffer_;
    size_t n_latch_{};
};
}