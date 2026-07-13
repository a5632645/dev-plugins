#pragma once
#include <complex>
#include <vector>
#include "pluginshared/simd.hpp"
#include "pluginshared/align_allocator.hpp"

namespace dsp::com {
template <simd::IsSimdFloat T>
class ParallelDelayLine {
public:
    struct State {
        T y_re;
        T y_im;
        T y_conj_re;
        T y_conj_im;
    };

    void Init(float max_ms, float fs) {
        float d = max_ms * fs / 1000.0f;
        size_t i = static_cast<size_t>(std::ceil(d));
        Init(i);
    }

    void Init(size_t max_samples) {
        size_t a = 1;
        while (a < max_samples) {
            a *= 2;
        }
        mask_ = static_cast<int>(a - 1);
        delay_length_ = static_cast<int>(a);
        buffer_.resize(a * 2);
    }

    void Reset() noexcept {
        wpos_ = 0;
        std::fill(buffer_.begin(), buffer_.end(), State{});
    }

    State GetBeforePush(float delay_samples) const noexcept {
        float frpos = static_cast<float>(wpos_ + delay_length_) - delay_samples;
        auto t = frpos - std::floor(frpos);
        auto rpos = static_cast<int>(frpos);
        auto irpos = rpos & mask_;
        auto rprev1 = (irpos - 1) & (mask_);
        auto rnext1 = (irpos + 1) & (mask_);
        auto rnext2 = (irpos + 2) & (mask_);

        auto yn1 = buffer_[static_cast<size_t>(rprev1)];
        auto y0 = buffer_[static_cast<size_t>(irpos)];
        auto y1 = buffer_[static_cast<size_t>(rnext1)];
        auto y2 = buffer_[static_cast<size_t>(rnext2)];

        // auto d0 = (y1 - yn1) * 0.5f;
        auto d0_y_re = (y1.y_re - yn1.y_re) * 0.5f;
        auto d0_y_im = (y1.y_im - yn1.y_im) * 0.5f;
        auto d0_y_conj_re = (y1.y_conj_re - yn1.y_conj_re) * 0.5f;
        auto d0_y_conj_im = (y1.y_conj_im - yn1.y_conj_im) * 0.5f;
        // auto d1 = (y2 - y0) * 0.5f;
        auto d1_y_re = (y2.y_re - y0.y_re) * 0.5f;
        auto d1_y_im = (y2.y_im - y0.y_im) * 0.5f;
        auto d1_y_conj_re = (y2.y_conj_re - y0.y_conj_re) * 0.5f;
        auto d1_y_conj_im = (y2.y_conj_im - y0.y_conj_im) * 0.5f;
        // auto d = y1 - y0;
        auto d_y_re = y1.y_re - y0.y_re;
        auto d_y_im = y1.y_im - y0.y_im;
        auto d_y_conj_re = y1.y_conj_re - y0.y_conj_re;
        auto d_y_conj_im = y1.y_conj_im - y0.y_conj_im;
        // auto m0 = 3.0f * d - 2.0f * d0 - d1;
        auto m0_y_re = 3.0f * d_y_re - 2.0f * d0_y_re - d1_y_re;
        auto m0_y_im = 3.0f * d_y_im - 2.0f * d0_y_im - d1_y_im;
        auto m0_y_conj_re = 3.0f * d_y_conj_re - 2.0f * d0_y_conj_re - d1_y_conj_re;
        auto m0_y_conj_im = 3.0f * d_y_conj_im - 2.0f * d0_y_conj_im - d1_y_conj_im;
        // auto m1 = d0 - 2.0f * d + d1;
        auto m1_y_re = d0_y_re - 2.0f * d_y_re + d1_y_re;
        auto m1_y_im = d0_y_im - 2.0f * d_y_im + d1_y_im;
        auto m1_y_conj_re = d0_y_conj_re - 2.0f * d_y_conj_re + d1_y_conj_re;
        auto m1_y_conj_im = d0_y_conj_im - 2.0f * d_y_conj_im + d1_y_conj_im;
        // return y0 + t * (d0 + t * (m0 + t * m1));
        auto y0_y_re = y0.y_re + t * (d0_y_re + t * (m0_y_re + t * m1_y_re));
        auto y0_y_im = y0.y_im + t * (d0_y_im + t * (m0_y_im + t * m1_y_im));
        auto y0_y_conj_re = y0.y_conj_re + t * (d0_y_conj_re + t * (m0_y_conj_re + t * m1_y_conj_re));
        auto y0_y_conj_im = y0.y_conj_im + t * (d0_y_conj_im + t * (m0_y_conj_im + t * m1_y_conj_im));
        return State{y0_y_re, y0_y_im, y0_y_conj_re, y0_y_conj_im};
    }

