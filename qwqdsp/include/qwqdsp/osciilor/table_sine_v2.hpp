#pragma once
#include <array>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <cstdint>

namespace qwqdsp::oscillor {
template<class T, size_t kTableBits = 16>
class TableSineV2 {
public:
    static_assert(kTableBits <= 16, "too large lookup table");

    static constexpr uint32_t kTableSize = 1 << kTableBits;
    static constexpr uint32_t kFracLen = 32 - kTableBits;
    static constexpr uint64_t kScale = 0x100000000;
    static constexpr uint32_t kQuadPhase = static_cast<uint32_t>(kScale / 4);
    static constexpr uint32_t kShift = kFracLen;

    inline static const std::array kSineTable = [] {
        std::array<T, kTableSize> r;
        for (uint32_t i = 0; i < kTableSize; ++i) {
            r[i] = std::sin(std::numbers::pi_v<T> * 2 * static_cast<T>(i) / static_cast<T>(kTableSize));
        }
        return r;
    }();

    T operator[](uint32_t phase) const noexcept {
        return kSineTable[phase >> kShift];
    }

    static T Sine(uint32_t phase) noexcept {
        return kSineTable[phase >> kShift];
    }

    static T Cosine(uint32_t phase) noexcept {
        uint32_t const t = phase + kQuadPhase;
        return kSineTable[t >> kShift];
    }

    static constexpr uint32_t Omega2PhaseInc(T omega) noexcept {
        omega /= std::numbers::pi_v<T> * 2;
        return static_cast<uint32_t>(omega * static_cast<T>(kScale));
    }
};
}