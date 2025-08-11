#pragma once
#include <span>
#include <array>
#include <vector>
#include "AudioFFT/AudioFFT.h"

namespace dsp {

class STFTVocoder {
public:
    // static constexpr int kFFTSize = 1024;
    // static constexpr int kHopSize = kFFTSize / 4;
    // static constexpr int kNumBins = kFFTSize / 2 + 1;

    void Init(float fs);
    void Process(std::span<float> block, std::span<float> block2);
    
    void SetBandwidth(float bw);
    void SetRelease(float ms);
    void SetAttack(float ms);
    void SetBlend(float blend);
    void SetFFTSize(int size);

    int GetFFTSize() const { return fft_size_; }

    std::vector<float> gains_{};
private:
    float Blend(float x);

    audiofft::AudioFFT fft_;
    std::vector<float> window_{};
    std::vector<float> hann_window_{};
    std::vector<float> temp_main_{};
    std::vector<float> temp_side_{};
    std::vector<float> real_main_{};
    std::vector<float> real_side_{};
    std::vector<float> imag_main_{};
    std::vector<float> imag_side_{};
    std::array<float, 32768> main_inputBuffer_{};
    std::array<float, 32768> side_inputBuffer_{};
    std::array<float, 32768> main_outputBuffer_{};
    int fft_size_{};
    int hop_size_{};
    int numInput_{};
    int writeEnd_{};
    int writeAddBegin_{};
    float bandwidth_{};
    float decay_{};
    float attck_{};
    float sample_rate_{};
    float blend_{};
    float window_gain_{};
    float release_ms_{};
};

}