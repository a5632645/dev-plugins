#pragma once
#include <vector>
#include "../align_allocator.hpp"
#include "../simd.hpp"

namespace pluginshared::dsp {

template <simd::IsSimdFloat SimdT>
class DelayLineSingleChannelMultiTime {
public:
    void Init(float max_ms, float fs) {
        float d = max_ms * fs / 1000.0f;
        size_t i = static_cast<size_t>(d);
        Init(i);
    }

    void Init(size_t max_samples) {
        uint32_t a = 1;
        while (a < max_samples) {
            a *= 2;
        }
        size_ = static_cast<int>(a);
        mask_ = static_cast<int>(a - 1);
        uint32_t each_size = a * 2;
        buffer_.resize(each_size * simd::LaneSize<SimdT>);
    }

    void Reset() noexcept {
        wpos_ = 0;
        std::fill(buffer_.begin(), buffer_.end(), float{});
    }

    void Push(float x) noexcept {
        wpos_ = (wpos_ + 1) & mask_;
        buffer_[wpos_] = x;
        buffer_[wpos_ + size_] = x;
    }

    SimdT GetAfterPush(SimdT delay_samples) noexcept {
        auto rpos = static_cast<float>(wpos_ + size_) - delay_samples;
        return GetRpos(rpos);
    }

    SimdT GetBeforePush(SimdT delay_samples) noexcept {
        auto rpos = static_cast<float>(wpos_ + mask_) - delay_samples;
        return GetRpos(rpos);
    }
private:
    simd::Float128 GetRpos(simd::Float128 rpos) noexcept {
        auto t = simd::Frac(rpos);
        // we are at the -1 position[-1, 0, 1, 2]
        auto irpos = simd::ToInt(rpos) - 1;
        irpos &= mask_;

#ifndef SIMDE_X86_AVX2_NATIVE
        alignas(16) auto [yn1, y0, y1, y2] =
            simd::Transpose(simd::Loadu128(buffer_.data() + irpos[0]), simd::Loadu128(buffer_.data() + irpos[1]),
                            simd::Loadu128(buffer_.data() + irpos[2]), simd::Loadu128(buffer_.data() + irpos[3]));
#else
        float const* raw = buffer_.data();
        simde__m128i base_vindex = simd::ToSimde(irpos);
        auto yn1 = simd::FromSimde(simde_mm_i32gather_ps(raw, base_vindex, 4));
        auto y0 =
            simd::FromSimde(simde_mm_i32gather_ps(raw, simde_mm_add_epi32(base_vindex, simde_mm_set1_epi32(1)), 4));
        auto y1 =
            simd::FromSimde(simde_mm_i32gather_ps(raw, simde_mm_add_epi32(base_vindex, simde_mm_set1_epi32(2)), 4));
        auto y2 =
            simd::FromSimde(simde_mm_i32gather_ps(raw, simde_mm_add_epi32(base_vindex, simde_mm_set1_epi32(3)), 4));
#endif

        auto d0 = (y1 - yn1) * 0.5f;
        auto d1 = (y2 - y0) * 0.5f;
        auto d = y1 - y0;
        auto m0 = 3.0f * d - 2.0f * d0 - d1;
        auto m1 = d0 - 2.0f * d + d1;
        return y0 + t * (d0 + t * (m0 + t * m1));
    }

    simd::Float256 GetRpos(simd::Float256 rpos) noexcept {
        auto t = simd::Frac(rpos);
        // we are at the -1 position[-1, 0, 1, 2]
        auto irpos = simd::ToInt(rpos) - 1;
        irpos &= mask_;

#ifndef SIMDE_X86_AVX2_NATIVE
        alignas(32) auto [yn1, y0, y1, y2] =
            simd::Transpose256(simd::Loadu128(buffer_.data() + irpos[0]), simd::Loadu128(buffer_.data() + irpos[1]),
                               simd::Loadu128(buffer_.data() + irpos[2]), simd::Loadu128(buffer_.data() + irpos[3]),
                               simd::Loadu128(buffer_.data() + irpos[4]), simd::Loadu128(buffer_.data() + irpos[5]),
                               simd::Loadu128(buffer_.data() + irpos[6]), simd::Loadu128(buffer_.data() + irpos[7]));
#else
        float const* raw = buffer_.data();
        simde__m256i base_vindex = simd::ToSimde(irpos);
        auto yn1 = simd::FromSimde(simde_mm256_i32gather_ps(raw, base_vindex, 4));
        auto y0 = simd::FromSimde(
            simde_mm256_i32gather_ps(raw, simde_mm256_add_epi32(base_vindex, simde_mm256_set1_epi32(1)), 4));
        auto y1 = simd::FromSimde(
            simde_mm256_i32gather_ps(raw, simde_mm256_add_epi32(base_vindex, simde_mm256_set1_epi32(2)), 4));
        auto y2 = simd::FromSimde(
            simde_mm256_i32gather_ps(raw, simde_mm256_add_epi32(base_vindex, simde_mm256_set1_epi32(3)), 4));
#endif

        auto d0 = (y1 - yn1) * 0.5f;
        auto d1 = (y2 - y0) * 0.5f;
        auto d = y1 - y0;
        auto m0 = 3.0f * d - 2.0f * d0 - d1;
        auto m1 = d0 - 2.0f * d + d1;
        return y0 + t * (d0 + t * (m0 + t * m1));
    }

    std::vector<float, simd::AlignedAllocator<float, alignof(SimdT)>> buffer_;
    int size_{};
    int wpos_{};
    int mask_{};
};

} // namespace pluginshared::dsp
