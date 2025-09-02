#pragma once
#include <complex>

namespace qwqdsp::filter {
template<class T = float>
class IIRHilbertCpx {
public:
    void Reset() noexcept {
        real0_.Reset();
        real1_.Reset();
        real2_.Reset();
        real3_.Reset();
        imag0_.Reset();
        imag1_.Reset();
        imag2_.Reset();
        imag3_.Reset();
        latch_ = 0;
    }

    using TCpx = std::complex<T>;
    TCpx Tick(TCpx x) noexcept {
        TCpx real{};
        TCpx imag{};
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
    template<T alpha>
    struct APF {
        TCpx z0_{};
        TCpx z1_{};

        void Reset() noexcept {
            z0_ = TCpx{};
            z1_ = TCpx{};
        }

        TCpx Tick(TCpx x) noexcept {
            T in = x + alpha * z1_;
            T out = -alpha * in + z1_;
            z1_ = z0_;
            z0_ = in;
            return out;
        }
    };

    APF<T(0.4021921162426)> real0_;
    APF<T(0.8561710882420)> real1_;
    APF<T(0.9722909545651)> real2_;
    APF<T(0.9952884791278)> real3_;
    APF<T(0.6923878)> imag0_;
    APF<T(0.9360654322959)> imag1_;
    APF<T(0.9882295226860)> imag2_;
    APF<T(0.9987488452737)> imag3_;
    TCpx latch_{};
};

template<class T = float>
class IIRHilbertDeeperCpx {
public:
    using TCpx = std::complex<T>;

    void Reset() noexcept {
        real0_.Reset();
        real1_.Reset();
        real2_.Reset();
        real3_.Reset();
        real4_.Reset();
        real5_.Reset();
        real6_.Reset();
        real7_.Reset();
        imag0_.Reset();
        imag1_.Reset();
        imag2_.Reset();
        imag3_.Reset();
        imag4_.Reset();
        imag5_.Reset();
        imag6_.Reset();
        imag7_.Reset();
        latch_ = 0;
    }

    TCpx Tick(TCpx x) noexcept {
        TCpx real{};
        TCpx imag{};
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
    template<T alpha>
    struct APF {
        TCpx z0_{};
        TCpx z1_{};

        void Reset() noexcept {
            z0_ = TCpx{};
            z1_ = TCpx{};
        }

        TCpx Tick(TCpx x) noexcept {
            TCpx in = x + alpha * z1_;
            TCpx out = -alpha * in + z1_;
            z1_ = z0_;
            z0_ = in;
            return out;
        }
    };

    APF<T(0.0406273391966415)> real0_;
    APF<T(0.2984386654059753)> real1_;
    APF<T(0.5938455547890998)> real2_;
    APF<T(0.7953345677003365)> real3_;
    APF<T(0.9040699927853059)> real4_;
    APF<T(0.9568366727621767)> real5_;
    APF<T(0.9815966237057977)> real6_;
    APF<T(0.9938718801312583)> real7_;
    APF<T(0.1500685240941415)> imag0_;
    APF<T(0.4538477444783975)> imag1_;
    APF<T(0.7081016258869689)> imag2_;
    APF<T(0.8589957406397113)> imag3_;
    APF<T(0.9353623391637175)> imag4_;
    APF<T(0.9715130669899118)> imag5_;
    APF<T(0.9886689766148302)> imag6_;
    APF<T(0.9980623781456869)> imag7_;
    TCpx latch_{};
};
}