    void Push(State x) noexcept {
        wpos_ = (wpos_ + 1) & mask_;
        buffer_[static_cast<size_t>(wpos_)] = x;
        buffer_[static_cast<size_t>(wpos_ + delay_length_)] = x;
    }
private:
    std::vector<State, simd::AlignedAllocator<State, alignof(State)>> buffer_;
    int delay_length_{};
    int wpos_{};
    int mask_{};
};

template <simd::IsSimdFloat T>
class IirNFilter {
public:
    void Init(float fs, float max_ms) {
        delay_l_.Init(max_ms, fs);
        delay_r_.Init(max_ms, fs);
    }

    void Reset() noexcept {
        delay_l_.Reset();
        delay_r_.Reset();
    }

    std::array<float, 2> Tick(float left, float right, float delay_l, float delay_r) noexcept {
        auto s_l = delay_l_.GetBeforePush(delay_l);
        auto s_r = delay_r_.GetBeforePush(delay_r);

        // auto y = s.y * pole + s.x * residual;
        simd::SimdComplex<T> y_l = simd::SimdComplex<T>{s_l.y_re, s_l.y_im} * pole_ + left * residual_;
        simd::SimdComplex<T> y_r = simd::SimdComplex<T>{s_r.y_re, s_r.y_im} * pole_ + right * residual_;

        delay_l_.Push(typename decltype(delay_l_)::State{y_l.re, y_l.im, T{}, T{}});
        delay_r_.Push(typename decltype(delay_r_)::State{y_r.re, y_r.im, T{}, T{}});
        return {simd::ReduceAdd(y_l.re), simd::ReduceAdd(y_r.re)};
    }

    std::array<std::complex<float>, 2> TickCpx(float left, float right, float delay_l, float delay_r,
                                               std::complex<float> zrotate_l, std::complex<float> zrotate_r) noexcept {
        auto s_l = delay_l_.GetBeforePush(delay_l);
        auto s_r = delay_r_.GetBeforePush(delay_r);

        // auto y = s.y * pole + s.x * residual;
        simd::SimdComplex<T> y_l = simd::SimdComplex<T>{s_l.y_re, s_l.y_im} * pole_ + left * residual_;
        simd::SimdComplex<T> y_r = simd::SimdComplex<T>{s_r.y_re, s_r.y_im} * pole_ + right * residual_;
        y_l = zrotate_l * y_l;
        y_r = zrotate_r * y_r;
        simd::SimdComplex<T> y_l_conj =
            simd::SimdComplex<T>{s_l.y_conj_re, s_l.y_conj_im} * pole_.conj() + left * residual_.conj();
        simd::SimdComplex<T> y_r_conj =
            simd::SimdComplex<T>{s_r.y_conj_re, s_r.y_conj_im} * pole_.conj() + right * residual_.conj();
        y_l_conj = zrotate_l * y_l_conj;
        y_r_conj = zrotate_r * y_r_conj;

        delay_l_.Push(typename decltype(delay_l_)::State{y_l.re, y_l.im, y_l_conj.re, y_l_conj.im});
        delay_r_.Push(typename decltype(delay_r_)::State{y_r.re, y_r.im, y_r_conj.re, y_r_conj.im});
        return {(y_l + y_l_conj).ReduceAdd(), (y_r + y_r_conj).ReduceAdd()};
    }

    void Set(simd::SimdComplex<T> residual, simd::SimdComplex<T> pole) noexcept {
        pole_ = pole;
        residual_ = residual;
    }
private:
    ParallelDelayLine<T> delay_l_;
    ParallelDelayLine<T> delay_r_;
    simd::SimdComplex<T> pole_;
    simd::SimdComplex<T> residual_;
};
} // namespace dsp::com
