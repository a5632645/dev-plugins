#pragma once
#include <cstdint>
#include <limits>

namespace qwqdsp::oscillor {
class WhiteNoise {
public:
    void SetSeed(uint32_t seed) noexcept {
        reg_ = seed;
    }

    float Next01() noexcept {
        reg_ *= 1103515245;
        reg_ += 12345;
        return reg_ / static_cast<float>(std::numeric_limits<uint32_t>::max());
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

class [[deprecated("not implement")]] PinkNoise {

};

class [[deprecated("not implement")]] BrownNoise {

};
} // namespace dsp
