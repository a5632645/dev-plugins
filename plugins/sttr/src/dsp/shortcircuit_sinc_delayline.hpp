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

    /** Sinc-interpolated read at N delays (one per lane), stereo.
        Only the first N lanes are read; lanes >= N come back as zero. */
    template <int N>
    inline void read(simd::Float128 delay, simd::Float128& l, simd::Float128& r) {
        // one 4-lane accumulator per input lane (lanes >= N stay zero)
        simd::Float128 accL[4] = {};
        simd::Float128 accR[4] = {};

        // vectorised index math for all 4 lanes at once (lanes >= N hold garbage,
        // which is fine — only the first N lanes are consumed below)
        simd::Int128 const iDelay = simd::ToInt(delay);
        simd::Float128 const frac = delay - simd::ToFloat(iDelay);
        simd::Int128 const sincTableOffset =
            simd::ToInt((simd::BroadcastF128(1.0f) - frac) * simd::BroadcastF128(static_cast<float>(kM))) * kN;
        simd::Int128 const readPtr = (simd::Int128{wp_, wp_, wp_, wp_} - iDelay - (kN >> 1)) & mask_;

        for (int i = 0; i < N; ++i) {
            accL[i] = dot16(bufferL_.data(), readPtr[i], table_.SincTableF32 + sincTableOffset[i]);
            accR[i] = dot16(bufferR_.data(), readPtr[i], table_.SincTableF32 + sincTableOffset[i]);
        }

        // horizontal reduce per lane: transpose so each output lane holds the four
        // tap-chunk partial sums of one input lane, then collapse with a vertical add.
        alignas(16) auto const tL = simd::Transpose(accL[0], accL[1], accL[2], accL[3]);
        alignas(16) auto const tR = simd::Transpose(accR[0], accR[1], accR[2], accR[3]);
        l = tL[0] + tL[1] + tL[2] + tL[3];
        r = tR[0] + tR[1] + tR[2] + tR[3];
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
    /** 16-tap windowed-sinc dot product (4 x 4-lane SIMD).
        Returns the four 4-tap partial sums; the caller collapses them. */
    static simd::Float128 dot16(const float* data, int readPtr, const float* table) {
        simd::Float128 o = simd::Loadu128(data + readPtr) * simd::Loadu128(table);
        o += simd::Loadu128(data + readPtr + 4) * simd::Loadu128(table + 4);
        o += simd::Loadu128(data + readPtr + 8) * simd::Loadu128(table + 8);
        o += simd::Loadu128(data + readPtr + 12) * simd::Loadu128(table + 12);
        return o;
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
