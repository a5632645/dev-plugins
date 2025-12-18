#pragma once
#include "simd_pack.hpp"

namespace qwqdsp_simd_element {

template<size_t N>
class StereoIIRHilbertCpx {
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
        lag_.Broadcast(0);
    }

    /**
     * @param x => [L_re, L_im, R_re, R_im]
     * @return  => [L_re, L_im, R_re, R_im]
     */
    PackFloat<N> Tick(PackFloat<N> const& x) noexcept {
        PackFloat<N> real{};
        PackFloat<N> imag{};
        real = real0_.Tick(x);
        real = real1_.Tick(real);
        real = real2_.Tick(real);
        real = real3_.Tick(real);
        imag = imag0_.Tick(x);
        imag = imag1_.Tick(imag);
        imag = imag2_.Tick(imag);
        imag = imag3_.Tick(imag);
        // delay the imag one sample
        // multiply imag with j then add with real
        PackFloat<N> mulj;
        QWQDSP_AUTO_VECTORLIZE
        for (size_t i = 0; i < N; i += 4) {
            mulj[i] = -lag_[i + 1];
            mulj[i + 1] = lag_[i];
            mulj[i + 2] = -lag_[i + 3];
            mulj[i + 3] = lag_[i + 2];
        }
        lag_ = imag;
        return real + mulj;
    }
private:
    template<float alpha>
    struct APF {
        PackFloat<N> z0_{};
        PackFloat<N> z1_{};

        void Reset() noexcept {
            z0_.Broadcast(0);
            z1_.Broadcast(0);
        }

        /**
         * @param x => [L_re, L_im, R_re, R_im]
         * @return  => [L_re, L_im, R_re, R_im]
        */
        PackFloat<N> Tick(PackFloat<N> const& x) noexcept {
            auto in = x + alpha * z1_;
            auto out = z1_ - alpha * in;
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
    qwqdsp_simd_element::PackFloat<N> lag_{};
};

template<size_t N>
class StereoIIRHilbertDeeperCpx {
public:
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
        lag_.Broadcast(0);
    }

    /**
     * @param x => [L_re, L_im, R_re, R_im] or 复制2次 or 复制4次
     * @return  => [L_re, L_im, R_re, R_im] 同上
     */
    PackFloat<N> Tick(PackFloat<N> const& x) noexcept {
        PackFloat<N> real{};
        PackFloat<N> imag{};
        real = real0_.Tick(x);
        real = real1_.Tick(real);
        real = real2_.Tick(real);
        real = real3_.Tick(real);
        real = real4_.Tick(real);
        real = real5_.Tick(real);
        real = real6_.Tick(real);
        real = real7_.Tick(real);
        imag = imag0_.Tick(x);
        imag = imag1_.Tick(imag);
        imag = imag2_.Tick(imag);
        imag = imag3_.Tick(imag);
        imag = imag4_.Tick(imag);
        imag = imag5_.Tick(imag);
        imag = imag6_.Tick(imag);
        imag = imag7_.Tick(imag);
        // delay the imag one sample
        // multiply imag with j then add with real
        PackFloat<N> mulj;
        QWQDSP_AUTO_VECTORLIZE
        for (size_t i = 0; i < N; i += 4) {
            mulj[i] = -lag_[i + 1];
            mulj[i + 1] = lag_[i];
            mulj[i + 2] = -lag_[i + 3];
            mulj[i + 3] = lag_[i + 2];
        }
        lag_ = imag;
        return real + mulj;
    }
private:
    template<float alpha>
    struct APF {
        PackFloat<N> z0_{};
        PackFloat<N> z1_{};

        void Reset() noexcept {
            z0_.Broadcast(0);
            z1_.Broadcast(0);
        }

        /**
         * @param x => [L_re, L_im, R_re, R_im]
         * @return  => [L_re, L_im, R_re, R_im]
        */
        PackFloat<N> Tick(PackFloat<N> const& x) noexcept {
            auto in = x + alpha * z1_;
            auto out = z1_ - alpha * in;
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
    PackFloat<N> lag_{};
};

} // namespace qwqdsp_simd_element
