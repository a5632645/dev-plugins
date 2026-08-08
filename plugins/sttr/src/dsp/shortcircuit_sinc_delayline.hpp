#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <vector>

#include "pluginshared/simd/inst.hpp"
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
 *
 * Templated on (inst, SimdT): each ISA variant is a distinct type, and the
 * read kernel picks 128-bit vs 256-bit dot products via requires on SimdT.
 */
template <simd::Inst inst, class SimdT>
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

    /** Sinc-interpolated read at N delays (one per lane), stereo — 128-bit dot path.
        Only the first N lanes are read; lanes >= N come back as zero. */
    template <int N>
        requires std::same_as<SimdT, simd::Float128>
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
            dot16LR(bufferL_.data(), bufferR_.data(), readPtr[i], table_.SincTableF32 + sincTableOffset[i], accL[i],
                    accR[i]);
        }

        // horizontal reduce per lane: transpose so each output lane holds the four
        // tap-chunk partial sums of one input lane, then collapse with a vertical add.
        alignas(16) auto const tL = simd::Transpose(accL[0], accL[1], accL[2], accL[3]);
        alignas(16) auto const tR = simd::Transpose(accR[0], accR[1], accR[2], accR[3]);
        l = tL[0] + tL[1] + tL[2] + tL[3];
        r = tR[0] + tR[1] + tR[2] + tR[3];
    }

    /** Sinc-interpolated read at N delays (one per lane), stereo — 256-bit dot path.
        Same signature as the 128-bit read; selected via requires on SimdT. */
    template <int N>
        requires std::same_as<SimdT, simd::Float256>
    inline void read(simd::Float128 delay, simd::Float128& l, simd::Float128& r) {
        // one 8-lane accumulator per input lane (lanes >= N stay zero)
        simd::Float256 accL[8] = {};
        simd::Float256 accR[8] = {};

        // vectorised index math for the 4 delay lanes
        simd::Int128 const iDelay = simd::ToInt(delay);
        simd::Float128 const frac = delay - simd::ToFloat(iDelay);
        simd::Int128 const sincTableOffset =
            simd::ToInt((simd::BroadcastF128(1.0f) - frac) * simd::BroadcastF128(static_cast<float>(kM))) * kN;
        simd::Int128 const readPtr = (simd::Int128{wp_, wp_, wp_, wp_} - iDelay - (kN >> 1)) & mask_;

        for (int i = 0; i < N; ++i) {
            dot16LR256(bufferL_.data(), bufferR_.data(), readPtr[i], table_.SincTableF32 + sincTableOffset[i], accL[i],
                       accR[i]);
        }

        // horizontal reduce per lane: transpose the 8x8 partial-sum matrix so each
        // output lane holds the eight tap-chunk sums of one input lane, then
        // collapse with a vertical add; only the low 4 lanes hold grain results.
        alignas(32) auto const colL = Transpose8x8(accL);
        alignas(32) auto const colR = Transpose8x8(accR);
        simd::Float256 const outL = colL[0] + colL[1] + colL[2] + colL[3] + colL[4] + colL[5] + colL[6] + colL[7];
        simd::Float256 const outR = colR[0] + colR[1] + colR[2] + colR[3] + colR[4] + colR[5] + colR[6] + colR[7];
        l = simd::Float128{outL[0], outL[1], outL[2], outL[3]};
        r = simd::Float128{outR[0], outR[1], outR[2], outR[3]};
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
    /** 16-tap windowed-sinc dot products for both channels at once.
        The sinc table row is loaded once and shared by L/R; returns the four
        4-tap partial sums per channel, the caller collapses them. */
    static void dot16LR(const float* dataL, const float* dataR, int readPtr, const float* table, simd::Float128& oL,
                        simd::Float128& oR) {
        simd::Float128 const t0 = simd::Loadu128(table);
        simd::Float128 const t1 = simd::Loadu128(table + 4);
        simd::Float128 const t2 = simd::Loadu128(table + 8);
        simd::Float128 const t3 = simd::Loadu128(table + 12);

        oL = simd::Loadu128(dataL + readPtr) * t0;
        oR = simd::Loadu128(dataR + readPtr) * t0;
        oL += simd::Loadu128(dataL + readPtr + 4) * t1;
        oR += simd::Loadu128(dataR + readPtr + 4) * t1;
        oL += simd::Loadu128(dataL + readPtr + 8) * t2;
        oR += simd::Loadu128(dataR + readPtr + 8) * t2;
        oL += simd::Loadu128(dataL + readPtr + 12) * t3;
        oR += simd::Loadu128(dataR + readPtr + 12) * t3;
    }

    /** 16-tap windowed-sinc dot products for both channels at once (256-bit).
        The sinc table row is loaded once and shared by L/R; returns the two
        8-tap partial sums per channel, the caller collapses them. */
    static void dot16LR256(const float* dataL, const float* dataR, int readPtr, const float* table, simd::Float256& oL,
                           simd::Float256& oR) {
        simd::Float256 const t0 = simd::Loadu256(table);
        simd::Float256 const t1 = simd::Loadu256(table + 8);

        oL = simd::Loadu256(dataL + readPtr) * t0;
        oR = simd::Loadu256(dataR + readPtr) * t0;
        oL += simd::Loadu256(dataL + readPtr + 8) * t1;
        oR += simd::Loadu256(dataR + readPtr + 8) * t1;
    }

    /** Transpose an 8x8 matrix stored as 8 Float256 rows into 8 columns.
        Native 256-bit shuffle network: unpack lo/hi -> shuffle_ps -> permute2f128;
        compiles to vunpcklps / vunpckhps / vshufps / vperm2f128 under AVX2. */
    static std::array<simd::Float256, 8> Transpose8x8(simd::Float256 const (&m)[8]) {
        using simd::Float256;
        using simd::Shuffle;

        // unpack lo/hi on row pairs
        Float256 const t0 = Shuffle<Float256, 0, 8, 1, 9, 4, 12, 5, 13>(m[0], m[1]);
        Float256 const t1 = Shuffle<Float256, 2, 10, 3, 11, 6, 14, 7, 15>(m[0], m[1]);
        Float256 const t2 = Shuffle<Float256, 0, 8, 1, 9, 4, 12, 5, 13>(m[2], m[3]);
        Float256 const t3 = Shuffle<Float256, 2, 10, 3, 11, 6, 14, 7, 15>(m[2], m[3]);
        Float256 const t4 = Shuffle<Float256, 0, 8, 1, 9, 4, 12, 5, 13>(m[4], m[5]);
        Float256 const t5 = Shuffle<Float256, 2, 10, 3, 11, 6, 14, 7, 15>(m[4], m[5]);
        Float256 const t6 = Shuffle<Float256, 0, 8, 1, 9, 4, 12, 5, 13>(m[6], m[7]);
        Float256 const t7 = Shuffle<Float256, 2, 10, 3, 11, 6, 14, 7, 15>(m[6], m[7]);

        // vshufps within each 128-bit lane: {a0,a1,b0,b1} / {a2,a3,b2,b3}
        Float256 const v0 = Shuffle<Float256, 0, 1, 8, 9, 4, 5, 12, 13>(t0, t2);
        Float256 const v1 = Shuffle<Float256, 2, 3, 10, 11, 6, 7, 14, 15>(t0, t2);
        Float256 const v2 = Shuffle<Float256, 0, 1, 8, 9, 4, 5, 12, 13>(t1, t3);
        Float256 const v3 = Shuffle<Float256, 2, 3, 10, 11, 6, 7, 14, 15>(t1, t3);
        Float256 const v4 = Shuffle<Float256, 0, 1, 8, 9, 4, 5, 12, 13>(t4, t6);
        Float256 const v5 = Shuffle<Float256, 2, 3, 10, 11, 6, 7, 14, 15>(t4, t6);
        Float256 const v6 = Shuffle<Float256, 0, 1, 8, 9, 4, 5, 12, 13>(t5, t7);
        Float256 const v7 = Shuffle<Float256, 2, 3, 10, 11, 6, 7, 14, 15>(t5, t7);

        // vperm2f128: {lo,lo} then {hi,hi} of each (v_i, v_{i+4}) pair
        return {
            Shuffle<Float256, 0, 1, 2, 3, 8, 9, 10, 11>(v0, v4),
            Shuffle<Float256, 0, 1, 2, 3, 8, 9, 10, 11>(v1, v5),
            Shuffle<Float256, 0, 1, 2, 3, 8, 9, 10, 11>(v2, v6),
            Shuffle<Float256, 0, 1, 2, 3, 8, 9, 10, 11>(v3, v7),
            Shuffle<Float256, 4, 5, 6, 7, 12, 13, 14, 15>(v0, v4),
            Shuffle<Float256, 4, 5, 6, 7, 12, 13, 14, 15>(v1, v5),
            Shuffle<Float256, 4, 5, 6, 7, 12, 13, 14, 15>(v2, v6),
            Shuffle<Float256, 4, 5, 6, 7, 12, 13, 14, 15>(v3, v7),
        };
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
