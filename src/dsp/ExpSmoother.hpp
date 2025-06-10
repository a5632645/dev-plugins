#pragma once
#include <cmath>

namespace dsp{

class ExpSmoother {
public:
    void Init(float sampleRate) {
        sampleRate_ = sampleRate;
    }

    float Process(float in) {
        if (in > latch_) {
            latch_ = latch_ * biggerCoeff_ + (1 - biggerCoeff_) * in;
        }
        else {
            latch_ = latch_ * smallerCoeff_ + (1 - smallerCoeff_) * in;
        }
        return latch_;
    }

    void SetAttackTime(float ms) {
        biggerCoeff_ = std::exp(-1.0f / (sampleRate_ * ms / 1000.0f));
    }

    void SetReleaseTime(float ms) {
        smallerCoeff_ = std::exp(-1.0f / (sampleRate_ * ms / 1000.0f));
    }
private:
    float sampleRate_ = 0;
    float latch_ = 0;
    float biggerCoeff_ = 0;
    float smallerCoeff_ = 0;
};

}