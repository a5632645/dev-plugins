#pragma once
#include <cstddef>
#include <numbers>
#include <span>
#include <vector>

namespace qwqdsp::window {
struct Taylor {
    static void Window(std::span<float> window, std::span<float> dwindow, float side_lobe, int nbar) {
        const double amplification = pow(10.0, -side_lobe / 20.0);
        const double a = acosh(amplification) / std::numbers::pi_v<float>;
        const double a2 = a * a;
        const double sp2 = nbar * nbar / (a2 + (nbar - 0.5f) * (nbar - 0.5f));
        constexpr auto time_delta = 0.0001;
        const size_t N = window.size();
        std::vector<double> front_val(N, 1.0);
        std::vector<double> back_val(N, 1.0);
        for (int m = 1; m < nbar; ++m)
        {
            double numerator = 1.0;
            double denominator = 1.0;

            for (int i = 1; i < nbar; ++i)
            {
                numerator *= (1.0 - m * m / (sp2 * (a2 + (i - 0.5) * (i - 0.5))));
                if (i != m)
                {
                    denominator *= (1.0 - static_cast<float>(m) * m / i * i);
                }
            }

            const double Fm = -(numerator / denominator);

            for (size_t i = 0; i < N; ++i)
            {
                const double x = 2 * std::numbers::pi_v<float> * (i + 0.5) / N;
                const double front_x = 2 * std::numbers::pi_v<float> * ((i + 0.5) / N - time_delta);
                const double back_x = 2 * std::numbers::pi_v<float> * ((i + 0.5) / N + time_delta);
                window[i] += static_cast<float>(Fm * cos(m * x));
                front_val[i] += static_cast<float>(Fm * cos(m * front_x));
                back_val[i] += static_cast<float>(Fm * cos(m * back_x));
            }
        }

        if (!dwindow.empty()) {
            for (size_t i = 0; i < N; ++i)
            {
                dwindow[i] = static_cast<float>((back_val[i] - front_val[i]) / (2 * time_delta));
            }
        }
    }
};
}