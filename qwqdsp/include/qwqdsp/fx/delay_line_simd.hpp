#pragma once
#include <vector>

namespace qwqdsp::fx {
enum class DelayLineInterpSIMD {
    None,
    Lagrange3rd,
    PCHIP,
    Linear,
};
template<class SIMD_TYPE, DelayLineInterpSIMD kInterpType>
class DelayLineSIMD {
public:
    using SIMD_INT_TYPE = typename SIMD_TYPE::IntType;

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
        std::fill(buffer_.begin(), buffer_.end(), SIMD_TYPE{});
    }

    void Push(SIMD_TYPE x) noexcept {
        buffer_[static_cast<size_t>(wpos_++)] = x;
        wpos_ &= mask_;
    }

    template<size_t kAcitveScalars = SIMD_TYPE::kSize>
    SIMD_TYPE GetAfterPush(SIMD_TYPE delay_samples) noexcept {
        // return Get(delay_samples + 1);
        SIMD_TYPE rpos = SIMD_TYPE::FromSingle(static_cast<float>(wpos_ + mask_)) - delay_samples;
        SIMD_INT_TYPE irpos = rpos.ToInt();
        SIMD_INT_TYPE mask = SIMD_INT_TYPE::FromSingle(mask_);
        irpos &= mask;
        
        if constexpr (kInterpType == DelayLineInterpSIMD::None) {
            SIMD_TYPE r;
            for (size_t i = 0; i < kAcitveScalars; ++i) {
                r.x[i] = buffer_[static_cast<size_t>(irpos.x[i])].x[i];
            }
            return r;
        }
        else if constexpr (kInterpType == DelayLineInterpSIMD::Lagrange3rd) {
            SIMD_INT_TYPE inext1 = (irpos + SIMD_INT_TYPE::FromSingle(1)) & mask;
            SIMD_INT_TYPE inext2 = (irpos + SIMD_INT_TYPE::FromSingle(2)) & mask;
            SIMD_INT_TYPE inext3 = (irpos + SIMD_INT_TYPE::FromSingle(3)) & mask;
            SIMD_TYPE t = rpos.Frac();

            SIMD_TYPE d1 = t - SIMD_TYPE::FromSingle(1.0f);
            SIMD_TYPE d2 = t - SIMD_TYPE::FromSingle(2.0f);
            SIMD_TYPE d3 = t - SIMD_TYPE::FromSingle(3.0f);

            SIMD_TYPE c1 = d1 * d2 * d3 / SIMD_TYPE::FromSingle(-6.0f);
            SIMD_TYPE c2 = d2 * d3 * SIMD_TYPE::FromSingle(0.5f);
            SIMD_TYPE c3 = d1 * d3 * SIMD_TYPE::FromSingle(0.5f);
            SIMD_TYPE c4 = d1 * d2 / SIMD_TYPE::FromSingle(6.0f);

            SIMD_TYPE y0;
            SIMD_TYPE y1;
            SIMD_TYPE y2;
            SIMD_TYPE y3;
            for (size_t i = 0; i < kAcitveScalars; ++i) {
                y0.x[i] = buffer_[irpos.x[i]].x[i];
                y1.x[i] = buffer_[inext1.x[i]].x[i];
                y2.x[i] = buffer_[inext2.x[i]].x[i];
                y3.x[i] = buffer_[inext3.x[i]].x[i];
            }

            return y0 * c1 + t * (y1 * c2 + y2 * c3 + y3 * c4);
        }
        else if constexpr (kInterpType == DelayLineInterpSIMD::Linear) {
            SIMD_INT_TYPE inext1 = (irpos + SIMD_INT_TYPE::FromSingle(1)) & mask;
            SIMD_TYPE t = rpos.Frac();

            SIMD_TYPE y0;
            SIMD_TYPE y1;
            for (size_t i = 0; i < kAcitveScalars; ++i) {
                y0.x[i] = buffer_[irpos.x[i]].x[i];
                y1.x[i] = buffer_[inext1.x[i]].x[i];
            }

            return y0 + t * (y1 - y0);
        }
        else if constexpr (kInterpType == DelayLineInterpSIMD::PCHIP) {
            SIMD_INT_TYPE iprev1 = (irpos - SIMD_INT_TYPE::FromSingle(1)) & mask;
            SIMD_INT_TYPE inext1 = (irpos + SIMD_INT_TYPE::FromSingle(1)) & mask;
            SIMD_INT_TYPE inext2 = (irpos + SIMD_INT_TYPE::FromSingle(2)) & mask;
            SIMD_TYPE t = rpos.Frac();

            SIMD_TYPE yn1;
            SIMD_TYPE y0;
            SIMD_TYPE y1;
            SIMD_TYPE y2;
            for (size_t i = 0; i < kAcitveScalars; ++i) {
                yn1.x[i] = buffer_[static_cast<size_t>(iprev1.x[i])].x[i];
                y0.x[i] = buffer_[static_cast<size_t>(irpos.x[i])].x[i];
                y1.x[i] = buffer_[static_cast<size_t>(inext1.x[i])].x[i];
                y2.x[i] = buffer_[static_cast<size_t>(inext2.x[i])].x[i];
            }

            SIMD_TYPE d0 = (y1 - yn1) * SIMD_TYPE::FromSingle(0.5f);
            SIMD_TYPE d1 = (y2 - y0) * SIMD_TYPE::FromSingle(0.5f);
            SIMD_TYPE d = y1 - y0;
            SIMD_TYPE m0 = SIMD_TYPE::FromSingle(3.0f) * d - SIMD_TYPE::FromSingle(2.0f) * d0 - d1;
            SIMD_TYPE m1 = d0 - SIMD_TYPE::FromSingle(2.0f) * d + d1;
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
    template<size_t kAcitveScalars = SIMD_TYPE::kSize>
    SIMD_TYPE GetBeforePush(SIMD_TYPE delay_samples) noexcept {
        // return Get(delay_samples);
        SIMD_TYPE rpos = SIMD_TYPE::FromSingle(static_cast<float>(wpos_) + static_cast<float>(buffer_.size())) - delay_samples;
        SIMD_INT_TYPE irpos = rpos.ToInt();
        SIMD_INT_TYPE mask = SIMD_INT_TYPE::FromSingle(mask_);
        irpos &= mask;
        
        if constexpr (kInterpType == DelayLineInterpSIMD::None) {
            SIMD_TYPE r;
            for (size_t i = 0; i < kAcitveScalars; ++i) {
                r.x[i] = buffer_[static_cast<size_t>(irpos.x[i])].x[i];
            }
            return r;
        }
        else if constexpr (kInterpType == DelayLineInterpSIMD::Lagrange3rd) {
            SIMD_INT_TYPE inext1 = (irpos + SIMD_INT_TYPE::FromSingle(1)) & mask;
            SIMD_INT_TYPE inext2 = (irpos + SIMD_INT_TYPE::FromSingle(2)) & mask;
            SIMD_INT_TYPE inext3 = (irpos + SIMD_INT_TYPE::FromSingle(3)) & mask;
            SIMD_TYPE t = rpos.Frac();

            SIMD_TYPE d1 = t - SIMD_TYPE::FromSingle(1.0f);
            SIMD_TYPE d2 = t - SIMD_TYPE::FromSingle(2.0f);
            SIMD_TYPE d3 = t - SIMD_TYPE::FromSingle(3.0f);

            SIMD_TYPE c1 = d1 * d2 * d3 / SIMD_TYPE::FromSingle(-6.0f);
            SIMD_TYPE c2 = d2 * d3 * SIMD_TYPE::FromSingle(0.5f);
            SIMD_TYPE c3 = d1 * d3 * SIMD_TYPE::FromSingle(0.5f);
            SIMD_TYPE c4 = d1 * d2 / SIMD_TYPE::FromSingle(6.0f);

            SIMD_TYPE y0;
            SIMD_TYPE y1;
            SIMD_TYPE y2;
            SIMD_TYPE y3;
            for (size_t i = 0; i < kAcitveScalars; ++i) {
                y0.x[i] = buffer_[irpos.x[i]].x[i];
                y1.x[i] = buffer_[inext1.x[i]].x[i];
                y2.x[i] = buffer_[inext2.x[i]].x[i];
                y3.x[i] = buffer_[inext3.x[i]].x[i];
            }

            return y0 * c1 + t * (y1 * c2 + y2 * c3 + y3 * c4);
        }
        else if constexpr (kInterpType == DelayLineInterpSIMD::Linear) {
            SIMD_INT_TYPE inext1 = (irpos + SIMD_INT_TYPE::FromSingle(1)) & mask;
            SIMD_TYPE t = rpos.Frac();

            SIMD_TYPE y0;
            SIMD_TYPE y1;
            for (size_t i = 0; i < kAcitveScalars; ++i) {
                y0.x[i] = buffer_[irpos.x[i]].x[i];
                y1.x[i] = buffer_[inext1.x[i]].x[i];
            }

            return y0 + t * (y1 - y0);
        }
        else if constexpr (kInterpType == DelayLineInterpSIMD::PCHIP) {
            SIMD_INT_TYPE iprev1 = (irpos - SIMD_INT_TYPE::FromSingle(1)) & mask;
            SIMD_INT_TYPE inext1 = (irpos + SIMD_INT_TYPE::FromSingle(1)) & mask;
            SIMD_INT_TYPE inext2 = (irpos + SIMD_INT_TYPE::FromSingle(2)) & mask;
            SIMD_TYPE t = rpos.Frac();

            SIMD_TYPE yn1;
            SIMD_TYPE y0;
            SIMD_TYPE y1;
            SIMD_TYPE y2;
            for (size_t i = 0; i < kAcitveScalars; ++i) {
                yn1.x[i] = buffer_[static_cast<size_t>(iprev1.x[i])].x[i];
                y0.x[i] = buffer_[static_cast<size_t>(irpos.x[i])].x[i];
                y1.x[i] = buffer_[static_cast<size_t>(inext1.x[i])].x[i];
                y2.x[i] = buffer_[static_cast<size_t>(inext2.x[i])].x[i];
            }

            SIMD_TYPE d0 = (y1 - yn1) * SIMD_TYPE::FromSingle(0.5f);
            SIMD_TYPE d1 = (y2 - y0) * SIMD_TYPE::FromSingle(0.5f);
            SIMD_TYPE d = y1 - y0;
            SIMD_TYPE m0 = SIMD_TYPE::FromSingle(3.0f) * d - SIMD_TYPE::FromSingle(2.0f) * d0 - d1;
            SIMD_TYPE m1 = d0 - SIMD_TYPE::FromSingle(2.0f) * d + d1;
            return y0 + t * (
                d0 + t * (
                    m0 + t * m1
                )
            );
        }
    }

    SIMD_TYPE GetAfterPush(float delay_samples) noexcept {
        // return Get(delay_samples + 1);
        float rpos = static_cast<float>(wpos_ + mask_) - delay_samples;
        size_t irpos = static_cast<size_t>(rpos) & static_cast<size_t>(mask_);
        
        if constexpr (kInterpType == DelayLineInterpSIMD::None) {
            return buffer_[irpos];
        }
        else if constexpr (kInterpType == DelayLineInterpSIMD::Lagrange3rd) {
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

            SIMD_TYPE y0 = buffer_[irpos];
            SIMD_TYPE y1 = buffer_[inext1];
            SIMD_TYPE y2 = buffer_[inext2];
            SIMD_TYPE y3 = buffer_[inext3];

            return y0 * SIMD_TYPE::FromSingle(c1) + SIMD_TYPE::FromSingle(t) * (y1 * SIMD_TYPE::FromSingle(c2) + y2 * SIMD_TYPE::FromSingle(c3) + y3 * SIMD_TYPE::FromSingle(c4));
        }
        else if constexpr (kInterpType == DelayLineInterpSIMD::Linear) {
            size_t inext1 = (irpos + 1) & mask_;
            float t = rpos - static_cast<float>(irpos);

            SIMD_TYPE y0 = buffer_[irpos];
            SIMD_TYPE y1 = buffer_[inext1];

            return y0 + SIMD_TYPE::FromSingle(t) * (y1 - y0);
        }
        else if constexpr (kInterpType == DelayLineInterpSIMD::PCHIP) {
            size_t iprev1 = (irpos - 1) & static_cast<size_t>(mask_);
            size_t inext1 = (irpos + 1) & static_cast<size_t>(mask_);
            size_t inext2 = (irpos + 2) & static_cast<size_t>(mask_);
            SIMD_TYPE t = SIMD_TYPE::FromSingle(rpos).Frac();

            SIMD_TYPE yn1 = buffer_[iprev1];
            SIMD_TYPE y0 = buffer_[irpos];
            SIMD_TYPE y1 = buffer_[inext1];
            SIMD_TYPE y2 = buffer_[inext2];

            SIMD_TYPE d0 = (y1 - yn1) * SIMD_TYPE::FromSingle(0.5f);
            SIMD_TYPE d1 = (y2 - y0) * SIMD_TYPE::FromSingle(0.5f);
            SIMD_TYPE d = y1 - y0;
            SIMD_TYPE m0 = SIMD_TYPE::FromSingle(3.0f) * d - SIMD_TYPE::FromSingle(2.0f) * d0 - d1;
            SIMD_TYPE m1 = d0 - SIMD_TYPE::FromSingle(2.0f) * d + d1;
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
    SIMD_TYPE GetBeforePush(float delay_samples) noexcept {
        // return Get(delay_samples);
        float rpos = static_cast<float>(wpos_) + static_cast<float>(buffer_.size()) - delay_samples;
        size_t irpos = static_cast<size_t>(rpos) & static_cast<size_t>(mask_);
        
        if constexpr (kInterpType == DelayLineInterpSIMD::None) {
            return buffer_[irpos];
        }
        else if constexpr (kInterpType == DelayLineInterpSIMD::Lagrange3rd) {
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

            SIMD_TYPE y0 = buffer_[irpos];
            SIMD_TYPE y1 = buffer_[inext1];
            SIMD_TYPE y2 = buffer_[inext2];
            SIMD_TYPE y3 = buffer_[inext3];

            return y0 * SIMD_TYPE::FromSingle(c1) + SIMD_TYPE::FromSingle(t) * (y1 * SIMD_TYPE::FromSingle(c2) + y2 * SIMD_TYPE::FromSingle(c3) + y3 * SIMD_TYPE::FromSingle(c4));
        }
        else if constexpr (kInterpType == DelayLineInterpSIMD::Linear) {
            size_t inext1 = (irpos + 1) & mask_;
            float t = rpos - static_cast<float>(irpos);

            SIMD_TYPE y0 = buffer_[irpos];
            SIMD_TYPE y1 = buffer_[inext1];

            return y0 + SIMD_TYPE::FromSingle(t) * (y1 - y0);
        }
        else if constexpr (kInterpType == DelayLineInterpSIMD::PCHIP) {
            size_t iprev1 = (irpos - 1) & static_cast<size_t>(mask_);
            size_t inext1 = (irpos + 1) & static_cast<size_t>(mask_);
            size_t inext2 = (irpos + 2) & static_cast<size_t>(mask_);
            SIMD_TYPE t = SIMD_TYPE::FromSingle(rpos).Frac();

            SIMD_TYPE yn1 = buffer_[iprev1];
            SIMD_TYPE y0 = buffer_[irpos];
            SIMD_TYPE y1 = buffer_[inext1];
            SIMD_TYPE y2 = buffer_[inext2];

            SIMD_TYPE d0 = (y1 - yn1) * SIMD_TYPE::FromSingle(0.5f);
            SIMD_TYPE d1 = (y2 - y0) * SIMD_TYPE::FromSingle(0.5f);
            SIMD_TYPE d = y1 - y0;
            SIMD_TYPE m0 = SIMD_TYPE::FromSingle(3.0f) * d - SIMD_TYPE::FromSingle(2.0f) * d0 - d1;
            SIMD_TYPE m1 = d0 - SIMD_TYPE::FromSingle(2.0f) * d + d1;
            return y0 + t * (
                d0 + t * (
                    m0 + t * m1
                )
            );
        }
    }
private:
    std::vector<SIMD_TYPE> buffer_;
    int wpos_{};
    int mask_{};
};
}