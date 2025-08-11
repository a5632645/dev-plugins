#pragma once
#include <numbers>
#include <span>
#include <cmath>

namespace qwqdsp::window {
struct Hann {
    static void Window(std::span<float> x) {
        const size_t N = x.size();
        for (size_t n = 0; n < N; ++n) {
            const float t = n / (N - 1.0f);
            x[n] = 0.5 * (1.0 - cos(2.0 * std::numbers::pi_v<float> * t));
        }
    }

    static void DWindow(std::span<float> x) {
        const size_t N = x.size();
        for (size_t n = 0; n < N; ++n) {
            const float t = n / (N - 1.0f);
            x[n] = std::numbers::pi_v<float> * 2 * std::sin(std::numbers::pi_v<float> * 2 * t);
        }
    }
};
}