#include "stft_vocoder.hpp"
#include "utli.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <numeric>

namespace dsp {

void STFTVocoder::Init(float fs) {
    sample_rate_ = fs;
    SetFFTSize(1024);
}

void STFTVocoder::SetFFTSize(int size) {
    fft_size_ = size;
    fft_.init(size);
    hann_window_.resize(size);
    window_.resize(size);
    temp_main_.resize(size);
    temp_side_.resize(size);
    hop_size_ = size / 4;
    for (int i = 0; i < size; ++i) {
        hann_window_[i] = 0.5f - 0.5f * std::cos(2.0f* std::numbers::pi_v<float> * i / (size - 1.0f));
    }
    size_t num_bins = fft_.ComplexSize(size);
    gains_.resize(num_bins);
    real_main_.resize(num_bins);
    imag_main_.resize(num_bins);
    real_side_.resize(num_bins);
    imag_side_.resize(num_bins);
    SetRelease(release_ms_);
    SetBandwidth(bandwidth_);
}

void STFTVocoder::SetRelease(float ms) {
    release_ms_ = ms;
    decay_ = utli::GetDecayValue(sample_rate_ / hop_size_, ms);
}

void STFTVocoder::SetBlend(float blend) {
    blend_ = blend;
}

void STFTVocoder::Process(std::span<float> block, std::span<float> block2) {
    assert(block2.size() == block.size());

    std::copy(block.begin(), block.end(), main_inputBuffer_.begin() + numInput_);
    std::copy(block2.begin(), block2.end(), side_inputBuffer_.begin() + numInput_);
    numInput_ += static_cast<int>(block.size());
    while (numInput_ >= fft_size_) {
        for (int i = 0; i < fft_size_; ++i) {
            temp_main_[i] = window_[i] * main_inputBuffer_[i];
        }
        std::copy_n(side_inputBuffer_.cbegin(), fft_size_, temp_side_.begin());
        numInput_ -= hop_size_;
        for (int i = 0; i < numInput_; i++) {
            main_inputBuffer_[i] = main_inputBuffer_[i + hop_size_];
        }
        for (int i = 0; i < numInput_; i++) {
            side_inputBuffer_[i] = side_inputBuffer_[i + hop_size_];
        }

        // do fft
        fft_.fft(temp_main_.data(), real_main_.data(), imag_main_.data());
        fft_.fft(temp_side_.data(), real_side_.data(), imag_side_.data());
        
        // spectral processing
        size_t num_bins = fft_.ComplexSize(fft_size_);
        for (size_t i = 0; i < num_bins; ++i) {
            float power = std::abs(real_main_[i] * real_main_[i] + imag_main_[i] * imag_main_[i]);
            float gain = std::sqrt(power) * window_gain_;
            gain = Blend(gain);

            if (gain > gains_[i]) {
                gains_[i] = attck_ * gains_[i] + (1 - attck_) * gain;
            }
            else {
                gains_[i] = decay_ * gains_[i] + (1 - decay_) * gain;
            }
            
            real_side_[i] *= gains_[i];
            imag_side_[i] *= gains_[i];
        }

        fft_.ifft(temp_main_.data(), real_side_.data(), imag_side_.data());

        // overlay add
        for (int i = 0; i < fft_size_; i++) {   
            main_outputBuffer_[i + writeAddBegin_] += temp_main_[i] * hann_window_[i];
        }
        writeEnd_ = writeAddBegin_ + fft_size_;
        writeAddBegin_ += hop_size_;
    }

    if (writeAddBegin_ >= static_cast<int>(block.size())) {
        // extract output
        int extractSize = static_cast<int>(block.size());
        for (int i = 0; i < extractSize; ++i) {
            block[i] = main_outputBuffer_[i] * 4.0f;
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
    float f0 = bandwidth_ * fft_size_ / 1024.0f;
    for (int i = 0; i < fft_size_; i++) {
        float x = (2 * std::numbers::pi_v<float> * f0 * (i - fft_size_ / 2.0f)) / fft_size_;
        float sinc = std::abs(x) < 1e-6 ? 1.0f : std::sin(x) / x;
        window_[i] = sinc * hann_window_[i];
    }
    window_gain_ = 2.0f / std::accumulate(window_.begin(), window_.end(), 0.0f);
}

void STFTVocoder::SetAttack(float ms) {
    attck_ = utli::GetDecayValue(sample_rate_ / hop_size_, ms);
}

float STFTVocoder::Blend(float x) {
    x = 2.0f * x - 1.0f;
    x = (blend_ + x) / (1 + blend_ * x);
    x = 0.5f * x + 0.5f;
    return x;
}

}