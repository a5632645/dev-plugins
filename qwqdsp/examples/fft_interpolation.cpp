#include <cstddef>
#include <numbers>
#include "qwqdsp/spectral/real_fft.hpp"

int main() {
    float sin[512];
    for (size_t i = 0; i < 512; ++i) {
        sin[i] = std::sin(std::numbers::pi_v<float> * 2 * i / 512.0f);
    }

    std::complex<float> spectral[qwqdsp::spectral::RealFFT::NumBins(1024)]{};
    float pad_sin[1024];
    qwqdsp::spectral::RealFFT fft;
    fft.Init(512);
    fft.FFT(sin, {spectral, fft.NumBins()});
    for (size_t i = 0; i < fft.NumBins(); ++i) {
        spectral[i] *= (1024.0f / 512.0f);
    }
    fft.Init(1024);
    fft.IFFT(pad_sin, spectral);
}