#pragma once
#include <numbers>
#include <span>
#include <cmath>

namespace qwqdsp::window {
struct Hann {
    // 单侧，w = 2pi * kMainLobeWidth / N
    static constexpr float kMainLobeWidth = 1.5f;
    static constexpr float k3dBWidth = 1.44f;
    static constexpr float kSideLobeAmp = -31.5f;
    static constexpr float kDecay = -60.0f;

    static void Window(std::span<float> x, bool for_analyze_not_fir) {
        const size_t N = x.size();
        if (for_analyze_not_fir) {
            for (size_t n = 0; n < N; ++n) {
                const float t = n / static_cast<float>(N);
                x[n] = 0.5 * (1.0 - cos(2.0 * std::numbers::pi_v<float> * t));
            }
        }
        else {
            for (size_t n = 0; n < N; ++n) {
                const float t = n / (N - 1.0f);
                x[n] = 0.5 * (1.0 - cos(2.0 * std::numbers::pi_v<float> * t));
            }
        }
    }

    static void DWindow(std::span<float> x) {
        const size_t N = x.size();
        for (size_t n = 0; n < N; ++n) {
            const float t = n / static_cast<float>(N);
            x[n] = std::numbers::pi_v<float> * 2 * std::sin(std::numbers::pi_v<float> * 2 * t);
        }
    }
};
}