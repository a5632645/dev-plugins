#pragma once
#include <vector>
#include "simd_pack.hpp"

namespace qwqdsp_simd_element {
enum class DelayLineInterp {
    None,
    Lagrange3rd,
    PCHIP,
    Linear,
};

template<size_t N, DelayLineInterp kInterpType>
class DelayLine {
public:
    void Init(float max_ms, float fs) {
        float d = max_ms * fs / 1000.0f;
        size_t i = static_cast<size_t>(d);
        Init(i);
    }

    void Init(size_t max_samples) {
        size_t a = 1;
        while (a < max_samples) {
            a *= 2;
        }
        if (buffer_.size() < a) {
            buffer_.resize(a);
        }
        mask_ = static_cast<int>(a - 1);
    }

    void Reset() noexcept {
        wpos_ = 0;
        std::fill(buffer_.begin(), buffer_.end(), PackFloat<N>{});
    }

    void Push(PackFloatCRef<N> x) noexcept {
        buffer_[static_cast<size_t>(wpos_++)] = x;
        wpos_ &= mask_;
    }

    PackFloat<N> GetAfterPush(PackFloatCRef<N> delay_samples) noexcept {
        // return Get(delay_samples + 1);
        auto rpos = PackFloat<N>::vBroadcast(static_cast<float>(wpos_ + mask_)) - delay_samples;
        auto irpos = rpos.ToInt();
        auto mask = PackInt32<N>::vBroadcast(mask_);
        irpos &= mask;
        
        if constexpr (kInterpType == DelayLineInterp::None) {
            PackFloat<N> r;
            for (size_t i = 0; i < N; ++i) {
                r.x[i] = buffer_[static_cast<size_t>(irpos.x[i])].x[i];
            }
            return r;
        }
        else if constexpr (kInterpType == DelayLineInterp::Lagrange3rd) {
            auto inext1 = (irpos + 1) & mask;
            auto inext2 = (irpos + 2) & mask;
            auto inext3 = (irpos + 3) & mask;
            auto t = PackOps::Frac(rpos);

            auto d1 = t - 1.0f;
            auto d2 = t - 2.0f;
            auto d3 = t - 3.0f;

            auto c1 = d1 * d2 * d3 / -6.0f;
            auto c2 = d2 * d3 * 0.5f;
            auto c3 = d1 * d3 * (-0.5f);
            auto c4 = d1 * d2 / 6.0f;

            PackFloat<N> y0;
            PackFloat<N> y1;
            PackFloat<N> y2;
            PackFloat<N> y3;
            for (size_t i = 0; i < N; ++i) {
                y0[i] = buffer_[static_cast<size_t>(irpos[i])][i];
                y1[i] = buffer_[static_cast<size_t>(inext1[i])][i];
                y2[i] = buffer_[static_cast<size_t>(inext2[i])][i];
                y3[i] = buffer_[static_cast<size_t>(inext3[i])][i];
            }

            return y0 * c1 + t * (y1 * c2 + y2 * c3 + y3 * c4);
        }
        else if constexpr (kInterpType == DelayLineInterp::Linear) {
            auto inext1 = (irpos + 1) & mask;
            auto t = rpos.Frac();

            PackFloat<N> y0;
            PackFloat<N> y1;
            for (size_t i = 0; i < N; ++i) {
                y0.x[i] = buffer_[irpos.x[i]].x[i];
                y1.x[i] = buffer_[inext1.x[i]].x[i];
            }

            return y0 + t * (y1 - y0);
        }
        else if constexpr (kInterpType == DelayLineInterp::PCHIP) {
            auto iprev1 = (irpos - 1) & mask;
            auto inext1 = (irpos + 1) & mask;
            auto inext2 = (irpos + 2) & mask;
            auto t = PackOps::Frac(rpos);

            PackFloat<N> yn1;
            PackFloat<N> y0;
            PackFloat<N> y1;
            PackFloat<N> y2;
            for (size_t i = 0; i < N; ++i) {
                yn1[i] = buffer_[static_cast<size_t>(iprev1[i])][i];
                y0[i] = buffer_[static_cast<size_t>(irpos[i])][i];
                y1[i] = buffer_[static_cast<size_t>(inext1[i])][i];
                y2[i] = buffer_[static_cast<size_t>(inext2[i])][i];
            }

            PackFloat<N> d0 = (y1 - yn1) * 0.5f;
            PackFloat<N> d1 = (y2 - y0) * 0.5f;
            PackFloat<N> d = y1 - y0;
            PackFloat<N> m0 = 3.0f * d - 2.0f * d0 - d1;
            PackFloat<N> m1 = d0 - 2.0f * d + d1;
            return y0 + t * (
                d0 + t * (
                    m0 + t * m1
                )
            );
        }
    }

