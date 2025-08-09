#pragma once
#include <complex>

namespace dsp {
class IIRHilbert {
public:
    std::complex<float> Tick(float x) {
        float real{};
        float imag{};
        real = real0_.Tick(x);
        real = real1_.Tick(real);
        real = real2_.Tick(real);
        real = real3_.Tick(real);
        imag = latch_;
        latch_ = imag0_.Tick(x);
        latch_ = imag1_.Tick(latch_);
        latch_ = imag2_.Tick(latch_);
        latch_ = imag3_.Tick(latch_);
        return {real, imag};
    }
private:
    template<float alpha>
    struct APF {
        float z0_{};
        float z1_{};
        float Tick(float x) {
            float in = x + alpha * z1_;
            float out = -alpha * in + z1_;
            z1_ = z0_;
            z0_ = in;
            return out;
        }
    };

    APF<0.4021921162426f> real0_;
    APF<0.8561710882420f> real1_;
    APF<0.9722909545651f> real2_;
    APF<0.9952884791278f> real3_;
    APF<0.6923878f> imag0_;
    APF<0.9360654322959f> imag1_;
    APF<0.9882295226860f> imag2_;
    APF<0.9987488452737f> imag3_;
    float latch_{};
};
}