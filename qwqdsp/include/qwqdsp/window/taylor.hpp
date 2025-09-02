#pragma once
#include <cstddef>
#include <numbers>
#include <span>
#include <vector>

namespace qwqdsp::window {
struct Taylor {
    static constexpr float sq(float x) noexcept {
        return x * x;
    }

    /**
     * @param side_lobe > 0
     * @note 如果要用于分析，请使用N+1的窗然后丢弃最后一个样本
     */
    static void Window(std::span<float> window, float side_lobe, size_t nbars) noexcept {
        const double amplification = pow(10.0f, side_lobe / 20.0f);
        const double a = acosh(amplification) / std::numbers::pi_v<float>;
        const double a2 = sq(a);
        const double sp2 = sq(nbars) / (a2 + sq(nbars - 0.5));
        for (size_t i = 0; i < window.size(); ++i) {
            window[i] = 1.0;
        }

        for (size_t m = 1; m < nbars; ++m) {
            double numerator = 1.0;
            double denominator = 1.0;

            for (size_t i = 1; i < nbars; ++i) {
                numerator *= (1.0 - sq(m) / (sp2 * (a2 + sq(i - 0.5))));
                if (i != m) {
                    denominator *= (1.0 - sq(m) / sq(i));
                }
            }

            const double Fm = -(numerator / denominator);

            for (size_t i = 0; i < window.size(); ++i) {
                const double x = 2 * std::numbers::pi_v<float> * (i + 0.5) / window.size();
                window[i] += static_cast<float>(Fm * cos(m * x));
            }
        }
    }

    /**
     * @param side_lobe > 0
     */
    static void Window(std::span<float> window, std::span<float> dwindow, float side_lobe, size_t nbars) {
        const double amplification = pow(10.0f, side_lobe / 20.0f);
        const double a = acosh(amplification) / std::numbers::pi_v<float>;
        const double a2 = sq(a);
        const double sp2 = sq(nbars) / (a2 + sq(nbars - 0.5));
        for (size_t i = 0; i < window.size(); ++i) {
            window[i] = 1.0;
        }

        constexpr auto time_delta = 0.0001;
        std::vector<double> front_val(window.size(), 1.0);
        std::vector<double> back_val(window.size(), 1.0);
        for (size_t m = 1; m < nbars; ++m) {
            double numerator = 1.0;
            double denominator = 1.0;

            for (size_t i = 1; i < nbars; ++i) {
                numerator *= (1.0 - sq(m) / (sp2 * (a2 + sq(i - 0.5))));
                if (i != m) {
                    denominator *= (1.0 - sq(m) / sq(i));
                }
            }

            const double Fm = -(numerator / denominator);

            for (size_t i = 0; i < window.size(); ++i) {
                const double x = 2 * std::numbers::pi_v<float> * (i + 0.5) / window.size();
                const double front_x = 2 * std::numbers::pi_v<float> * ((i + 0.5) / window.size() - time_delta);
                const double back_x = 2 * std::numbers::pi_v<float> * ((i + 0.5) / window.size() + time_delta);
                window[i] += static_cast<float>(Fm * cos(m * x));
                front_val[i] += static_cast<float>(Fm * cos(m * front_x));
                back_val[i] += static_cast<float>(Fm * cos(m * back_x));
            }
        }

        for (size_t i = 0; i < window.size(); ++i) {
            dwindow[i] = static_cast<float>((back_val[i] - front_val[i]) / (2 * time_delta));
        }
    }
};
}