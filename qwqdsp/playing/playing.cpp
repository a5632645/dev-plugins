#include <qwqdsp/spectral/ipp_real_fft.hpp>
#include <qwqdsp/spectral/oouras_real_fft.hpp>
#include <numbers>
#include <cmath>
#include <utility>

int main() {
    float test[1024];
    float w = 3.0f / 1024.0f * std::numbers::pi_v<float> * 2.0f;
    for (int i = 0; i < 1024; ++i) {
        test[i] = std::sin(w * i) + 1.0f;
    }

    qwqdsp_spectral::OourasRealFFT fft;
    std::pair<float, float> reim[513];
    fft.Init(1024);
    fft.FFT(test, (float*)reim);

    float gain[513];
    for (int i = 0; i < 513; ++i) {
        gain[i] = std::sqrt(reim[i].first * reim[i].first + reim[i].second * reim[i].second);
    }

    float output[1024];
    fft.IFFT((float*)reim, output);
}
