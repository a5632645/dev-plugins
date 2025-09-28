#pragma once
#include <cstdint>
#include <limits>
#include <bit>

namespace qwqdsp::oscillor {
class WhiteNoise {
public:
    void SetSeed(uint32_t seed) noexcept {
        reg_ = seed;
    }

    float Next01() noexcept {
        reg_ *= 1103515245;
        reg_ += 12345;
        return static_cast<float>(reg_) / static_cast<float>(std::numeric_limits<uint32_t>::max());
    }

    float Next() noexcept {
        auto e = Next01();
        return e * 2 - 1;
    }

    uint32_t NextUInt() noexcept {
        reg_ *= 1103515245;
        reg_ += 12345;
        return reg_;
    }

    uint32_t GetReg() const noexcept {
        return reg_;
    }
private:
    uint32_t reg_{};
};

/**
 * @brief Paul Kellet 在 Allan 分析中的改进方法
 * @ref https://www.firstpr.com.au/dsp/pink-noise/
 */
class PinkNoise {
public:
    void SetSeed(uint32_t seed) noexcept {
        white_.SetSeed(seed);
    }

    float Next() noexcept {
        float const white = white_.Next();
        b0_ = 0.99886f * b0_ + white * 0.0555179f;
        b1_ = 0.99332f * b1_ + white * 0.0750759f;
        b2_ = 0.96900f * b2_ + white * 0.1538520f;
        b3_ = 0.86650f * b3_ + white * 0.3104856f;
        b4_ = 0.55000f * b4_ + white * 0.5329522f;
        b5_ = -0.7616f * b5_ - white * 0.0168980f;
        float const pink = b0_ + b1_ + b2_ + b3_ + b4_ + b5_ + b6_ + white * 0.5362f;
        b6_ = white * 0.115926f;
        return pink * 0.25f;
    }
private:
    WhiteNoise white_;
    float b0_{};
    float b1_{};
    float b2_{};
    float b3_{};
    float b4_{};
    float b5_{};
    float b6_{};
};

/**
 * @brief Voss-McCartney 算法
 * @ref https://www.firstpr.com.au/dsp/pink-noise/
 */
class PinkNoiseHQ {
public:
    void SetSeed(uint32_t seed) noexcept {
        white_.SetSeed(seed);
        for (auto& s : captrue_noise_) {
            s = white_.Next();
        }
    }

    float Next() noexcept {
        uint32_t const old = update_phase_;
        ++update_phase_;
        uint32_t diff = update_phase_ ^ old;
        while (diff) {
            // uint32_t const pos = __builtin_ctzl(diff);
            auto const pos = std::countr_zero(diff);
            captrue_noise_[pos] = white_.Next();
            diff &= ~(static_cast<uint32_t>(1) << pos);
        }

        float sum{};
        for (auto const x : captrue_noise_) {
            sum += x;
        }
        return sum / 8.0f;
    }
private:
    WhiteNoise white_;
    uint32_t update_phase_{};
    float captrue_noise_[32]{};
};

class BrownNoise {
public:
    void SetSeed(uint32_t seed) noexcept {
        white_.SetSeed(seed);
    }

    float Next() noexcept {
        // 可以用一个更好的积分器
        latch_ = 0.99f * latch_ + 0.01f * white_.Next();
        return latch_ * 8.0f;
    }
private:
    WhiteNoise white_;
    float latch_{};
};
} // namespace dsp
