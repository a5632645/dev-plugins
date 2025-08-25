#pragma once
#include <span>
#include <cmath>
#include <numbers>

namespace qwqdsp::window {
struct Hamming {
    // 单侧，w = pi * kMainLobeWidth / N
    static constexpr float kMainLobeWidth = 1.36f;
    static constexpr float k3dBWidth = 1.3f;
    static constexpr float kSideLobeAmp = -43.2f;
    static constexpr float kDecay = -20.0f;

    static void Window(std::span<float> x, bool for_analyze_not_fir) {
        const size_t N = x.size();
        if (for_analyze_not_fir) {
            for (size_t n = 0; n < N; ++n) {
                const float t = n / static_cast<float>(N) - 0.5f;
                x[n] = 0.53836f + 0.46164f * std::cos(std::numbers::pi_v<float> * 2 * t);
            }
        }
        else {
            for (size_t n = 0; n < N; ++n) {
                const float t = n / (N - 1.0f) - 0.5f;
                x[n] = 0.53836f + 0.46164f * std::cos(std::numbers::pi_v<float> * 2 * t);
            }
        }
    }

    static void DWindow(std::span<float> x) {
        const size_t N = x.size();
        for (size_t n = 0; n < N; ++n) {
            const float t = n / static_cast<float>(N) - 0.5f;
            x[n] = -0.46164f * std::numbers::pi_v<float> * 2 * std::sin(std::numbers::pi_v<float> * 2 * t);
        }
    }
};
}