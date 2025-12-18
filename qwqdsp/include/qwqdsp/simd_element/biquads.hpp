#pragma once
#include "qwqdsp/simd_element/simd_pack.hpp"
#include "qwqdsp/filter/biquad_coeff.hpp"

namespace qwqdsp_simd_element {

template<size_t N>
class Biquads {
public:
    void Reset() noexcept {
        s1_.broadcast(0);
        s2_.broadcast(0);
    }

    PackFloat<N> TickMultiChannel(PackFloat<N> const& x) noexcept {
        PackFloat<N> y = x * b0_ + s1_;
        s1_ = x * b1_ - y * a1_ + s2_;
        s2_ = x * b2_ - y * a2_;
        return y;
    }

    float TickCascade(float x) noexcept {
        PackFloat<N> vx;
        PackFloat<N> vy;
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) {
            vx[i] = x;
            x = x * b0_[i] + s1_[i];
            vy[i] = x;
        }
        s1_ = vx * b1_ - vy * a1_ + s2_;
        s2_ = vx * b2_ - vy * a2_;
        return x;
    }

    void Set(size_t ch, qwqdsp_filter::BiquadCoeff const& c) noexcept {
        b0_[ch] = c.b0;
        b1_[ch] = c.b1;
        b2_[ch] = c.b2;
        a1_[ch] = c.a1;
        a2_[ch] = c.a2;
    }

    void SetAll(qwqdsp_filter::BiquadCoeff const& c) noexcept {
        b0_.Broadcast(c.b0);
        b1_.Broadcast(c.b1);
        b2_.Broadcast(c.b2);
        a1_.Broadcast(c.a1);
        a2_.Broadcast(c.a2);
    }
private:
    PackFloat<N> b0_{};
    PackFloat<N> b1_{};
    PackFloat<N> b2_{};
    PackFloat<N> a1_{};
    PackFloat<N> a2_{};
    PackFloat<N> s1_{};
    PackFloat<N> s2_{};
};

} // namespace qwqdsp_simd_element
