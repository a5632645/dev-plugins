#pragma once
#include <array>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <cstdint>
#include <complex>

namespace qwqdsp_oscillator {
/**
 * @brief 线性插值正弦查找表
 */
template<class T, size_t kTableBits = 12>
class TableSineV2 {
public:
    static_assert(kTableBits <= 16, "too large lookup table");

    static constexpr uint32_t kTableSize = 1 << kTableBits;
    static constexpr uint32_t kFracLen = 32 - kTableBits;
    static constexpr uint64_t kScale = 0x100000000;
    static constexpr uint32_t kQuadPhase = static_cast<uint32_t>(kScale / 4);
    static constexpr uint32_t kShift = kFracLen;
    static constexpr T kFracFactor = 1.0f / static_cast<T>(1 << kShift);

    inline static const std::array kSineTable = [] {
        std::array<T, kTableSize + 1> r;
        r[kTableSize] = 0;
        for (uint32_t i = 0; i < kTableSize; ++i) {
            r[i] = static_cast<float>(std::sin(std::numbers::pi_v<double> * 2 * static_cast<double>(i) / static_cast<double>(kTableSize)));
        }
        return r;
    }();

    static T Sine(uint32_t phase) noexcept {
        uint32_t const idx = phase >> kShift;
        uint32_t const frac = phase & ((1 << kShift) - 1);
        T const frac_norm = static_cast<T>(frac) * kFracFactor;
        T const v_low = kSineTable[idx];
        T const v_high = kSineTable[idx + 1];
        T const delta = v_high - v_low;
        return v_low + delta * frac_norm;
    }

    static T Cosine(uint32_t phase) noexcept {
        uint32_t const t = phase + kQuadPhase;
        return Sine(t);;
    }

    static std::complex<T> GetCpx(uint32_t phase) noexcept {
        return {Cosine(phase), Sine(phase)};
    }

    static constexpr uint32_t Omega2PhaseInc(T omega) noexcept {
        omega /= std::numbers::pi_v<T> * 2;
        return static_cast<uint32_t>(omega * static_cast<T>(kScale));
    }

    static constexpr uint32_t FreqPhaseInc(T f, T fs) noexcept {
        return static_cast<uint32_t>(f * static_cast<T>(kScale) / fs);
    }
};

// template<class T, size_t kTableBits = 12>
// class TableSineV4 {
// public:
//     static constexpr uint32_t kTableSize = 1 << kTableBits;
//     static constexpr uint32_t kFracLen = 32 - kTableBits;
//     static constexpr uint64_t kScale = 0x100000000;
//     static constexpr uint32_t kQuadPhase = static_cast<uint32_t>(kScale / 4);
//     static constexpr uint32_t kShift = kFracLen;
//     static constexpr T kFracFactor = 1.0f / static_cast<T>(1 << kShift);

//     inline static const std::array<T, kTableSize + 2> kSineTable = [] {
//         std::array<T, kTableSize + 2> r;
//         T const two_pi_over_N = std::numbers::pi_v<T> * 2 / static_cast<T>(kTableSize);

//         for (uint32_t i = 0; i < kTableSize; ++i) {
//             r[i + 1] = std::sin(static_cast<T>(i) * two_pi_over_N);
//         }

//         r[0] = r[kTableSize];
//         r[kTableSize + 1] = r[1];
//         return r;
//     }();
    
//     // --- 二次抛物线插值 Sine 函数 ---
//     static T Sine(uint32_t phase) noexcept {
//         uint32_t const idx = phase >> kShift;
//         uint32_t const frac = phase & ((1 << kShift) - 1);
//         T const t = static_cast<T>(frac) * kFracFactor;
//         auto yn1 = kSineTable[idx];
//         auto y0 = kSineTable[idx + 1];
//         auto y1 = kSineTable[idx + 2];
//         auto C = y0;
//         auto B = y1 - y0;
//         auto A = (y1 + yn1) / 2 - y0;
//         return A * t * (1 - t) + B * t + C;
//     }
    
//     static T Cosine(uint32_t phase) noexcept {
//         uint32_t const t = phase + static_cast<uint32_t>(kScale / 4);
//         return Sine(t);
//     }

//     static constexpr uint32_t Omega2PhaseInc(T omega) noexcept {
//         omega /= std::numbers::pi_v<T> * 2;
//         return static_cast<uint32_t>(omega * static_cast<T>(kScale));
//     }

//     static constexpr uint32_t FreqPhaseInc(T f, T fs) noexcept {
//         return static_cast<uint32_t>(f * static_cast<T>(kScale) / fs);
//     }
// };
}
