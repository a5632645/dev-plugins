#pragma once
#include <numbers>
#include <span>
#include <cmath>

namespace qwqdsp::window {
struct Hann {
    // 和分析有关的
    // f = width / N
    static constexpr float kMainlobeWidth = 3.0f;
    static constexpr float kSidelobe = -31.5565f;
    static constexpr float kSidelobeRolloff = -18.0f;
    // 和滤波器设计有关的
    // 卷积之后第一个旁瓣的大小
    static constexpr float kStopband = -44.0f;
    static constexpr float kTransmit = 3.1f;

    static void Window(std::span<float> x, bool for_analyze_not_fir) noexcept {
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

    static void ApplyWindow(std::span<float> x, bool for_analyze_not_fir) noexcept {
        const size_t N = x.size();
        if (for_analyze_not_fir) {
            for (size_t n = 0; n < N; ++n) {
                const float t = n / static_cast<float>(N);
                x[n] *= 0.5 * (1.0 - cos(2.0 * std::numbers::pi_v<float> * t));
            }
        }
        else {
            for (size_t n = 0; n < N; ++n) {
                const float t = n / (N - 1.0f);
                x[n] *= 0.5 * (1.0 - cos(2.0 * std::numbers::pi_v<float> * t));
            }
        }
    }

    static void DWindow(std::span<float> x) noexcept {
        const size_t N = x.size();
        for (size_t n = 0; n < N; ++n) {
            const float t = n / static_cast<float>(N);
            x[n] = std::numbers::pi_v<float> * 2 * std::sin(std::numbers::pi_v<float> * 2 * t);
        }
    }
};
}