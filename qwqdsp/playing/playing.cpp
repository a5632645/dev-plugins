#include <cstddef>
#include <numbers>
#include <cmath>

#include "qwqdsp/filter/polyphase_apf.hpp"
#include "qwqdsp/spectral/real_fft.hpp"
#include "qwqdsp/window/helper.hpp"


int main() {
    qwqdsp::filter::PolyphaseAPF apf;
    apf.Init(64);
    apf.SetNLatch(5);
    apf.SetAlpha(0.6f);
    float ir[4096];
    ir[0] = apf.Tick(1.0f);
    for (size_t i = 1; i < 4096; ++i) {
        ir[i] = apf.Tick(0.0f);
    }

    float pad[4096];
    qwqdsp::window::Helper::ZeroPad(pad, ir);
    qwqdsp::spectral::RealFFT fft;
    fft.Init(4096);
    float gains[fft.NumBins(4096)];
    fft.FFTGainPhase(pad, gains);
    for (auto& x : gains) {
        x = std::max(-100.0f, 20.0f * std::log10(x + 1e-18f));
    }
}