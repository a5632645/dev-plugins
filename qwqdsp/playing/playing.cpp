#include <algorithm>
#include <array>
#include <cstddef>
#include <numbers>
#include <vector>
// #include "OLEDDisplayRGB.h"
// #include "stb_image_write.h"
#include <raylib.h>

#include "qwqdsp/osciilor/dsf.hpp"
#include "qwqdsp/convert.hpp"
#include "AudioFile.h"

int main() {
    qwqdsp::oscillor::DSFComplexFactor dsf;
    dsf.SetN(128);
    dsf.SetAmpGain(0.9f);
    dsf.SetAmpPhase(std::numbers::pi_v<float> / 8);
    dsf.SetW0(qwqdsp::convert::Freq2W(50, 48000));
    dsf.SetWSpace(qwqdsp::convert::Freq2W(50, 48000));

    AudioFile<float>::AudioBuffer out;
    out.resize(2);
    for (size_t i = 0; i < 48000; ++i) {
        auto s = dsf.Tick();
        out[0].push_back(s.real());
        out[1].push_back(s.imag());
    }

    AudioFile<float> outfile;
    outfile.setAudioBuffer(out);
    outfile.setSampleRate(48000);
    outfile.setBitDepth(32);
    outfile.save("dsf.wav");
}
