#pragma once

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <vector>

#include <emmintrin.h>

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

    /** The table provider must outlive this delay line. */
    explicit ShortcircuitSincDelayLine(ShortcircuitSincTableProvider& st) {
        SetSincTable(st);
    }

    void SetSincTable(ShortcircuitSincTableProvider& st) {
        st.init(); // no-op if already initialized
        sinctable_ = st.SincTableF32;
    }

    /** Size the circular buffer for max_ms milliseconds at sample rate fs. */
    void Init(float max_ms, float fs) {
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

        l = dot16(bufferL_.data(), readPtr, sinctable_ + sincTableOffset);
        r = dot16(bufferR_.data(), readPtr, sinctable_ + sincTableOffset);
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
    /** 16-tap windowed-sinc dot product (4 x 4-lane SSE2). */
    static float dot16(const float* data, int readPtr, const float* table) {
        __m128 a = _mm_loadu_ps(&data[readPtr]);
        __m128 b = _mm_loadu_ps(&table[0]);
        __m128 o = _mm_mul_ps(a, b);

        a = _mm_loadu_ps(&data[readPtr + 4]);
        b = _mm_loadu_ps(&table[4]);
        o = _mm_add_ps(o, _mm_mul_ps(a, b));

        a = _mm_loadu_ps(&data[readPtr + 8]);
        b = _mm_loadu_ps(&table[8]);
        o = _mm_add_ps(o, _mm_mul_ps(a, b));

        a = _mm_loadu_ps(&data[readPtr + 12]);
        b = _mm_loadu_ps(&table[12]);
        o = _mm_add_ps(o, _mm_mul_ps(a, b));

        __m128 r = _mm_add_ps(o, _mm_movehl_ps(o, o));
        r = _mm_add_ss(r, _mm_shuffle_ps(r, r, 1));

        float res;
        _mm_store_ss(&res, r);
        return res;
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

    const float* sinctable_{nullptr};
    std::vector<float> bufferL_, bufferR_;
    int comb_size_{0};
    int mask_{0};
    int wp_{0};
};
