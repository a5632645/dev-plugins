#pragma once
#include <span>
#include <array>
#include "AudioFFT/AudioFFT.h"
#include "filter.hpp"

namespace dsp {

class CepstrumVocoder {
public:
    static constexpr int kFFTSize = 1024;
    static constexpr int kHopSize = 128;
    static constexpr int kNumBins = kFFTSize / 2 + 1;

    static float GainToDb(float gain);

    void Init(float fs);
    void SetFFTSize(int size);
    void Process(std::span<float> block, std::span<float> block2);

    void SetOmega(float omega);
    void SetRelease(float ms);

    std::array<float, kNumBins> gains_{};
private:
    Filter spectral_filter_;
    audiofft::AudioFFT fft_;
    std::array<float, kFFTSize> hann_window_{};
    std::array<float, 4096> main_inputBuffer_{};
    std::array<float, 4096> side_inputBuffer_{};
    std::array<float, 4096> main_outputBuffer_{};
    int numInput_{};
    int writeEnd_{};
    int writeAddBegin_{};
    float decay_{};
    float sample_rate_{};
};

}