#pragma once
#include <cstdint>
#include <limits>

namespace qwqdsp::oscillor {
class WhiteNoise {
public:
    void SetSeed(uint32_t seed) {
        reg_ = seed;
    }

    float Next01() {
        reg_ *= 1103515245;
        reg_ += 12345;
        return reg_ / static_cast<float>(std::numeric_limits<uint32_t>::max());
    }

    float Next() {
        auto e = Next01();
        return e * 2 - 1;
    }

    float Lowpassed() {
        float last = reg_ / static_cast<float>(std::numeric_limits<uint32_t>::max());
        float curr = Next01();
        return (last + curr) * 0.5f;
    }

    float Lowpassed01() {
        return Lowpassed() * 0.5f - 0.5f;
    }

    uint32_t NextUInt() {
        reg_ *= 1103515245;
        reg_ += 12345;
        return reg_;
    }

    uint32_t GetReg() const {
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
