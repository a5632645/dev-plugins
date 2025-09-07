#include "qwqdsp/filter/median.hpp"

int main() {
    qwqdsp::filter::Median<double, 3> m;
    double test[9] = { 1.0f, 4.0f, 1.0f, 0.0f, 2.0f, 2.0f, 2.0f, 1.0f, 1.0f };
    double out[9];
    for (size_t i = 0; i < 9; ++i) {
        out[i] = m.Tick(test[i], [](double a, double b) noexcept {
            return a < b;
        });
    }
}