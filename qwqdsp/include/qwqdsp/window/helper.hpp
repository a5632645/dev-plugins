#pragma once
#include <cassert>
#include <numeric>
#include <span>

namespace qwqdsp::window {
struct Helper {
    static void Normalize(std::span<float> x) noexcept {
        float gain = NormalizeGain(x);
        for (auto& v : x) {
            v *= gain;
        }
    }

    static float NormalizeGain(std::span<const float> x) noexcept {
        float gain = 2.0f / std::accumulate(x.begin(), x.end(), 0.0f);
        return gain;
    }

    static void TWindow(std::span<float> buffer, std::span<const float> window) noexcept {
        assert(buffer.size() == window.size());
        float offset = 0.5f * window.size();
        for (int k = 0; k < window.size(); ++k) {
            buffer[k] = window[k] * (k - offset);
        }
    }

    static void ZeroPhasePad(std::span<float> output, std::span<const float> input) noexcept {
        assert(input.size() % 2 == 1); 
        assert(output.size() >= input.size());
        auto k = (input.size() - 1) / 2;
        std::fill(output.begin(), output.end(), 0.0f);
        std::copy_n(input.begin(), k, output.begin() + (output.size() - k));
        std::copy(input.begin() + k, input.end(), output.begin());
    }

    static void ZeroPad(std::span<float> output, std::span<const float> input) noexcept {
        assert(output.size() >= input.size());
        auto it = std::copy(input.begin(), input.end(), output.begin());
        std::fill(it, output.end(), 0.0f);
    }
};
}