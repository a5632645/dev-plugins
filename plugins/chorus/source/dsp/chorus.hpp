#pragma once
#include <array>

#include "Pitcher.h"

namespace chorus {

class Chorus {
public:
    void Init(float fs);
    void Process(float* left, float* right, size_t num_samples);

    void Update();

    float detune{};
    float spread{};
private:
    std::array<Pitcher, 16> pitch_shifter_;
};

} // namespace chorus
