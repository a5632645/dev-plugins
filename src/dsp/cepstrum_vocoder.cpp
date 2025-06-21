#include "cepstrum_vocoder.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include "dsp/windows.h"

namespace dsp {

void CepstrumVocoder::Init(float fs) {
    sample_rate_ = fs;
    SetFFTSize(kFFTSize);
}

void CepstrumVocoder::SetFFTSize(int size) {
    fft_.init(kFFTSize);
    for (int i = 0; i < kFFTSize; ++i) {
        hann_window_[i] = 0.5 - 0.5 * std::cos(2 * M_PI * i / (kFFTSize - 1));
    }
}

void CepstrumVocoder::SetRelease(float ms) {
    decay_ = std::exp(-1.0f / ((sample_rate_ / kHopSize) * ms / 1000.0f));
}

void CepstrumVocoder::Process(std::span<float> block, std::span<float> block2) {
    assert(block2.size() == block.size());

    std::copy(block.begin(), block.end(), main_inputBuffer_.begin() + numInput_);
    std::copy(block2.begin(), block2.end(), side_inputBuffer_.begin() + numInput_);
    numInput_ += static_cast<int>(block.size());
    while (numInput_ >= kFFTSize) {
        float main_timeBuffer[kFFTSize]{};
        float side_timeBuffer[kFFTSize]{};
        for (int i = 0; i < kFFTSize; ++i) {
            main_timeBuffer[i] = hann_window_[i] * main_inputBuffer_[i];
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
        float g = 1.0f / (kFFTSize / 2.0f);
        spectral_filter_.ResetLatch();
        for (int i = 1; i < kNumBins; ++i) {
            float gain = std::abs(main_real[i] * main_real[i] + main_imag[i] * main_imag[i]) * g;
            float db = CepstrumVocoder::GainToDb(gain);
            float spectral_evelop = spectral_filter_.ProcessSingle(db);

            if (spectral_evelop > gains_[i]) {
                gains_[i] = spectral_evelop;
            }
            else {
                gains_[i] = decay_ * gains_[i] + (1 - decay_) * spectral_evelop;
            }

            float se_gain = std::pow(10.0f, spectral_evelop / 20.0f);
            side_real[i] *= se_gain;
            side_imag[i] *= se_gain;
        }

        // get ifft output
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

float CepstrumVocoder::GainToDb(float gain) {
    return 20.0f * std::log10(gain + 1e-5f); // mininal -100dB
}

void CepstrumVocoder::SetOmega(float omega) {
    spectral_filter_.MakeLowpassDirect(omega);
}

}