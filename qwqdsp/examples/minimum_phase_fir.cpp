#include "qwqdsp/spectral/real_fft.hpp"
#include "qwqdsp/window/blackman.hpp"
#include "qwqdsp/spectral/complex_fft.hpp"
#include "qwqdsp/filter/window_fir.hpp"
#include "qwqdsp/window/helper.hpp"
#include <cstddef>
#include <numbers>

// @ref https://www.researchgate.net/publication/3317646_Design_of_Optimal_Minimum-phase_Digital_FIR_Filters_Using_Discrete_Hilbert_Transforms
int main() {
    float fir[65];
    qwqdsp::filter::WindowFIR::Lowpass(fir, std::numbers::pi_v<float> / 2);

    float fir_pad[4096];
    qwqdsp::window::Helper::ZeroPad(fir_pad, fir);

    qwqdsp::spectral::ComplexFFT<false> fft;
    fft.Init(4096);
    constexpr size_t num_bins = fft.NumBins(4096);
    float gains[num_bins];
    fft.FFTGainPhase(fir_pad, gains);

    float log_gains[num_bins];
    for (size_t i = 0; i < num_bins; ++i) {
        log_gains[i] = std::log(gains[i] + 1e-18f);
    }

    float phases[num_bins]{};
    float ir[4096];
    fft.IFFT<float>(ir, log_gains, phases);
    ir[0] = 0;
    ir[num_bins / 2] = 0;
    for (size_t i = num_bins / 2 + 1; i < num_bins; ++i) {
        ir[i] = -ir[i];
    }

    fft.FFT(ir, log_gains, phases);
    fft.IFFTGainPhase(ir, gains, phases);

    float slice[65];
    for (size_t i = 0; i < 65; ++i) {
        slice[i] = ir[i];
    }

    float min_phase_pad[1024];
    float fir_pad2[1024];
    qwqdsp::window::Helper::ZeroPad(fir_pad2, fir);
    qwqdsp::window::Helper::ZeroPad(min_phase_pad, slice);
    qwqdsp::spectral::RealFFT fft2;
    constexpr size_t num_bins2 = fft2.NumBins(1024);
    float fir_gains[num_bins2];
    float min_phase_gains[num_bins2];
    fft2.Init(1024);
    fft2.FFTGainPhase(min_phase_pad, min_phase_gains);
    fft2.FFTGainPhase(fir_pad2, fir_gains);
    for (size_t i = 0; i < num_bins2; ++i) {
        fir_gains[i] = 20.0f * std::log10(fir_gains[i] + 1e-6f);
        min_phase_gains[i] = 20.0f * std::log10(min_phase_gains[i] + 1e-6f);
    }
}