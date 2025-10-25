#include <cstddef>
#include <numbers>

#include "qwqdsp/fx/uniform_convolution.hpp"

int main() {
    float sin[513];
    for (size_t i = 0; i < 513; ++i) {
        sin[i] = std::sin(std::numbers::pi_v<float> * 2 * i / 513.0f);
    }

    qwqdsp::fx::UniformConvolution conv;
    conv.Init(256);
    conv.SetIR(sin);

    float test[4096]{1.0f};
    test[1024] = 1;
    conv.Process(test);
}
