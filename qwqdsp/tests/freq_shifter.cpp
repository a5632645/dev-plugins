#include "qwqdsp/filter/iir_hilbert.hpp"
#include "qwqdsp/filter/iir_cpx_hilbert.hpp"
#include "qwqdsp/oscillator/vic_sine_osc.hpp"
#include "../playing/AudioFile.h"

static constexpr auto kInputFile 
= R"(C:\Users\Kawai\Music\gunge_slice.wav)";
static constexpr auto kOutputFile 
= R"(C:\Users\Kawai\Music\gunge_slice-shift.wav)";
static constexpr auto kOutputFile2 
= R"(C:\Users\Kawai\Music\gunge_slice-shift2.wav)";
static constexpr auto kShift = -150; //hz

static void FreqShifter() {
    qwqdsp_filter::IIRHilbertDeeper<> hilbert;
    qwqdsp_oscillator::VicSineOsc osc_;
    
    AudioFile<float> file;
    if (file.load(kInputFile)) {
        osc_.Reset(0);
        osc_.SetFreq(kShift, file.getSampleRate());

        auto& io = file.samples.front();
        for (auto& s : io) {
            auto analyze_signal = hilbert.Tick(s);
            osc_.Tick();
            auto quad = osc_.GetCpx();
            analyze_signal *= quad;
            s = analyze_signal.real();
        }

        file.setNumChannels(1);
        file.save(kOutputFile);
    }
}

static void FreqShifterAntialaising() {
    qwqdsp_filter::IIRHilbertDeeper<> hilbert;
    qwqdsp_filter::IIRHilbertDeeperCpx<> antialaising_filter;
    qwqdsp_oscillator::VicSineOsc osc_;
    
    AudioFile<float> file;
    if (file.load(kInputFile)) {    
        osc_.Reset(0);
        osc_.SetFreq(kShift, file.getSampleRate());

        auto& io = file.samples.front();
        for (auto& s : io) {
            auto analyze_signal = hilbert.Tick(s);
            osc_.Tick();
            auto quad = osc_.GetCpx();
            analyze_signal *= quad;
            // 移除负频率
            analyze_signal = antialaising_filter.Tick(analyze_signal);
            s = analyze_signal.real();
        }

        file.setNumChannels(1);
        file.save(kOutputFile2);
    }
}

int main() {
    FreqShifter();
    FreqShifterAntialaising();
}
