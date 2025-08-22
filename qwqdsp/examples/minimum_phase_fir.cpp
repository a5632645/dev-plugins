#include "qwqdsp/spectral/real_fft.hpp"

int main() {
    constexpr size_t fft_size = 4096;
    constexpr size_t init = qwqdsp::spectral::RealFFT::NumBins(fft_size);
    float gain[init]{};
    float log_gain[init]{};
    float phases[init]{};
    std::fill_n(gain, 500, 1.0f);
    for (size_t i = 0; i < init; ++i) {
        log_gain[i] = std::log(gain[i] + 1e-18f);
    }

    qwqdsp::spectral::RealFFT fft;
    fft.Init(fft_size);
    float ifft0[fft_size];
    fft.IFFT(ifft0, log_gain, phases);
    float h[fft_size]{};
    h[0] = 1.0f;
    h[fft_size / 2] = 1.0f;
    for (size_t i = 1; i < fft_size / 2; ++i) {
        h[i] = 2.0f;
    }

    float real_gains[fft_size]{};
    for (size_t i = 0; i < fft_size; ++i) {
        real_gains[i] = h[i] * ifft0[i];
    }
    float real[init];
    float imag[init];
    fft.FFT(real_gains, real, imag);

    float impluse[fft_size];
    fft.IFFTGainPhase(impluse, gain, imag);

    float test[fft_size / 2];
    std::copy_n(impluse, fft_size / 2, test);
    fft.Init(fft_size / 2);
    fft.FFT(test, {real, fft_size/4+1}, {imag, fft_size/4+1});
    float test_gain[fft_size/4+1];
    for (size_t i = 0; i <= fft_size/4; ++i) {
        test_gain[i] = std::abs(std::complex{real[i], imag[i]});
    }
    // 使用GRAPHICAL WATCH查看
}