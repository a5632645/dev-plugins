#pragma once
#include <complex>

namespace qwqdsp {
template<class TT = float>
class IIRHilbertCpx {
public:
    using T = std::complex<TT>;
    std::complex<TT> Tick(T x) {
        T real{};
        T imag{};
        real = real0_.Tick(x);
        real = real1_.Tick(real);
        real = real2_.Tick(real);
        real = real3_.Tick(real);
        imag = latch_;
        latch_ = imag0_.Tick(x);
        latch_ = imag1_.Tick(latch_);
        latch_ = imag2_.Tick(latch_);
        latch_ = imag3_.Tick(latch_);
        return {real.real() - imag.imag(), real.imag() + imag.real()};
    }
private:
    template<TT alpha>
    struct APF {
        T z0_{};
        T z1_{};
        T Tick(T x) {
            T in = x + alpha * z1_;
            T out = -alpha * in + z1_;
            z1_ = z0_;
            z0_ = in;
            return out;
        }
    };

    APF<TT(0.4021921162426)> real0_;
    APF<TT(0.8561710882420)> real1_;
    APF<TT(0.9722909545651)> real2_;
    APF<TT(0.9952884791278)> real3_;
    APF<TT(0.6923878)> imag0_;
    APF<TT(0.9360654322959)> imag1_;
    APF<TT(0.9882295226860)> imag2_;
    APF<TT(0.9987488452737)> imag3_;
    T latch_{};
};

template<class TT = float>
class IIRHilbertDeeperCpx {
public:
    using T = std::complex<TT>;
    std::complex<TT> Tick(T x) {
        T real{};
        T imag{};
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
        return {real.real() - imag.imag(), real.imag() + imag.real()};
    }
private:
    template<TT alpha>
    struct APF {
        T z0_{};
        T z1_{};
        T Tick(T x) {
            T in = x + alpha * z1_;
            T out = -alpha * in + z1_;
            z1_ = z0_;
            z0_ = in;
            return out;
        }
    };

    APF<TT(0.0406273391966415)> real0_;
    APF<TT(0.2984386654059753)> real1_;
    APF<TT(0.5938455547890998)> real2_;
    APF<TT(0.7953345677003365)> real3_;
    APF<TT(0.9040699927853059)> real4_;
    APF<TT(0.9568366727621767)> real5_;
    APF<TT(0.9815966237057977)> real6_;
    APF<TT(0.9938718801312583)> real7_;
    APF<TT(0.1500685240941415)> imag0_;
    APF<TT(0.4538477444783975)> imag1_;
    APF<TT(0.7081016258869689)> imag2_;
    APF<TT(0.8589957406397113)> imag3_;
    APF<TT(0.9353623391637175)> imag4_;
    APF<TT(0.9715130669899118)> imag5_;
    APF<TT(0.9886689766148302)> imag6_;
    APF<TT(0.9980623781456869)> imag7_;
    T latch_{};
};
}