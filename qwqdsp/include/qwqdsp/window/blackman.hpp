#pragma once
#include <numbers>
#include <span>
#include <cmath>

namespace qwqdsp::window {
struct Blackman {
    static void Window(std::span<float> x) {
        const size_t N = x.size();
        constexpr float twopi = std::numbers::pi_v<float> * 2;
        constexpr float a0 = 0.42659f;
        constexpr float a1 = 0.496562f;
        constexpr float a2 = 0.076849f; 
        for (size_t n = 0; n < N; ++n) {
            const float t = n / (N - 1.0f);
            x[n] = a0 - a1 * std::cos(twopi * t) + a2 * std::cos(twopi * 2 * t);
        }
    }

    static void DWindow(std::span<float> x) {
        const size_t N = x.size();
        constexpr float twopi = std::numbers::pi_v<float> * 2;
        constexpr float a1 = 0.496562f;
        constexpr float a2 = 0.076849f; 
        for (size_t n = 0; n < N; ++n) {
            const float t = n / (N - 1.0f);
            x[n] = a1 * twopi * std::sin(twopi * t) - a2 * twopi * 2 * std::sin(twopi * 2 * t);
        }
    }
};
}