#pragma once
#include <cstdint>
#include <limits>
#include <bit>

namespace qwqdsp::oscillor {
class WhiteNoise {
public:
    using SeedType = uint32_t;
    static constexpr float kScale = 1.0f / static_cast<float>(std::numeric_limits<uint32_t>::max());

    /**
     * @param seed 设置种子,直接改变寄存器的值
     */
    void SetSeed(uint32_t seed) noexcept {
        reg_ = seed;
    }

    /**
     * @return [0,1]之间的随机数
     */
    float Next01() noexcept {
        reg_ *= 1103515245;
        reg_ += 12345;
        return static_cast<float>(reg_) * kScale;
    }

    /**
     * @return [-1,1]之间的随机数
     */
    float Next() noexcept {
        auto e = Next01();
        return e * 2 - 1;
    }

    /**
     * @return 下一个32位数
     */
    uint32_t NextUInt() noexcept {
        reg_ *= 1103515245;
        reg_ += 12345;
        return reg_;
    }

    /**
     * @return 获取寄存器的值
     */
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
    /**
     * @param seed 寄存器的值
     */
    void SetSeed(uint32_t seed) noexcept {
        white_.SetSeed(seed);
    }

    WhiteNoise::SeedType GetSeed() const noexcept {
        return white_.GetReg();
    }

    void Reset() noexcept {
        b0_ = 0;
        b1_ = 0;
        b2_ = 0;
        b3_ = 0;
        b4_ = 0;
        b5_ = 0;
        b6_ = 0;
    }

    /**
     * @return 可能在[-1,1]之间的随机数
     */
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
    }

    WhiteNoise::SeedType GetSeed() const noexcept {
        return white_.GetReg();
    }

    void Reset() noexcept {
        update_phase_ = 0;
        for (auto& s : captrue_noise_) {
            s = 0;
        }
    }

    float Next() noexcept {
        uint16_t const old = update_phase_;
        ++update_phase_;
        uint32_t diff = update_phase_ ^ old;
        while (diff) {
            auto const pos = static_cast<uint32_t>(std::countr_zero(diff));
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
    uint16_t update_phase_{};
    float captrue_noise_[16]{};
};

class BrownNoise {
public:
    void SetSeed(uint32_t seed) noexcept {
        white_.SetSeed(seed);
    }

    WhiteNoise::SeedType GetSeed() const noexcept {
        return white_.GetReg();
    }

    void Reset() noexcept {
        latch_ = 0;
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
