#pragma once
#include <array>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <cstdint>
#include <complex>

namespace qwqdsp_oscillator {
/**
 * @brief 抛物线插值正弦查找表
 */
template<class T, size_t kTableBits = 12>
class TableSineV3 {
public:
    static constexpr uint32_t kTableSize = 1 << kTableBits;
    static constexpr uint32_t kFracLen = 32 - kTableBits;
    static constexpr uint64_t kScale = 0x100000000;
    static constexpr uint32_t kQuadPhase = static_cast<uint32_t>(kScale / 4);
    static constexpr uint32_t kShift = kFracLen;
    static constexpr T kFracFactor = 1.0f / static_cast<T>(1 << kShift);

    inline static const std::array<T, kTableSize + 2> kSineTable = [] {
        std::array<T, kTableSize + 2> r;
        double const two_pi_over_N = std::numbers::pi_v<double> * 2 / static_cast<T>(kTableSize);

        for (uint32_t i = 0; i < kTableSize; ++i) {
            r[i] = static_cast<T>(std::sin(static_cast<double>(i) * two_pi_over_N));
        }
        // 周期性闭合，用于插值
        r[kTableSize] = r[0];   // sin(2pi) = sin(0)
        r[kTableSize + 1] = r[1]; // sin(2pi + delta) = sin(delta)
        return r;
    }();
    
    // --- 二次抛物线插值 Sine 函数 ---
    static T Sine(uint32_t phase) noexcept {
        uint32_t const idx = phase >> kShift;
        uint32_t const frac = phase & ((1 << kShift) - 1);
        T const t = static_cast<T>(frac) * kFracFactor; // t 范围 [0, 1]
        
        // 查找三个点: y0, y1, y2
        T const y0 = kSineTable[idx];
        T const y1 = kSineTable[idx + 1];
        T const y2 = kSineTable[idx + 2];

        // 1. 计算一阶差分 (近似斜率)
        T const d1 = y1 - y0;
        
        // 2. 计算二阶差分 (近似曲率)
        // d2 = (y2 - y1) - (y1 - y0) = y2 - 2*y1 + y0
        T const d2 = y2 - 2 * y1 + y0; 
        
        // 3. 应用插值公式（基于 Newton 前向差分形式的变体）
        // Sine(t) = y0 + d1 * t + (d2/2) * t * (t-1)
        // 简化为：Sine(t) = y0 + t * (d1 + d2/2 * (t-1))
        
        T const t_minus_1 = t - static_cast<T>(1.0);
        T const quad_term = d2 * static_cast<T>(0.5) * t_minus_1; // 二次校正项：(d2/2) * (t-1)
        
        return y0 + t * (d1 + quad_term);
    }
    
    static T Cosine(uint32_t phase) noexcept {
        uint32_t const t = phase + static_cast<uint32_t>(kScale / 4);
        return Sine(t);
    }

    static std::complex<T> GetCpx(uint32_t phase) noexcept {
        return {Cosine(phase), Sine(phase)};
    }

    static constexpr uint32_t Omega2PhaseInc(T omega) noexcept {
        omega /= std::numbers::pi_v<T> * 2;
        return static_cast<uint32_t>(omega * static_cast<T>(kScale));
    }

    static constexpr uint32_t FloatPhase2Phase(T phase01) noexcept {
        return static_cast<uint32_t>(phase01 * static_cast<T>(kScale));
    }

    static constexpr uint32_t FreqPhaseInc(T f, T fs) noexcept {
        return static_cast<uint32_t>(f * static_cast<T>(kScale) / fs);
    }
};
}
