#pragma once
#include <span>
#include <cmath>
#include <numbers>

namespace qwqdsp::window {
struct Hamming {
    static void Window(std::span<float> x) {
        const size_t N = x.size();
        for (size_t n = 0; n < N; ++n) {
            const float t = n / (N - 1.0f) - 0.5f;
            x[n] = 0.53836f + 0.46164f * std::cos(std::numbers::pi_v<float> * 2 * t);
        }
    }

    static void DWindow(std::span<float> x) {
        const size_t N = x.size();
        for (size_t n = 0; n < N; ++n) {
            const float t = n / (N - 1.0f) - 0.5f;
            x[n] = -0.46164f * std::numbers::pi_v<float> * 2 * std::sin(std::numbers::pi_v<float> * 2 * t);
        }
    }
};
}