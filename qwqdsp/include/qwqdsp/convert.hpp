#pragma once
#include <numbers>
#include <cmath>

namespace qwqdsp::convert {
static constexpr float Freq2W(float f, float fs) {
    return f * std::numbers::pi_v<float> * 2 / fs;
}
}