    /**
     * @param delay_samples 此处不能小于1，否则为非因果滤波器（或者被绕回读取max_samples处）
     */
    PackFloat<N> GetBeforePush(PackFloatCRef<N> delay_samples) noexcept {
        // return Get(delay_samples);
        PackFloat<N> rpos = PackFloat<N>::vBroadcast(static_cast<float>(wpos_) + static_cast<float>(buffer_.size()) - delay_samples);
        PackInt32<N> irpos = rpos.ToInt();
        PackInt32<N> mask = PackInt32<N>::vBroadcast(mask_);
        irpos &= mask;
        
        if constexpr (kInterpType == DelayLineInterp::None) {
            PackFloat<N> r;
            for (size_t i = 0; i < N; ++i) {
                r.x[i] = buffer_[static_cast<size_t>(irpos.x[i])].x[i];
            }
            return r;
        }
        else if constexpr (kInterpType == DelayLineInterp::Lagrange3rd) {
            auto inext1 = (irpos + 1) & mask;
            auto inext2 = (irpos + 2) & mask;
            auto inext3 = (irpos + 3) & mask;
            auto t = rpos.Frac();

            auto d1 = t - 1.0f;
            auto d2 = t - 2.0f;
            auto d3 = t - 3.0f;

            auto c1 = d1 * d2 * d3 / -6.0f;
            auto c2 = d2 * d3 * 0.5f;
            auto c3 = d1 * d3 * 0.5f;
            auto c4 = d1 * d2 / 6.0f;

            PackFloat<N> y0;
            PackFloat<N> y1;
            PackFloat<N> y2;
            PackFloat<N> y3;
            for (size_t i = 0; i < N; ++i) {
                y0.x[i] = buffer_[irpos.x[i]].x[i];
                y1.x[i] = buffer_[inext1.x[i]].x[i];
                y2.x[i] = buffer_[inext2.x[i]].x[i];
                y3.x[i] = buffer_[inext3.x[i]].x[i];
            }

            return y0 * c1 + t * (y1 * c2 + y2 * c3 + y3 * c4);
        }
        else if constexpr (kInterpType == DelayLineInterp::Linear) {
            auto inext1 = (irpos + 1) & mask;
            auto t = rpos.Frac();

            PackFloat<N> y0;
            PackFloat<N> y1;
            for (size_t i = 0; i < N; ++i) {
                y0.x[i] = buffer_[irpos.x[i]].x[i];
                y1.x[i] = buffer_[inext1.x[i]].x[i];
            }

            return y0 + t * (y1 - y0);
        }
        else if constexpr (kInterpType == DelayLineInterp::PCHIP) {
            auto iprev1 = (irpos - 1) & mask;
            auto inext1 = (irpos + 1) & mask;
            auto inext2 = (irpos + 2) & mask;
            auto t = rpos.Frac();

            PackFloat<N> yn1;
            PackFloat<N> y0;
            PackFloat<N> y1;
            PackFloat<N> y2;
            for (size_t i = 0; i < N; ++i) {
                yn1.x[i] = buffer_[static_cast<size_t>(iprev1.x[i])].x[i];
                y0.x[i] = buffer_[static_cast<size_t>(irpos.x[i])].x[i];
                y1.x[i] = buffer_[static_cast<size_t>(inext1.x[i])].x[i];
                y2.x[i] = buffer_[static_cast<size_t>(inext2.x[i])].x[i];
            }

            auto d0 = (y1 - yn1) * 0.5f;
            auto d1 = (y2 - y0) * 0.5f;
            auto d = y1 - y0;
            auto m0 = 3.0f * d - 2.0f * d0 - d1;
            auto m1 = d0 - 2.0f * d + d1;
            return y0 + t * (
                d0 + t * (
                    m0 + t * m1
                )
            );
        }
    }

