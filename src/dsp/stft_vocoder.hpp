#pragma once
#include <span>
#include <array>
#include "AudioFFT/AudioFFT.h"

namespace dsp {

class STFTVocoder {
public:
    static constexpr int kFFTSize = 2048;
    static constexpr int kHopSize = 512;
    static constexpr int kNumBins = kFFTSize / 2 + 1;

    void Init(float fs);
    void SetFFTSize(int size);
    void Process(std::span<float> block, std::span<float> block2);

    void SetBandwidth(float bw);

    std::array<float, kNumBins> gains_{};
private:
    audiofft::AudioFFT fft_;
    std::array<float, kFFTSize> window_{};
    std::array<float, kFFTSize> hann_window_{};
    std::array<float, 4096> main_inputBuffer_{};
    std::array<float, 4096> side_inputBuffer_{};
    std::array<float, 4096> main_outputBuffer_{};
    int numInput_{};
    int writeEnd_{};
    int writeAddBegin_{};
    float bandwidth_{0.5f};
};

}