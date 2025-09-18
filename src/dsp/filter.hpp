#pragma once
#include <span>
#include <numbers>
#include <cmath>

namespace dsp {

class Filter {
public:
    void Init(float sample_rate);
    void Process(std::span<float> block);
    float ProcessSingle(float x) {
        auto output = x * b0_ + latch1_;
        latch1_ = x * b1_ - output * a1_ + latch2_;
        latch2_ = x * b2_ - output * a2_;
        return output;
    }
    void MakeHighShelf(float db_gain, float freq, float s);
    void MakeHighPass(float pitch);
    void MakeDownSample(int dicimate);
    void MakeLowpassDirect(float omega);
    void ResetLatch();
    void MakeNone() {
        b0_ = 1.0f;
        b1_ = 0.0f;
        b2_ = 0.0f;
        a1_ = 0.0f;
        a2_ = 0.0f;
    }
private:
    float sample_rate_{};
    float b0_{};
    float b1_{};
    float b2_{};
    float a1_{};
    float a2_{};
    float latch1_{};
    float latch2_{};
};

class OnePoleFilter {
public:
    void SetCutoffHPF(float freq, float fs) {
        float omega = 2 * std::numbers::pi_v<float> * freq / fs;
        auto k = std::tan(omega / 2);
        b0_ = 1 / (1 + k);
        b1_ = -b0_;
        a1_ = (k - 1) / (k + 1);
    }

    void MakeNone() {
        b0_ = 1.0f;
        b1_ = 0.0f;
        a1_ = 0.0f;
    }

    void ClearInteral() {
        latch1_ = 0;
    }

    float Process(float in) {
        auto t = in - a1_ * latch1_;
        auto y = t * b0_ + b1_ * latch1_;
        latch1_ = t;
        return y;
    }
private:
    float b0_ = 0;
    float b1_ = 0;
    float a1_ = 0;
    float latch1_ = 0;
};

}