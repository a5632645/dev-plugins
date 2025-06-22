#pragma once
#include <span>
#include <array>
#include "AudioFFT/AudioFFT.h"

namespace dsp {

class CepstrumVocoder {
public:
    static constexpr int kFFTSize = 1024;
    static constexpr int kHopSize = 256;
    static constexpr int kNumBins = kFFTSize / 2 + 1;

    static float GainToDb(float gain);

    void Init(float fs);
    void SetFFTSize(int size);
    void Process(std::span<float> block, std::span<float> block2);

    void SetBlend(float blend);
    void SetRelease(float ms);
    void SetFiltering(float filtering);

    std::array<float, kNumBins> gains_{};
private:
    float Blend(float x);

    audiofft::AudioFFT fft_;
    audiofft::AudioFFT cepstrum_fft_;
    std::array<float, kFFTSize> hann_window_{};
    std::array<float, 4096> main_inputBuffer_{};
    std::array<float, 4096> side_inputBuffer_{};
    std::array<float, 4096> main_outputBuffer_{};
    int numInput_{};
    int writeEnd_{};
    int writeAddBegin_{};
    float decay_{};
    float sample_rate_{};
    float fft_gain_{};
    float blend_{};
    int filtering_{};
};

}