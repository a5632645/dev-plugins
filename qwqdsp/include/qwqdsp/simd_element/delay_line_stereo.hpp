#pragma once
#include <vector>
#include <array>
#include "simd_pack.hpp"

namespace qwqdsp_simd_element {
template<size_t N, bool kFastTrick>
class DelayLineStereo {
public:
    static_assert(N == 4 || N == 8, "unsupport channels");

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
        size_ = static_cast<uint32_t>(a);
        mask_ = static_cast<uint32_t>(a - 1);
        if constexpr (kFastTrick) {
            buffer_.resize((size_ + 4) * 2);
            ptrs_[0] = buffer_.data();
            ptrs_[1] = buffer_.data() + (size_ + 4);
        }
        else {
            buffer_.resize((size_ * 2) * 2);
            ptrs_[0] = buffer_.data();
            ptrs_[1] = buffer_.data() + (size_ * 2);
        }
    }

    void Reset() noexcept {
        wpos_ = 0;
        std::fill(buffer_.begin(), buffer_.end(), float{});
    }

    void WarpBuffer() noexcept {
        if constexpr (kFastTrick) {
            for (auto ptr : ptrs_) {
                PackFloat<4> t;
                t.Load(ptr + 0);
                t.Store(ptr + size_);
            }
        }
    }

    void Push(float left, float right) noexcept {
        wpos_ = (wpos_ + 1) & mask_;
        if constexpr (kFastTrick) {
            ptrs_[0][wpos_] = left;
            ptrs_[1][wpos_] = right;
        }
        else {
            ptrs_[0][wpos_] = left;
            ptrs_[0][wpos_ + size_] = left;
            ptrs_[1][wpos_] = right;
            ptrs_[1][wpos_ + size_] = right;
        }
    }

    std::pair<PackFloat<N>, PackFloat<N>> GetAfterPush(
        PackFloat<N> const& left_delay_samples,
        PackFloat<N> const& right_delay_samples
    ) noexcept {
        return {
            GetRpos(ptrs_[0], static_cast<float>(wpos_ + size_) - left_delay_samples),
            GetRpos(ptrs_[1], static_cast<float>(wpos_ + size_) - right_delay_samples)
        };
    }

    PackFloat<N> GetAfterPush(
        PackFloat<N> const& left_right_delay_samples
    ) noexcept {
        return GetRpos2(static_cast<float>(wpos_ + size_) - left_right_delay_samples);
    }

    std::pair<PackFloat<N>, PackFloat<N>> GetBeforePush(
        PackFloat<N> const& left_delay_samples,
        PackFloat<N> const& right_delay_samples
    ) noexcept {
        return {
            GetRpos(ptrs_[0], static_cast<float>(wpos_ + mask_) - left_delay_samples),
            GetRpos(ptrs_[1], static_cast<float>(wpos_ + mask_) - right_delay_samples)
        };
    }

