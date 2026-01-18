#pragma once
#include <cassert>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <span>

namespace qwqdsp_window {
struct Kaiser {
    // 和分析有关的
    // f = width / N
    static float MainLobeWidth(float beta) noexcept {
        float a = beta / std::numbers::pi_v<float>;
        return std::sqrt(1.0f + a * a);
    }
    static constexpr float kSidelobeRolloff = -6.0f;
    // 和滤波器设计有关的
    // 卷积之后第一个旁瓣的大小
    // qwqfixme: 可能需要补充信息
    // static constexpr float kStopband = -53.0f;
    // static constexpr float kTransmit = 3.3f;

    static float Bessel0(float x) {
#if __APPLE__
        const float significanceLimit = 1e-4f;
        float result = 0;
        float term = 1;
        float m = 0;
        while (term > significanceLimit) {
            result += term;
            ++m;
            term *= (x * x) / (4 * m * m);
        }

        return result;
#else
        return std::cyl_bessel_i(0.0f, x);
#endif
    }

    static inline float Bessel1(float x) {
#if __APPLE__
        if (std::abs(x) < 1e-7f) return 0.0f;
        float sum = 1.0f;
        float term = 1.0f;
        float x2 = x * x / 4.0f;
        for (int n = 1; n <= 25; ++n) {
            // 系数公式：term = (x/2) * Σ [ (x^2/4)^n / (n! * (n+1)!) ]
            term *= x2 / (n * (n + 1));
            sum += term;
            if (term < sum * 1e-7f) break;
        }
        return (x / 2.0f) * sum;
#else
        return std::cyl_bessel_i(1.0f, x);
#endif
    }

    static void Window(std::span<float> window, float beta, bool for_analyze_not_fir) noexcept {
        const size_t N = window.size();
        if (for_analyze_not_fir) {
            auto down = 1.0f / Bessel0(beta);
            for (size_t i = 0; i < N; ++i) {
                auto t = static_cast<float>(i) / static_cast<float>(N);
                t = 2 * t - 1;
                auto arg = std::sqrt(1.0f - t * t);
                window[i] = Bessel0(beta * arg) * down;
            }
        }
        else {
            auto down = 1.0f / Bessel0(beta);
            for (size_t i = 0; i < N; ++i) {
                auto t = static_cast<float>(i) / (static_cast<float>(N) - 1.0f);
                t = 2 * t - 1;
                auto arg = std::sqrt(1.0f - t * t);
                window[i] = Bessel0(beta * arg) * down;
            }
        }
    }

    static void ApplyWindow(std::span<float> x, float beta, bool for_analyze_not_fir) noexcept {
        const size_t N = x.size();
        if (for_analyze_not_fir) {
            auto down = 1.0f / Bessel0(beta);
            for (size_t i = 0; i < N; ++i) {
                auto t = static_cast<float>(i) / static_cast<float>(N);
                t = 2 * t - 1;
                auto arg = std::sqrt(1.0f - t * t);
                x[i] *= Bessel0(beta * arg) * down;
            }
        }
        else {
            auto down = 1.0f / Bessel0(beta);
            for (size_t i = 0; i < N; ++i) {
                auto t = static_cast<float>(i) / (static_cast<float>(N) - 1.0f);
                t = 2 * t - 1;
                auto arg = std::sqrt(1.0f - t * t);
                x[i] *= Bessel0(beta * arg) * down;
            }
        }
    }

    /**
     * @note 此方法只会生成周期性，也就是for analyze
     */
    static void Window(std::span<float> window, std::span<float> dwindow, float beta) noexcept {
        constexpr auto kTimeDelta = 0.001f;
        const size_t N = window.size();

        auto down = 1.0f / Bessel0(beta);
        for (size_t i = 0; i < N; ++i) {
            auto t = static_cast<float>(i) / static_cast<float>(N);
            t = 2 * t - 1;

            auto arg = std::sqrt(1.0f - t * t);
            window[i] = Bessel0(beta * arg) * down;
            if (i == 0) {
                dwindow.front() =
                    (Bessel0(beta * std::sqrt(1.0f - (t + kTimeDelta) * (t + kTimeDelta))) * down - window.front())
                    / kTimeDelta;
            }
            else if (i == N - 1) {
                dwindow.back() =
                    (Bessel0(beta * std::sqrt(1.0f - (t - kTimeDelta) * (t - kTimeDelta))) * down - window.back())
                    / -kTimeDelta;
            }
            else {
                dwindow[i] = Bessel1(beta * arg) * beta * (-t / arg) * down;
            }
        }
    }

    /**
     * @param side_lobe >0
     * @ref https://ww2.mathworks.cn/help/signal/ref/kaiser.html
     */
    static float Beta(float side_lobe) noexcept {
        assert(side_lobe > 0);
        if (side_lobe < 21.0f) {
            return 0.0f;
        }
        else if (side_lobe <= 50.0f) {
            return 0.5842f * std::pow(side_lobe - 21.0f, 0.4f) + 0.07886f * (side_lobe - 21.0f);
        }
        else {
            return 0.1102f * (side_lobe - 8.7f);
        }
    }
};
} // namespace qwqdsp_window
