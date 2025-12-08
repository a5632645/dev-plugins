#include <cstddef>
#include <numbers>

#include "qwqdsp/window/helper.hpp"
#include "qwqdsp/spectral/real_fft.hpp"
#include "qwqdsp/window/hamming.hpp"
#include "qwqdsp/filter/window_fir.hpp"

int main() {
    float test[65];
    qwqdsp_filter::WindowFIR::Bandstop(test, std::numbers::pi_v<float> / 4, std::numbers::pi_v<float> / 2);
    qwqdsp_window::Hamming::ApplyWindow(test, false);

    float pading[1024];
    std::complex<float> spectral[513];
    qwqdsp_window::Helper::ZeroPad(pading, test);
    qwqdsp_spectral::RealFFT fft;
    fft.Init(1024);
    fft.FFT(pading, spectral);

    float gains[513];
    for (size_t i = 0; i < 513; ++i) {
        gains[i] = std::abs(spectral[i]);
    }
}
