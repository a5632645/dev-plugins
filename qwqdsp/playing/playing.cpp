#include "qwqdsp/filter/fir.hpp"
#include "qwqdsp/filter/window_fir.hpp"

int main() {
    qwqdsp::filter::FIRTranspose fir;
    fir.SetCoeff([](std::vector<float>& coeff) {
        coeff.resize(65);
        qwqdsp::filter::WindowFIR::Lowpass(coeff, std::numbers::pi_v<float> * 0.5f);
    });

    float test[70]{1.0f};
    fir.Process(test);
    auto c = fir.GetCoeff();
}