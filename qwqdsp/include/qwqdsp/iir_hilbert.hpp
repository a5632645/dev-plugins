#pragma once
#include <complex>

namespace qwqdsp {
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

class IIRHilbertDeeper {
public:
    std::complex<float> Tick(float x) {
        float real{};
        float imag{};
        real = real0_.Tick(x);
        real = real1_.Tick(real);
        real = real2_.Tick(real);
        real = real3_.Tick(real);
        real = real4_.Tick(real);
        real = real5_.Tick(real);
        real = real6_.Tick(real);
        real = real7_.Tick(real);
        imag = latch_;
        latch_ = imag0_.Tick(x);
        latch_ = imag1_.Tick(latch_);
        latch_ = imag2_.Tick(latch_);
        latch_ = imag3_.Tick(latch_);
        latch_ = imag4_.Tick(latch_);
        latch_ = imag5_.Tick(latch_);
        latch_ = imag6_.Tick(latch_);
        latch_ = imag7_.Tick(latch_);
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

    APF<0.0406273391966415f> real0_;
    APF<0.2984386654059753f> real1_;
    APF<0.5938455547890998f> real2_;
    APF<0.7953345677003365f> real3_;
    APF<0.9040699927853059f> real4_;
    APF<0.9568366727621767f> real5_;
    APF<0.9815966237057977f> real6_;
    APF<0.9938718801312583f> real7_;
    APF<0.1500685240941415f> imag0_;
    APF<0.4538477444783975f> imag1_;
    APF<0.7081016258869689f> imag2_;
    APF<0.8589957406397113f> imag3_;
    APF<0.9353623391637175f> imag4_;
    APF<0.9715130669899118f> imag5_;
    APF<0.9886689766148302f> imag6_;
    APF<0.9980623781456869f> imag7_;
    float latch_{};
};
}