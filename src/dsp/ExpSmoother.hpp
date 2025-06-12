#pragma once
#include <cmath>

namespace dsp{

template<class Type>
class ExpSmoother {
public:
    void Init(Type sampleRate) {
        sampleRate_ = sampleRate;
    }

    Type Process(Type in) {
        if (in > latch_) {
            latch_ = latch_ * biggerCoeff_ + (1 - biggerCoeff_) * in;
        }
        else {
            latch_ = latch_ * smallerCoeff_ + (1 - smallerCoeff_) * in;
        }
        return latch_;
    }

    void SetAttackTime(Type ms) {
        biggerCoeff_ = std::exp((Type)-1.0 / (sampleRate_ * ms / (Type)1000.0));
    }

    void SetReleaseTime(Type ms) {
        smallerCoeff_ = std::exp((Type)-1.0 / (sampleRate_ * ms / (Type)1000.0));
    }
private:
    Type sampleRate_ = 0;
    Type latch_ = 0;
    Type biggerCoeff_ = 0;
    Type smallerCoeff_ = 0;
};

}