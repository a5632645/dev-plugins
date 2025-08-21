#pragma once
#include <cassert>
#include <numeric>
#include <span>

namespace qwqdsp::window {
struct Helper {
    static void Normalize(std::span<float> x) {
        float gain = NormalizeGain(x);
        for (auto& v : x) {
            v *= gain;
        }
    }

    static float NormalizeGain(std::span<const float> x) {
        float gain = 2.0f / std::accumulate(x.begin(), x.end(), 0.0f);
        return gain;
    }

    static void TWindow(std::span<float> buffer, std::span<const float> window) {
        assert(buffer.size() == window.size());
        float offset = 0.5f * (window.size() - 1.0f);
        for (int k = 0; k < window.size(); ++k) {
            buffer[k] = window[k] * (k - offset);
        }
    }

    [[deprecated("not implement")]]
    static void ZeroPhasePad(std::span<float> output, std::span<const float> input) {
    }

    static void ZeroPad(std::span<float> output, std::span<const float> input) {
        assert(output.size() >= input.size());
        auto it = std::copy(input.begin(), input.end(), output.begin());
        std::fill(it, output.end(), 0.0f);
    }
};
}