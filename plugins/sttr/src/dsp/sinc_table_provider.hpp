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

#ifndef INCLUDE_SST_BASIC_BLOCKS_TABLES_SINCTABLEPROVIDER_H
#define INCLUDE_SST_BASIC_BLOCKS_TABLES_SINCTABLEPROVIDER_H

#include <algorithm>
#include <cmath>
#include <cstdint>

// sst-basic-blocks SpecialFunctions.h relies on the POSIX M_PI macro, which is
// not defined by default on Windows/clang under strict C++; provide it here.
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// The windowing helpers below are inlined from sst-basic-blocks'
// dsp/SpecialFunctions.h, so this header no longer depends on that library.
namespace dsp
{

inline double sincf(double x)
{
    if (std::fabs(x) < 1e-30)
        return 1.0;
    return (std::sin(M_PI * x)) / (M_PI * x);
}

inline double symmetric_blackman(double i, int n)
{
    i -= (n / 2.0);

    return (0.42 - 0.5 * std::cos(2 * M_PI * i / (n)) + 0.08 * std::cos(4 * M_PI * i / (n)));
}

inline double BESSI0(double X)
{
    double Y, P1, P2, P3, P4, P5, P6, P7, Q1, Q2, Q3, Q4, Q5, Q6, Q7, Q8, Q9, AX, BX;
    P1 = 1.0;
    P2 = 3.5156229;
    P3 = 3.0899424;
    P4 = 1.2067429;
    P5 = 0.2659732;
    P6 = 0.360768e-1;
    P7 = 0.45813e-2;
    Q1 = 0.39894228;
    Q2 = 0.1328592e-1;
    Q3 = 0.225319e-2;
    Q4 = -0.157565e-2;
    Q5 = 0.916281e-2;
    Q6 = -0.2057706e-1;
    Q7 = 0.2635537e-1;
    Q8 = -0.1647633e-1;
    Q9 = 0.392377e-2;
    if (std::fabs(X) < 3.75)
    {
        Y = (X / 3.75) * (X / 3.75);
        return (P1 + Y * (P2 + Y * (P3 + Y * (P4 + Y * (P5 + Y * (P6 + Y * P7))))));
    }
    else
    {
        AX = std::fabs(X);
        Y = 3.75 / AX;
        BX = std::exp(AX) / std::sqrt(AX);
        AX = Q1 +
             Y * (Q2 + Y * (Q3 + Y * (Q4 + Y * (Q5 + Y * (Q6 + Y * (Q7 + Y * (Q8 + Y * Q9)))))));
        return (AX * BX);
    }
}

inline double symmetric_kaiser(double x, uint16_t nint, double Alpha)
{
    double N = (double)nint;
    x += N * 0.5;

    x = std::clamp(x, 0.0, N);
    double a = (2.0 * x / N - 1.0);
    return BESSI0(M_PI * Alpha * std::sqrt(1.0 - a * a)) / BESSI0(M_PI * Alpha);
}

} // namespace dsp

struct ShortcircuitSincTableProvider
{
    static constexpr uint32_t FIRipol_M = 256;
    static constexpr uint32_t FIRipol_N = 16;
    static constexpr uint32_t FIRipolI16_N = 16;
    static constexpr uint32_t FIRoffset = 8;

    void init()
    {
        if (initialized)
            return;

        float cutoff = 0.95f;
        float cutoffI16 = 0.95f;
        for (auto j = 0U; j < FIRipol_M + 1; j++)
        {
            for (auto i = 0U; i < FIRipol_N; i++)
            {
                double t =
                    -double(i) + double(FIRipol_N / 2.0) + double(j) / double(FIRipol_M) - 1.0;
                double val = (float)(dsp::symmetric_kaiser(t, FIRipol_N, 5.0) * cutoff *
                                     dsp::sincf(cutoff * t));

                SincTableF32[j * FIRipol_N + i] = static_cast<float>(val);
            }
        }
        for (auto j = 0U; j < FIRipol_M; j++)
        {
            for (auto i = 0U; i < FIRipol_N; i++)
            {
                SincOffsetF32[j * FIRipol_N + i] = (float)((SincTableF32[(j + 1) * FIRipol_N + i] -
                                                            SincTableF32[j * FIRipol_N + i]) *
                                                           (1.0 / 65536.0));
            }
        }

        for (auto j = 0U; j < FIRipol_M + 1; j++)
        {
            for (auto i = 0U; i < FIRipolI16_N; i++)
            {
                double t =
                    -double(i) + double(FIRipolI16_N / 2.0) + double(j) / double(FIRipol_M) - 1.0;
                double val = (float)(dsp::symmetric_kaiser(t, FIRipol_N, 5.0) * cutoffI16 *
                                     dsp::sincf(cutoffI16 * t));

                SincTableI16[j * FIRipolI16_N + i] = static_cast<short>(val * 16384);
            }
        }
        for (auto j = 0U; j < FIRipol_M; j++)
        {
            for (auto i = 0U; i < FIRipolI16_N; i++)
            {
                SincOffsetI16[j * FIRipolI16_N + i] =
                    (SincTableI16[(j + 1) * FIRipolI16_N + i] - SincTableI16[j * FIRipolI16_N + i]);
            }
        }

        initialized = true;
    }

    // TODO Rename these when i'm all done
    float SincTableF32[(FIRipol_M + 1) * FIRipol_N];
    float SincOffsetF32[(FIRipol_M)*FIRipol_N];
    short SincTableI16[(FIRipol_M + 1) * FIRipol_N];
    short SincOffsetI16[(FIRipol_M)*FIRipol_N];

  private:
    bool initialized{false};
};

#endif // SURGE_SINCTABLEPROVIDER_H
