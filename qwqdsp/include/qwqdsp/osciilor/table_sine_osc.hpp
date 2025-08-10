#pragma once
#include <array>
#include <cmath>
#include <limits>
#include <numbers>
#include <cstdint>

namespace qwqdsp {
/**
 * @brief 查表正弦波，频率精度比计算方法较高，效率比计算方法慢一倍(SIMD256)
 */
class TableSineOsc {
public:
    static constexpr uint32_t kNumBits = 16;
    static constexpr uint32_t kSize = 1 << kNumBits;
    static constexpr uint32_t kMask = kSize - 1;

    inline static const std::array kSineTable = [] {
        std::array<float, kSize> r;
        for (uint32_t i = 0; i < kSize; ++i) {
            r[i] = std::sin(std::numbers::pi_v<float> * 2.0f * i / kSize);
        }
        return r;
    }();

    void SetFreq(float f, float fs) {
        inc_ = static_cast<uint32_t>(f * static_cast<float>(std::numeric_limits<uint32_t>::max()) / fs);
    }

    float Tick() {
        phase_ += inc_;
        return kSineTable[phase_ >> 16];
    }

    void Reset(float phase) {
        phase_ = static_cast<uint32_t>(phase / std::numbers::pi_v<float> / 2.0 * static_cast<float>(std::numeric_limits<uint32_t>::max()));
    }

private:
    uint32_t phase_{};
    uint32_t inc_{};
};
}