#pragma once
#include <numeric>
#include <span>

namespace qwqdsp::window {
struct Helper {
    static void Normalize(std::span<float> x) {
        float gain = 2.0f / std::accumulate(x.begin(), x.end(), 0.0f);
        for (auto& v : x) {
            v *= gain;
        }
    }

    static float NormalizeGain(std::span<const float> x) {
        float gain = 2.0f / std::accumulate(x.begin(), x.end(), 0.0f);
        return gain;
    }

    static void TWindow(std::span<float> buffer, std::span<const float> window) {
        float offset = 0.5f * (window.size() - 1.0f);
        for (int k = 0; k < window.size(); ++k) {
            buffer[k] = window[k] * (k - offset);
        }
    }

    static void ZeroPhasePad(std::span<float> output, std::span<const float> input) {
    }

    static void ZeroPad(std::span<float> output, std::span<const float> input) {
    }
};
}