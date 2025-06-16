#include "stft_vocoder.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <numeric>
#include "dsp/windows.h"

namespace dsp {

void STFTVocoder::Init(float fs) {
    SetFFTSize(kFFTSize);
}

void STFTVocoder::SetFFTSize(int size) {
    fft_.init(kFFTSize);
    for (int i = 0; i < kFFTSize; ++i) {
        hann_window_[i] = 0.5 - 0.5 * std::cos(2 * M_PI * i / (kFFTSize - 1));
    }
}

void STFTVocoder::Process(std::span<float> block, std::span<float> block2) {
    assert(block2.size() == block.size());

    std::copy(block.begin(), block.end(), main_inputBuffer_.begin() + numInput_);
    std::copy(block2.begin(), block2.end(), side_inputBuffer_.begin() + numInput_);
    numInput_ += static_cast<int>(block.size());
    while (numInput_ >= kFFTSize) {
        // forward fft, copy 1024
        float main_timeBuffer[kFFTSize]{};
        float side_timeBuffer[kFFTSize]{};
        // zero pad to 2048
        for (int i = 0; i < kFFTSize; ++i) {
            main_timeBuffer[i] = window_[i] * main_inputBuffer_[i];
        }
        std::copy_n(side_inputBuffer_.cbegin(), kFFTSize, side_timeBuffer);
        // lost hopsize samples
        numInput_ -= kHopSize;
        for (int i = 0; i < numInput_; i++) {
            main_inputBuffer_[i] = main_inputBuffer_[i + kHopSize];
        }
        for (int i = 0; i < numInput_; i++) {
            side_inputBuffer_[i] = side_inputBuffer_[i + kHopSize];
        }

        // do fft
        std::array<float, kNumBins> main_real;
        std::array<float, kNumBins> main_imag;
        std::array<float, kNumBins> side_real;
        std::array<float, kNumBins> side_imag;
        fft_.fft(main_timeBuffer, main_real.data(), main_imag.data());
        fft_.fft(side_timeBuffer, side_real.data(), side_imag.data());

        // spectral processing
        float g = 1.0f / std::sqrt(static_cast<float>(kFFTSize));
        for (int i = 0; i < kNumBins; ++i) {
            side_real[i] *= g;
            side_imag[i] *= g;

            float v = std::abs(main_real[i] * main_real[i] + main_imag[i] * main_imag[i]);
            gains_[i] = v * g;
            side_real[i] *= v;
            side_imag[i] *= v;
        }

        // get ifft output, 2048 samlpes
        fft_.ifft(main_timeBuffer, side_real.data(), side_imag.data());

        // overlay add
        for (uint32_t i = 0; i < kFFTSize; i++) {   
            main_outputBuffer_[i + writeAddBegin_] += main_timeBuffer[i] * hann_window_[i];
        }
        writeEnd_ = writeAddBegin_ + kFFTSize;
        writeAddBegin_ += kHopSize;
    }

    if (writeAddBegin_ >= static_cast<int>(block.size())) {
        // extract output
        int extractSize = static_cast<int>(block.size());
        for (int i = 0; i < extractSize; ++i) {
            block[i] = main_outputBuffer_[i] / ((float)kFFTSize / kHopSize);
        }
        
        // shift output buffer
        int shiftSize = writeEnd_ - extractSize;
        for (int i = 0; i < shiftSize; i++) {
            main_outputBuffer_[i] = main_outputBuffer_[i + extractSize];
        }
        writeAddBegin_ -= extractSize;
        int newWriteEnd = writeEnd_ - extractSize;
        // zero shifed buffer
        for (int i = newWriteEnd; i < writeEnd_; ++i) {
            main_outputBuffer_[i] = 0.0f;
        }
        writeEnd_ = newWriteEnd;
    }
    else {
        // zero buffer
        std::fill(block.begin(), block.end(), 0.0f);
    }
}

void STFTVocoder::SetBandwidth(float bw) {
    bandwidth_ = bw;
    auto gen = signalsmith::windows::ApproximateConfinedGaussian::withBandwidth(bandwidth_ );
    gen.fill(window_, kFFTSize);
    float windows = std::accumulate(window_.begin(), window_.end(), 0.0f);
    float gain = 2.0f / std::sqrt(windows);
    for (auto& s : window_) {
        s *= gain;
    }
}

}