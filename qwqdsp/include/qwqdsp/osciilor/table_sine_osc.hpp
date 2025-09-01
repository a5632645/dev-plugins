#pragma once
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <numbers>
#include <cstdint>
#include <complex>

namespace qwqdsp::oscillor {
/**
 * @brief 查表正弦波，频率精度比计算方法较高，效率比计算方法在大SIMD下较慢
 *  精度   伪影(输出=0dB，频率=250hz@48khz)
 *   16     -102.61dB
 *   15      -97.56dB
 *   14      -90.31dB
 *   13      -84.60dB
 *   12      -78.89dB
 *   11      -72.47dB
 *   10      -66.59dB
 *    9      -60.66dB
 *    8      -54.72dB
 */
template<size_t kTableBits = 16>
    requires requires {kTableBits <= 24;}
class TableSineOsc {
public:
    static constexpr uint32_t kTableSize = 1 << kTableBits;
    static constexpr uint32_t kFracLen = 32 - kTableBits;
    static constexpr uint32_t kScale = std::numeric_limits<uint32_t>::max();
    static constexpr uint32_t kShift = kFracLen;

    inline static const std::array kSineTable = [] {
        std::array<float, kTableSize> r;
        for (uint32_t i = 0; i < kTableSize; ++i) {
            r[i] = std::sin(std::numbers::pi_v<float> * 2.0f * static_cast<float>(i) / static_cast<float>(kTableSize));
        }
        return r;
    }();

    void SetFreq(float f, float fs) {
        inc_ = static_cast<uint32_t>(f * static_cast<float>(kScale) / fs);
    }

    void SetFreq(float omega) {
        omega /= std::numbers::pi_v<float> * 2.0f;
        inc_ = static_cast<uint32_t>(omega * static_cast<float>(kScale));
    }

    float Tick() {
        phase_ += inc_;
        return kSineTable[phase_ >> kShift];
    }

    float Cosine() const {
        uint32_t t = phase_ + kScale / 4;
        return kSineTable[t >> kShift];
    }

    std::complex<float> GetCpx() const {
        return {Cosine(), kSineTable[phase_ >> kShift]};
    }

    std::complex<float> GetNPhaseCpx(size_t n) const {
        uint32_t t1 = phase_ * n;
        uint32_t t2 = t1 + kScale / 4;
        return {kSineTable[t2 >> kShift], kSineTable[t1 >>kShift]};
    }

    void Reset(float phase) {
        phase /= std::numbers::pi_v<float> * 2.0f;
        phase_ = static_cast<uint32_t>(phase * static_cast<float>(kScale));
    }

    void Reset() {
        phase_ = 0;
    }
private:
    uint32_t phase_{};
    uint32_t inc_{};
};
}