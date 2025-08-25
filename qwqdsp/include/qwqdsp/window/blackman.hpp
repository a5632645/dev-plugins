#pragma once
#include <numbers>
#include <span>
#include <cmath>

namespace qwqdsp::window {
struct Blackman {
    // 单侧，w = 2pi * kMainLobeWidth / N
    static constexpr float kMainLobeWidth = 2.0f;
    static constexpr float k3dBWidth = 1.68f;
    static constexpr float kSideLobeAmp = -92.2f;
    static constexpr float kDecay = -20.0f;

    static void Window(std::span<float> x, bool for_analyze_not_fir) {
        const size_t N = x.size();
        constexpr float twopi = std::numbers::pi_v<float> * 2;
        constexpr float a0 = 0.42659f;
        constexpr float a1 = 0.496562f;
        constexpr float a2 = 0.076849f; 
        if (for_analyze_not_fir) {
            for (size_t n = 0; n < x.size(); ++n) {
                const float t = n / static_cast<float>(N);
                x[n] = a0 - a1 * std::cos(twopi * t) + a2 * std::cos(twopi * 2 * t);
            }
        }
        else {
            for (size_t n = 0; n < x.size(); ++n) {
                const float t = n / (N - 1.0f);
                x[n] = a0 - a1 * std::cos(twopi * t) + a2 * std::cos(twopi * 2 * t);
            }
        }
    }

    static void DWindow(std::span<float> x) {
        const size_t N = x.size();
        constexpr float twopi = std::numbers::pi_v<float> * 2;
        constexpr float a1 = 0.496562f;
        constexpr float a2 = 0.076849f; 
        for (size_t n = 0; n < N; ++n) {
            const float t = static_cast<float>(n) / N;
            x[n] = a1 * twopi * std::sin(twopi * t) - a2 * twopi * 2 * std::sin(twopi * 2 * t);
        }
    }
};
}