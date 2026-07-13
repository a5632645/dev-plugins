#pragma once
#include <array>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <cstdint>
#include "../simd.hpp"

template<size_t kTableBits = 12>
class TableSineV3 {
public:
    static constexpr uint32_t kTableSize = 1 << kTableBits;
    static constexpr uint32_t kFracLen = 32 - kTableBits;
    static constexpr uint64_t kScale = 0x100000000;
    static constexpr uint32_t kQuadPhase = static_cast<uint32_t>(kScale / 4);
    static constexpr uint32_t kShift = kFracLen;
    static constexpr float kFracFactor = 1.0f / static_cast<float>(1 << kShift);

    inline static const std::array<float, kTableSize + 2> kSineTable = [] {
        std::array<float, kTableSize + 2> r;
        float const two_pi_over_N = std::numbers::pi_v<float> * 2 / static_cast<float>(kTableSize);

        for (uint32_t i = 0; i < kTableSize; ++i) {
            r[i] = std::sin(static_cast<float>(i) * two_pi_over_N);
        }
        r[kTableSize] = r[0];
        r[kTableSize + 1] = r[1];
        return r;
    }();
    
    static simd::Float128 Sine(simd::Uint128 phase) noexcept {
        auto idx = phase >> kShift;
        auto frac = phase & ((1 << kShift) - 1);
        auto t = __builtin_convertvector((simd::Int128)(frac), simd::Float128) * kFracFactor;

        simd::Float128 y0{kSineTable[idx[0]], kSineTable[idx[1]], kSineTable[idx[2]], kSineTable[idx[3]]};
        simd::Float128 y1{kSineTable[idx[0] + 1], kSineTable[idx[1] + 1], kSineTable[idx[2] + 1], kSineTable[idx[3] + 1]};
        simd::Float128 y2{kSineTable[idx[0] + 2], kSineTable[idx[1] + 2], kSineTable[idx[2] + 2], kSineTable[idx[3] + 2]};

        auto d1 = y1 - y0;
        auto d2 = y2 - 2 * y1 + y0; 
        auto t_minus_1 = t - 1.0f;
        auto quad_term = d2 * 0.5f * t_minus_1;
        return y0 + t * (d1 + quad_term);
    }
    
    static simd::Float128 Cosine(simd::Uint128 phase) noexcept {
        auto t = phase + kQuadPhase;
        return Sine(t);
    }

    static constexpr simd::Uint128 Omega2PhaseInc(simd::Float128 omega) noexcept {
        omega /= std::numbers::pi_v<float> * 2; // 应该是[0, 0.5]
        // return static_cast<uint32_t>(omega * static_cast<float>(kScale));
        
        return (simd::Uint128)simd::ToInt128(omega * static_cast<float>(kScale));
    }

    static constexpr simd::Uint128 FloatPhase2Phase(simd::Float128 phase01) noexcept {
        // return static_cast<uint32_t>(phase01 * static_cast<float>(kScale));

        // 将范围从 [0, 1] 映射到 [-2147483648, 2147483647]
        // 也就是先乘以 2^32，再减去 2^31
        auto v_scale = static_cast<float>(kScale);
        auto v_bias = 2147483648.0f; // 2^31
        
        auto scaled = (phase01 * v_scale) - v_bias;
        
        // 现在值在有符号 int32 范围内，可以安全转换
        auto signed_vec = __builtin_convertvector(scaled, simd::Int128);
        
        // 最后加上这个 2^31 的偏移量（在整数域加回）
        // 0x80000000 是 uint32 的 2^31
        return (simd::Uint128)(signed_vec + 0x80000000);
    }

    static constexpr simd::Uint128 FreqPhaseInc(simd::Float128 f, simd::Float128 fs) noexcept {
        // return static_cast<uint32_t>(f * static_cast<float>(kScale) / fs);
        auto vf = f * static_cast<float>(kScale) / fs; // [0, 0.5]
        return (simd::Uint128)simd::ToInt128(vf);
    }
};