    PackFloat<N> GetAfterPush(float delay_samples) noexcept {
        // return Get(delay_samples + 1);
        float rpos = static_cast<float>(wpos_ + mask_) - delay_samples;
        size_t irpos = static_cast<size_t>(rpos) & static_cast<size_t>(mask_);
        
        if constexpr (kInterpType == DelayLineInterp::None) {
            return buffer_[irpos];
        }
        else if constexpr (kInterpType == DelayLineInterp::Lagrange3rd) {
            size_t inext1 = (irpos + 1) & static_cast<size_t>(mask_);
            size_t inext2 = (irpos + 2) & static_cast<size_t>(mask_);
            size_t inext3 = (irpos + 3) & static_cast<size_t>(mask_);
            float t = rpos - static_cast<float>(irpos);

            float d1 = t - 1.0f;
            float d2 = t - 2.0f;
            float d3 = t - 3.0f;

            float c1 = d1 * d2 * d3 / -6.0f;
            float c2 = d2 * d3 * 0.5f;
            float c3 = d1 * d3 * 0.5f;
            float c4 = d1 * d2 / 6.0f;

            auto y0 = buffer_[irpos];
            auto y1 = buffer_[inext1];
            auto y2 = buffer_[inext2];
            auto y3 = buffer_[inext3];

            return y0 * c1 + t * (y1 * c2 + y2 * c3 + y3 * c4);
        }
        else if constexpr (kInterpType == DelayLineInterp::Linear) {
            size_t inext1 = (irpos + 1) & mask_;
            float t = rpos - static_cast<float>(irpos);

            auto y0 = buffer_[irpos];
            auto y1 = buffer_[inext1];

            return y0 + t * (y1 - y0);
        }
        else if constexpr (kInterpType == DelayLineInterp::PCHIP) {
            size_t iprev1 = (irpos - 1) & static_cast<size_t>(mask_);
            size_t inext1 = (irpos + 1) & static_cast<size_t>(mask_);
            size_t inext2 = (irpos + 2) & static_cast<size_t>(mask_);
            float t = rpos - static_cast<float>(irpos);

            auto yn1 = buffer_[iprev1];
            auto y0 = buffer_[irpos];
            auto y1 = buffer_[inext1];
            auto y2 = buffer_[inext2];

            auto d0 = (y1 - yn1) * 0.5f;
            auto d1 = (y2 - y0) * 0.5f;
            auto d = y1 - y0;
            auto m0 = 3.0f * d - 2.0f * d0 - d1;
            auto m1 = d0 - 2.0f * d + d1;
            return y0 + t * (
                d0 + t * (
                    m0 + t * m1
                )
            );
        }
    }

    /**
     * @param delay_samples 此处不能小于1，否则为非因果滤波器（或者被绕回读取max_samples处）
     */
    PackFloat<N> GetBeforePush(float delay_samples) noexcept {
        // return Get(delay_samples);
        float rpos = static_cast<float>(wpos_) + static_cast<float>(buffer_.size()) - delay_samples;
        size_t irpos = static_cast<size_t>(rpos) & static_cast<size_t>(mask_);
        
        if constexpr (kInterpType == DelayLineInterp::None) {
            return buffer_[irpos];
        }
        else if constexpr (kInterpType == DelayLineInterp::Lagrange3rd) {
            size_t inext1 = (irpos + 1) & static_cast<size_t>(mask_);
            size_t inext2 = (irpos + 2) & static_cast<size_t>(mask_);
            size_t inext3 = (irpos + 3) & static_cast<size_t>(mask_);
            float t = rpos - static_cast<float>(irpos);

            float d1 = t - 1.0f;
            float d2 = t - 2.0f;
            float d3 = t - 3.0f;

            float c1 = d1 * d2 * d3 / -6.0f;
            float c2 = d2 * d3 * 0.5f;
            float c3 = d1 * d3 * 0.5f;
            float c4 = d1 * d2 / 6.0f;

            auto y0 = buffer_[irpos];
            auto y1 = buffer_[inext1];
            auto y2 = buffer_[inext2];
            auto y3 = buffer_[inext3];

            return y0 * c1 + t * (y1 * c2 + y2 * c3 + y3 * c4);
        }
        else if constexpr (kInterpType == DelayLineInterp::Linear) {
            size_t inext1 = (irpos + 1) & mask_;
            float t = rpos - static_cast<float>(irpos);

            auto y0 = buffer_[irpos];
            auto y1 = buffer_[inext1];

            return y0 + t * (y1 - y0);
        }
        else if constexpr (kInterpType == DelayLineInterp::PCHIP) {
            size_t iprev1 = (irpos - 1) & static_cast<size_t>(mask_);
            size_t inext1 = (irpos + 1) & static_cast<size_t>(mask_);
            size_t inext2 = (irpos + 2) & static_cast<size_t>(mask_);
            float t = rpos - static_cast<float>(irpos);

            auto yn1 = buffer_[iprev1];
            auto y0 = buffer_[irpos];
            auto y1 = buffer_[inext1];
            auto y2 = buffer_[inext2];

            auto d0 = (y1 - yn1) * 0.5f;
            auto d1 = (y2 - y0) * 0.5f;
            auto d = y1 - y0;
            auto m0 = 3.0f * d - 2.0f * d0 - d1;
            auto m1 = d0 - 2.0f * d + d1;
            return y0 + t * (
                d0 + t * (
                    m0 + t * m1
                )
            );
        }
    }
private:
    std::vector<PackFloat<N>> buffer_;
    int wpos_{};
    int mask_{};
};
}
