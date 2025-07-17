#include "stft_vocoder.hpp"
#include "utli.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <numbers>

namespace dsp {

void STFTVocoder::Init(float fs) {
    sample_rate_ = fs;
    SetFFTSize(kFFTSize);
}

void STFTVocoder::SetFFTSize(int size) {
    fft_.init(size);
    for (int i = 0; i < kFFTSize; ++i) {
        hann_window_[i] = 0.5f - 0.5f * std::cos(2.0f* std::numbers::pi_v<float> * i / (kFFTSize - 1.0f));
    }
}

void STFTVocoder::SetRelease(float ms) {
    decay_ = utli::GetDecayValue(sample_rate_ / kHopSize, ms);
}

void STFTVocoder::SetBlend(float blend) {
    blend_ = blend;
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
        for (int i = 0; i < kNumBins; ++i) {
            // i know this is power spectrum, but it sounds better than mag spectrum(???)
            float power = std::abs(main_real[i] * main_real[i] + main_imag[i] * main_imag[i]);
            // float gain = power * window_gain_;
            float gain = std::sqrt(power + 1e-18f) * window_gain_;
            gain = Blend(gain);

            if (gain > gains_[i]) {
                gains_[i] = attck_ * gains_[i] + (1 - attck_) * gain;
            }
            else {
                gains_[i] = decay_ * gains_[i] + (1 - decay_) * gain;
            }
            
            side_real[i] *= gains_[i];
            side_imag[i] *= gains_[i];
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
    // generate sinc window
    float f0 = bandwidth_ * kFFTSize / kFFTSize;
    for (int i = 0; i < kFFTSize; i++) {
        float x = (2 * std::numbers::pi_v<float> * f0 * (i - kFFTSize / 2.0f)) / kFFTSize;
        float sinc = std::abs(x) < 1e-6 ? 1.0f : std::sin(x) / x;
        window_[i] = sinc * hann_window_[i];
    }
    // keep energy(???)
    // window_gain_ = 2.0f / std::accumulate(window_.begin(), window_.end(), 0.0f);
    float power = 0.0f;
    for (auto x : window_) {
        power += x * x;
    }
    window_gain_ = 1.0f / std::sqrt(power);
}

void STFTVocoder::SetAttack(float ms) {
    attck_ = utli::GetDecayValue(sample_rate_ / kHopSize, ms);
}

float STFTVocoder::Blend(float x) {
    x = 2.0f * x - 1.0f;
    x = (blend_ + x) / (1 + blend_ * x);
    x = 0.5f * x + 0.5f;
    return x;
}

}