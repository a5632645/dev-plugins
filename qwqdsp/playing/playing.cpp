#include "AudioFile.h"
#include "qwqdsp/fx/resample_iir.hpp"
#include "qwqdsp/fx/resample_coeffs.h"

int main() {
    AudioFile<float> infile;
    infile.load(R"(C:\Users\Kawai\Music\sweep.wav)");
    auto& sweep = infile.samples.front();

    qwqdsp::fx::ResampleIIR<qwqdsp::fx::coeff::BestCoeffs<float>, 128> resample;
    constexpr float kTargetFs = 96000.0f;
    resample.Init(infile.getSampleRate(), kTargetFs);
    auto sweep_resample = resample.Process<float>(sweep);
    
    AudioFile<float> outfile;
    outfile.setNumChannels(1);
    outfile.setBitDepth(32);
    outfile.setSampleRate(kTargetFs);
    outfile.setNumSamplesPerChannel(sweep_resample.size());
    outfile.samples.push_back(std::move(sweep_resample));
    outfile.save(R"(C:\Users\Kawai\Music\sweep-test.wav)");
}