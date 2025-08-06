#pragma once
#include <cmath>

namespace dsp{

template<class Type>
class ExpSmoother {
public:
    Type Process(Type in) {
        if (in > latch_) {
            latch_ = latch_ * biggerCoeff_ + (1 - biggerCoeff_) * in;
        }
        else {
            latch_ = latch_ * smallerCoeff_ + (1 - smallerCoeff_) * in;
        }
        return latch_;
    }

    void SetAttackTime(Type ms, Type fs) {
        biggerCoeff_ = std::exp((Type)-1.0 / (fs * ms / (Type)1000.0));
    }

    void SetReleaseTime(Type ms, Type fs) {
        smallerCoeff_ = std::exp((Type)-1.0 / (fs * ms / (Type)1000.0));
    }
private:
    Type latch_ = 0;
    Type biggerCoeff_ = 0;
    Type smallerCoeff_ = 0;
};

}