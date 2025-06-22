#include "cepstrum_vocoder.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <numbers>
#include <numeric>

namespace dsp {

void CepstrumVocoder::Init(float fs) {
    sample_rate_ = fs;
    SetFFTSize(kFFTSize);
}

void CepstrumVocoder::SetFFTSize(int size) {
    fft_.init(size);
    cepstrum_fft_.init(size / 2);
    for (int i = 0; i < kFFTSize; ++i) {
        hann_window_[i] = 0.5 - 0.5 * std::cos(2 * std::numbers::pi_v<float> * i / (kFFTSize - 1));
    }
    fft_gain_ = 2.0f / std::accumulate(hann_window_.begin(), hann_window_.end(), 0.0f);
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
        // half amp -> log
        std::array<float, kFFTSize / 2> log_amps;
        for (int i = 1; i < kNumBins; ++i) {
            float g = std::abs(main_real[i] * main_real[i] + main_imag[i] * main_imag[i]) * fft_gain_;
            log_amps[i - 1] = std::log(g + 1e-10f);
        }
        // filter
        cepstrum_fft_.fft(log_amps.data(), main_real.data(), main_imag.data());
        int len = kFFTSize / 2;
        int bins = len / 2 + 1;
        for (int i = filtering_; i < bins; ++i) {
            main_real[i] = 0.0f;
            main_imag[i] = 0.0f;
        }
        cepstrum_fft_.ifft(log_amps.data(), main_real.data(), main_imag.data());

        // vocoding
        for (int i = 1; i < kNumBins; ++i) {
            float log_amp = log_amps[i - 1];
            float gain = std::exp(log_amp);
            gain = Blend(gain);
            gains_[i] = decay_ * gains_[i] + (1 - decay_) * gain;

            side_real[i] *= gains_[i];
            side_imag[i] *= gains_[i];
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

void CepstrumVocoder::SetBlend(float blend) {
    blend_ = blend;
}

void CepstrumVocoder::SetFiltering(float filtering) {
    int cepstrum_fft_size = kFFTSize / 2;
    int bins = cepstrum_fft_size / 2 + 1;
    filtering_ = static_cast<int>(filtering * bins);
    filtering_ = std::max(filtering_, 1);
}

float CepstrumVocoder::Blend(float x) {
    x = 2.0f * x - 1.0f;
    x = (blend_ + x) / (1 + blend_ * x);
    x = 0.5f * x + 0.5f;
    return x;
}

}