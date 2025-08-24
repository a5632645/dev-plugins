#include <algorithm>
#include <array>
#include <cstddef>
#include <numbers>
#include <vector>
// #include "OLEDDisplayRGB.h"
// #include "stb_image_write.h"
#include <raylib.h>
#include "AudioFile.h"

#include "qwqdsp/spectral/real_fft.hpp"
#include "qwqdsp/window/helper.hpp"

int main() {
    AudioFile<float> kick;
    kick.load(R"(C:\Users\Kawai\Downloads\Music\kick2-short.wav)");
    auto& wav = kick.samples.front();

    float pad[32768];
    qwqdsp::window::Helper::ZeroPad(pad, wav);
    qwqdsp::spectral::RealFFT fft;
    fft.Init(32768);
    float imag[32768];
    fft.Hilbert(pad, imag, true);

    float env[32768];
    for (size_t i = 0; i < 32768; ++i) {
        env[i] = std::sqrt(pad[i] * pad[i] + imag[i] * imag[i]);
    }

    AudioFile<float>::AudioBuffer out;
    out.resize(1);
    out.front().resize(32768);
    std::copy(env, env + 32768, out.front().begin());
    AudioFile<float> outfile;
    outfile.setAudioBuffer(out);
    outfile.setSampleRate(kick.getSampleRate());
    outfile.setBitDepth(32);
    outfile.save(R"(C:\Users\Kawai\Desktop\debug\kick2-short.wav)");
}
