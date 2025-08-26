#pragma once
#include <cstddef>
#include <span>
#include <numbers>
#include <cmath>

namespace qwqdsp::window {
struct Lanczos {
    // 和分析有关的
    // f = width / N
    static constexpr float kMainlobeWidth = 1.625f;
    static constexpr float kSidelobe = -26.5935f;
    static constexpr float kSidelobeRolloff = -12.0f;
    // 和滤波器设计有关的
    // 卷积之后第一个旁瓣的大小
    // static constexpr float kStopband = -74.0f;
    // static constexpr float kTransmit = 5.5f;

    static void Window(std::span<float> x, bool for_analyze_not_fir) {
        if (for_analyze_not_fir) {
            const size_t N = x.size();
            for (size_t i = 0; i < N; ++i) {
                x[i] = Sinc(2.0 * i / N - 1.0f);
            }
        }
        else {
            const size_t N = x.size();
            for (size_t i = 0; i < N; ++i) {
                x[i] = Sinc(2.0 * i / (N - 1.0f) - 1.0f);
            }
        }
    }

    static void ApplyWindow(std::span<float> x, bool for_analyze_not_fir) {
        if (for_analyze_not_fir) {
            const size_t N = x.size();
            for (size_t i = 0; i < N; ++i) {
                x[i] *= Sinc(2.0 * i / N - 1.0f);
            }
        }
        else {
            const size_t N = x.size();
            for (size_t i = 0; i < N; ++i) {
                x[i] *= Sinc(2.0 * i / (N - 1.0f) - 1.0f);
            }
        }
    }
private:
    static float Sinc(float x) {
        x *= std::numbers::pi_v<float>;
        if (x == 0.0f) {
            return 1.0f;
        }
        else {
            return std::sin(x) / x;
        }
    }
};
}