    PackFloat<N> GetBeforePush(
        PackFloat<N> const& left_right_delay_samples
    ) noexcept {
        return GetRpos(static_cast<float>(wpos_ + mask_) - left_right_delay_samples);
    }
private:
    QWQDSP_FORCE_INLINE
    PackFloat<N> GetRpos(float* ptr, PackFloat<N> const& rpos) noexcept {
        auto t = PackOps::Frac(rpos);
        // we are at the -1 position[-1, 0, 1, 2]
        auto irpos = rpos.ToUint() - 1u;
        irpos &= mask_;

        PackFloat<N> yn1;
        PackFloat<N> y0;
        PackFloat<N> y1;
        PackFloat<N> y2;
        if constexpr (N == 4) {
            // load [-1, 0, 1, 2]
            PackFloat<4> channel0;
            channel0.Load(ptr + irpos[0]);
            PackFloat<4> channel1;
            channel1.Load(ptr + irpos[1]);
            PackFloat<4> channel2;
            channel2.Load(ptr + irpos[2]);
            PackFloat<4> channel3;
            channel3.Load(ptr + irpos[3]);

            // transpose
            yn1[0] = channel0[0];
            yn1[1] = channel1[0];
            yn1[2] = channel2[0];
            yn1[3] = channel3[0];

            y0[0] = channel0[1];
            y0[1] = channel1[1];
            y0[2] = channel2[1];
            y0[3] = channel3[1];

            y1[0] = channel0[2];
            y1[1] = channel1[2];
            y1[2] = channel2[2];
            y1[3] = channel3[2];

            y2[0] = channel0[3];
            y2[1] = channel1[3];
            y2[2] = channel2[3];
            y2[3] = channel3[3];
        }
        else if constexpr (N == 8) {
            // load [-1, 0, 1, 2]
            PackFloat<4> channel0;
            channel0.Load(ptr + irpos[0]);
            PackFloat<4> channel1;
            channel1.Load(ptr + irpos[1]);
            PackFloat<4> channel2;
            channel2.Load(ptr + irpos[2]);
            PackFloat<4> channel3;
            channel3.Load(ptr + irpos[3]);
            PackFloat<4> channel4;
            channel4.Load(ptr + irpos[4]);
            PackFloat<4> channel5;
            channel5.Load(ptr + irpos[5]);
            PackFloat<4> channel6;
            channel6.Load(ptr + irpos[6]);
            PackFloat<4> channel7;
            channel7.Load(ptr + irpos[7]);

            // transpose
            yn1[0] = channel0[0];
            yn1[1] = channel1[0];
            yn1[2] = channel2[0];
            yn1[3] = channel3[0];
            yn1[4] = channel4[0];
            yn1[5] = channel5[0];
            yn1[6] = channel6[0];
            yn1[7] = channel7[0];

            y0[0] = channel0[1];
            y0[1] = channel1[1];
            y0[2] = channel2[1];
            y0[3] = channel3[1];
            y0[4] = channel4[1];
            y0[5] = channel5[1];
            y0[6] = channel6[1];
            y0[7] = channel7[1];

            y1[0] = channel0[2];
            y1[1] = channel1[2];
            y1[2] = channel2[2];
            y1[3] = channel3[2];
            y1[4] = channel4[2];
            y1[5] = channel5[2];
            y1[6] = channel6[2];
            y1[7] = channel7[2];

            y2[0] = channel0[3];
            y2[1] = channel1[3];
            y2[2] = channel2[3];
            y2[3] = channel3[3];
            y2[4] = channel4[3];
            y2[5] = channel5[3];
            y2[6] = channel6[3];
            y2[7] = channel7[3];
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

    QWQDSP_FORCE_INLINE
    PackFloat<N> GetRpos2(PackFloat<N> const& rpos) noexcept {
        auto t = PackOps::Frac(rpos);
        // we are at the -1 position[-1, 0, 1, 2]
        auto irpos = rpos.ToUint() - 1u;
        irpos &= mask_;

        PackFloat<N> yn1;
        PackFloat<N> y0;
        PackFloat<N> y1;
        PackFloat<N> y2;
        if constexpr (N == 4) {
            // load [-1, 0, 1, 2]
            PackFloat<4> channel0;
            channel0.Load(ptrs_[0] + irpos[0]);
            PackFloat<4> channel1;
            channel1.Load(ptrs_[1] + irpos[1]);
            PackFloat<4> channel2;
            channel2.Load(ptrs_[0] + irpos[2]);
            PackFloat<4> channel3;
            channel3.Load(ptrs_[1] + irpos[3]);

            // transpose
            yn1[0] = channel0[0];
            yn1[1] = channel1[0];
            yn1[2] = channel2[0];
            yn1[3] = channel3[0];

            y0[0] = channel0[1];
            y0[1] = channel1[1];
            y0[2] = channel2[1];
            y0[3] = channel3[1];

            y1[0] = channel0[2];
            y1[1] = channel1[2];
            y1[2] = channel2[2];
            y1[3] = channel3[2];

            y2[0] = channel0[3];
            y2[1] = channel1[3];
            y2[2] = channel2[3];
            y2[3] = channel3[3];
        }
        else if constexpr (N == 8) {
            // load [-1, 0, 1, 2]
            PackFloat<4> channel0;
            channel0.Load(ptrs_[0] + irpos[0]);
            PackFloat<4> channel1;
            channel1.Load(ptrs_[1] + irpos[1]);
            PackFloat<4> channel2;
            channel2.Load(ptrs_[0] + irpos[2]);
            PackFloat<4> channel3;
            channel3.Load(ptrs_[1] + irpos[3]);
            PackFloat<4> channel4;
            channel4.Load(ptrs_[0] + irpos[4]);
            PackFloat<4> channel5;
            channel5.Load(ptrs_[1] + irpos[5]);
            PackFloat<4> channel6;
            channel6.Load(ptrs_[0] + irpos[6]);
            PackFloat<4> channel7;
            channel7.Load(ptrs_[1] + irpos[7]);

            // transpose
            yn1[0] = channel0[0];
            yn1[1] = channel1[0];
            yn1[2] = channel2[0];
            yn1[3] = channel3[0];
            yn1[4] = channel4[0];
            yn1[5] = channel5[0];
            yn1[6] = channel6[0];
            yn1[7] = channel7[0];

            y0[0] = channel0[1];
            y0[1] = channel1[1];
            y0[2] = channel2[1];
            y0[3] = channel3[1];
            y0[4] = channel4[1];
            y0[5] = channel5[1];
            y0[6] = channel6[1];
            y0[7] = channel7[1];

            y1[0] = channel0[2];
            y1[1] = channel1[2];
            y1[2] = channel2[2];
            y1[3] = channel3[2];
            y1[4] = channel4[2];
            y1[5] = channel5[2];
            y1[6] = channel6[2];
            y1[7] = channel7[2];

            y2[0] = channel0[3];
            y2[1] = channel1[3];
            y2[2] = channel2[3];
            y2[3] = channel3[3];
            y2[4] = channel4[3];
            y2[5] = channel5[3];
            y2[6] = channel6[3];
            y2[7] = channel7[3];
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

    std::vector<float> buffer_;
    std::array<float*, 2> ptrs_{};
    uint32_t size_{};
    uint32_t wpos_{};
    uint32_t mask_{};
};
} // namespace qwqdsp_simd_element
