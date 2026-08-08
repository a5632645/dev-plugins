#pragma once

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <vector>

#include "pluginshared/simd/simd.hpp"

#include "sinc_table_provider.hpp"

/**
 * Stereo windowed-sinc interpolating delay line using the
 * ShortcircuitSincTableProvider (16-tap, FIRipol_M = 256).
 *
 * - Power-of-two circular buffer, indexed via `& (size - 1)`.
 * - Stores the left and right channels together in one object.
 * - Init(max_ms, fs) sizes the buffer dynamically, rounding the needed
 *   sample count up to the next power of two.
 */
class ShortcircuitSincDelayLine {
public:
    using stp = ShortcircuitSincTableProvider;

    // FIRipol_M / FIRipol_N are uint32_t; keep int copies for clean arithmetic
    static constexpr int kN = static_cast<int>(stp::FIRipol_N);
    static constexpr int kM = static_cast<int>(stp::FIRipol_M);

    ShortcircuitSincDelayLine() = default;

    /** Size the circular buffer for max_ms milliseconds at sample rate fs. */
    void Init(float max_ms, float fs) {
        table_.init(); // build the sinc table once (no-op afterwards)

        size_t const samples = static_cast<size_t>(std::ceil(max_ms * 0.001f * fs));
        comb_size_ = static_cast<int>(NextPow2(std::max(samples, size_t(kN))));
        mask_ = comb_size_ - 1;

        bufferL_.assign(static_cast<size_t>(comb_size_) + kN, 0.0f);
        bufferR_.assign(static_cast<size_t>(comb_size_) + kN, 0.0f);
        wp_ = 0;
    }

    inline void write(float l, float r) {
        size_t const w = static_cast<size_t>(wp_);
        bufferL_[w] = l;
        bufferR_[w] = r;
        // mirror the first taps so wrapped reads near the end stay valid
        size_t const pad = (wp_ < kN) ? static_cast<size_t>(comb_size_) : 0u;
        bufferL_[w + pad] = l;
        bufferR_[w + pad] = r;
        wp_ = (wp_ + 1) & mask_;
    }

    /** Sinc-interpolated read, `delay` samples behind the write head (stereo). */
    inline void read(float delay, float& l, float& r) {
        int const iDelay = static_cast<int>(delay);
        float const frac = delay - static_cast<float>(iDelay);
        int const sincTableOffset = static_cast<int>((1.0f - frac) * static_cast<float>(kM)) * kN;
        int const readPtr = (wp_ - iDelay - (kN >> 1)) & mask_;

        l = dot16(bufferL_.data(), readPtr, table_.SincTableF32 + sincTableOffset);
        r = dot16(bufferR_.data(), readPtr, table_.SincTableF32 + sincTableOffset);
    }

    inline void clear() {
        std::fill(bufferL_.begin(), bufferL_.end(), 0.0f);
        std::fill(bufferR_.begin(), bufferR_.end(), 0.0f);
        wp_ = 0;
    }

    int combSize() const noexcept {
        return comb_size_;
    }

private:
    /** 16-tap windowed-sinc dot product (4 x 4-lane SIMD). */
    static float dot16(const float* data, int readPtr, const float* table) {
        simd::Float128 o = simd::Loadu128(data + readPtr) * simd::Loadu128(table);
        o += simd::Loadu128(data + readPtr + 4) * simd::Loadu128(table + 4);
        o += simd::Loadu128(data + readPtr + 8) * simd::Loadu128(table + 8);
        o += simd::Loadu128(data + readPtr + 12) * simd::Loadu128(table + 12);
        return simd::ReduceAdd(o);
    }

    static size_t NextPow2(size_t v) {
        v--;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
#if SIZE_MAX > UINT32_MAX
        v |= v >> 32;
#endif
        return v + 1;
    }

    static inline ShortcircuitSincTableProvider table_; // shared sinc table (single copy, like a DLL global)
    std::vector<float> bufferL_, bufferR_;
    int comb_size_{0};
    int mask_{0};
    int wp_{0};
};
