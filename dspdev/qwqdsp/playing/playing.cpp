#include <complex>
#include <array>
#include <numbers>

int main() {
    std::array<std::complex<double>, 2> poles{
        std::complex<double>{0.0, 0.1},
        {0.0, 0.2},
        // {0.0, 0.3},
        // {0.0, 0.4},
    };
    double G = 1;
    for (size_t i = 0; i < poles.size(); ++i) {
        G *= std::norm(poles[i]);
    }
    auto get_cascade_responce = [&poles](std::complex<double> z) {
        std::complex<double> r = 1;
        for (size_t i = 0; i < poles.size(); ++i) {
            auto p1 = poles[i];
            auto p2 = std::conj(p1);
            auto mul = (p1*z-1.0)/(z-p1)*(p2*z-1.0)/(p2-z);
            r *= mul;
        }
        return r;
    };

    std::array<std::complex<double>, poles.size()*2> residuals;
    for (size_t i = 0; i < poles.size(); ++i) {
        {
            std::complex<double> r = 1;
            auto z = poles[i];
            for (size_t j = 0; j < poles.size(); ++j) {
                auto p1 = poles[j];
                auto p2 = std::conj(p1);
                if (i == j) {
                    r *= (p1*z-1.0)/(z-p2)*(p2*z-1.0);
                }
                else {
                    r *= (p1*z-1.0)/(z-p2)*(p2*z-1.0)/(z-p1);
                }
            }
            residuals[2 * i] = r;
        }
        {
            std::complex<double> r = 1;
            auto z = std::conj(poles[i]);
            for (size_t j = 0; j < poles.size(); ++j) {
                auto p1 = poles[j];
                auto p2 = std::conj(p1);
                if (i == j) {
                    r *= (p1*z-1.0)/(z-p1)*(p2*z-1.0);
                }
                else {
                    r *= (p1*z-1.0)/(z-p2)*(p2*z-1.0)/(z-p1);
                }
            }
            residuals[2 * i + 1] = r;
        }
    }
    auto get_paralle_responce = [&residuals, &poles, G](std::complex<double> z) {
        double scale = 10.0;
        std::complex<double> r = G * scale;
        for (size_t i = 0; i < poles.size(); ++i) {
            r += residuals[2 * i] * scale / (z - poles[i]);
            r += residuals[2 * i + 1] * scale / (z - std::conj(poles[i]));
        }
        return r;
    };

    std::array<double, 1024> cascade_gain;
    std::array<double, 1024> paralle_gain;
    std::array<double, 1024> cascade_phase;
    std::array<double, 1024> paralle_phase;
    for (size_t i = 0; i < 1024; ++i) {
        auto const w = i * std::numbers::pi_v<double> / 1024.0;
        auto z = std::polar(1.0, w);
        cascade_gain[i] = std::abs(get_cascade_responce(z));
        cascade_phase[i] = std::arg(get_cascade_responce(z));
        paralle_gain[i] = std::abs(get_paralle_responce(z));
        paralle_phase[i] = std::arg(get_paralle_responce(z));
    }
}