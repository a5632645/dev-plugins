#pragma once
#include <numbers>
#include <span>
#include <cmath>

namespace qwqdsp::window {
struct Blackman {
    // 和分析有关的
    // f = width / N
    static constexpr float kMainlobeWidth = 3.0f;
    static constexpr float kSidelobe = -58.2336f;
    static constexpr float kSidelobeRolloff = -18.0f;
    // 和滤波器设计有关的
    // 卷积之后第一个旁瓣的大小
    static constexpr float kStopband = -74.0f;
    static constexpr float kTransmit = 5.5f;

    static void Window(std::span<float> x, bool for_analyze_not_fir) noexcept {
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

    static void ApplyWindow(std::span<float> x, bool for_analyze_not_fir) noexcept {
        const size_t N = x.size();
        constexpr float twopi = std::numbers::pi_v<float> * 2;
        constexpr float a0 = 0.42659f;
        constexpr float a1 = 0.496562f;
        constexpr float a2 = 0.076849f; 
        if (for_analyze_not_fir) {
            for (size_t n = 0; n < x.size(); ++n) {
                const float t = n / static_cast<float>(N);
                x[n] *= a0 - a1 * std::cos(twopi * t) + a2 * std::cos(twopi * 2 * t);
            }
        }
        else {
            for (size_t n = 0; n < x.size(); ++n) {
                const float t = n / (N - 1.0f);
                x[n] *= a0 - a1 * std::cos(twopi * t) + a2 * std::cos(twopi * 2 * t);
            }
        }
    }

    static void DWindow(std::span<float> x) noexcept {
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