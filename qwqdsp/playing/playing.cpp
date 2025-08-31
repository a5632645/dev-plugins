#include <cmath>
#include <numbers>

#include "qwqdsp/filter/median.hpp"

int main() {
    qwqdsp::filter::Median median;
    median.Init(5);

    float test[] = {
        1, 1, 2, 6, 2, 3, 1, 0, 4, 1, 2, -1
    };

    size_t const size = std::size(test);
    float output[size]{};
    for (size_t i = 0; i < size; ++i) {
        output[i] = median.Tick(test[i]);
    }
}