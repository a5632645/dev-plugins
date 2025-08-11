#pragma once
#include <span>
#include <cmath>

namespace qwqdsp::window {
struct Helper {
    static void Normalize(std::span<float> x) {
        float e = 0.0f;
        for (auto s : x) {
            e += s * s;
        }
        float gain = 1.0f / std::sqrt(e);
        for (auto& s : x) {
            s *= gain;
        }
    }
};
}