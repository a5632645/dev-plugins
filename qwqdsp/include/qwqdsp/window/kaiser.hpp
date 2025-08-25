#pragma once
#include <cstddef>
#include <numbers>
#include <span>
#include <cmath>

namespace qwqdsp::window {
struct Kaiser {
    // 单侧，w = pi * kMainLobeWidth / N
    static float MainLobeWidth(float beta) {
        float a = beta / std::numbers::pi_v<float>;
        return std::sqrt(1.0f + a * a);
    }

    static void Window(std::span<float> window, float beta, bool for_analyze_not_fir) {
        const size_t N = window.size();
        auto inc = 2.0f / (N - 1.0f);
        if (for_analyze_not_fir) {
            inc = 2.0f / N;
        }
        auto down = 1.0f / std::cyl_bessel_i(0, beta);
        for (size_t i = 0; i < N; ++i) {
            auto t = -1.0f + i * inc;
            auto arg = std::sqrt(1.0 - t * t);
            window[i] = std::cyl_bessel_i(0, beta * arg) * down;
        }
    }

    static void Window(std::span<float> window, std::span<float> dwindow, float beta) {
        constexpr auto kTimeDelta = 0.001f;
        const size_t N = window.size();
        auto inc = 2.0f / N;
        auto down = 1.0f / std::cyl_bessel_i(0, beta);
        for (size_t i = 0; i < N; ++i) {
            auto t = -1.0f + i * inc;
            auto arg = std::sqrt(1.0 - t * t);
            window[i] = std::cyl_bessel_i(0, beta * arg) * down;
            if (i == 0) {
                dwindow.front() = (std::cyl_bessel_i(0, beta * std::sqrt(1.0 - (t + kTimeDelta) * (t + kTimeDelta))) * down - window.front()) / kTimeDelta;
            }
            else if (i == N - 1) {
                dwindow.back() = (std::cyl_bessel_i(0, beta * std::sqrt(1.0 - (t - kTimeDelta) * (t - kTimeDelta))) * down - window.back()) / -kTimeDelta;
            }
            else {
                dwindow[i] = std::cyl_bessel_i(1, beta * arg) * beta * (-t / arg) * down;
            }
        }
    }

    /**
     * @param side_lobe >0
     * @ref https://ww2.mathworks.cn/help/signal/ref/kaiser.html
     */
    static float Beta(float side_lobe) {
        if (side_lobe < 21.0) {
            return 0.0;
        }
        else if (side_lobe <= 50.0) {
            return 0.5842 * std::pow(side_lobe - 21.0, 0.4)
                + 0.07886 * (side_lobe - 21.0);
        }
        else {
            return 0.1102 * (side_lobe - 8.7);
        }
    }
};
}