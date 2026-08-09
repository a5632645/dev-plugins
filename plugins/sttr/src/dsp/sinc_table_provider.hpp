/*
 * sst-basic-blocks - an open source library of core audio utilities
 * built by Surge Synth Team.
 *
 * Provides a collection of tools useful on the audio thread for blocks,
 * modulation, etc... or useful for adapting code to multiple environments.
 *
 * Copyright 2023, various authors, as described in the GitHub
 * transaction log. Parts of this code are derived from similar
 * functions original in Surge or ShortCircuit.
 *
 * sst-basic-blocks is released under the GNU General Public Licence v3
 * or later (GPL-3.0-or-later). The license is found in the "LICENSE"
 * file in the root of this repository, or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html.
 *
 * A very small number of explicitly chosen header files can also be
 * used in an MIT/BSD context. Please see the README.md file in this
 * repo or the comments in the individual files. Only headers with an
 * explicit mention that they are dual licensed may be copied and reused
 * outside the GPL3 terms.
 *
 * All source in sst-basic-blocks available at
 * https://github.com/surge-synthesizer/sst-basic-blocks
 */

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>

// The windowing helpers below are inlined from sst-basic-blocks'
// dsp/SpecialFunctions.h, so this header no longer depends on that library.
namespace dsp {

inline double sincf(double x) {
    if (std::fabs(x) < 1e-30) return 1.0;
    return (std::sin(std::numbers::pi * x)) / (std::numbers::pi * x);
}

inline double symmetric_blackman(double i, int n) {
    i -= (n / 2.0);

    return (0.42 - 0.5 * std::cos(2 * std::numbers::pi * i / (n)) + 0.08 * std::cos(4 * std::numbers::pi * i / (n)));
}

inline double besselI0(double x) {
    double y, p1, p2, p3, p4, p5, p6, p7, q1, q2, q3, q4, q5, q6, q7, q8, q9, ax, bx;
    p1 = 1.0;
    p2 = 3.5156229;
    p3 = 3.0899424;
    p4 = 1.2067429;
    p5 = 0.2659732;
    p6 = 0.360768e-1;
    p7 = 0.45813e-2;
    q1 = 0.39894228;
    q2 = 0.1328592e-1;
    q3 = 0.225319e-2;
    q4 = -0.157565e-2;
    q5 = 0.916281e-2;
    q6 = -0.2057706e-1;
    q7 = 0.2635537e-1;
    q8 = -0.1647633e-1;
    q9 = 0.392377e-2;
    if (std::fabs(x) < 3.75) {
        y = (x / 3.75) * (x / 3.75);
        return (p1 + y * (p2 + y * (p3 + y * (p4 + y * (p5 + y * (p6 + y * p7))))));
    }
    ax = std::fabs(x);
    y = 3.75 / ax;
    bx = std::exp(ax) / std::sqrt(ax);
    ax = q1 + y * (q2 + y * (q3 + y * (q4 + y * (q5 + y * (q6 + y * (q7 + y * (q8 + y * q9)))))));
    return ax * bx;
}

inline double symmetric_kaiser(double x, uint16_t nint, double alpha) {
    double const n = static_cast<double>(nint);
    x += n * 0.5;

    x = std::clamp(x, 0.0, n);
    double const a = (2.0 * x / n - 1.0);
    return besselI0(std::numbers::pi * alpha * std::sqrt(1.0 - a * a)) / besselI0(std::numbers::pi * alpha);
}

} // namespace dsp

struct ShortcircuitSincTableProvider {
    static constexpr uint32_t kFIRipolM = 256;
    static constexpr uint32_t kFIRipolN = 16;
    static constexpr uint32_t kFIRoffset = 8;

    void Init() {
        if (initialized_) return;

        float const cutoff = 0.95f;
        for (auto j = 0U; j < kFIRipolM + 1; j++) {
            for (auto i = 0U; i < kFIRipolN; i++) {
                double const t = -double(i) + double(kFIRipolN / 2.0) + double(j) / double(kFIRipolM) - 1.0;
                double const val = (float)(dsp::symmetric_kaiser(t, kFIRipolN, 5.0) * cutoff * dsp::sincf(cutoff * t));

                sinc_table_f32[j * kFIRipolN + i] = static_cast<float>(val);
            }
        }
        for (auto j = 0U; j < kFIRipolM; j++) {
            for (auto i = 0U; i < kFIRipolN; i++) {
                sinc_offset_f32[j * kFIRipolN + i] =
                    (float)((sinc_table_f32[(j + 1) * kFIRipolN + i] - sinc_table_f32[j * kFIRipolN + i])
                            * (1.0 / 65536.0));
            }
        }

        initialized_ = true;
    }

    // 32-byte aligned: each 16-tap row is 64 bytes (one cache line) and the read
    // offsets are always row-aligned, so future 256-bit SIMD (AVX) can use aligned loads.
    alignas(32) float sinc_table_f32[(kFIRipolM + 1) * kFIRipolN];
    alignas(32) float sinc_offset_f32[(kFIRipolM)*kFIRipolN];
private:
    bool initialized_{false};
};
