#include "stft_vocoder.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <numeric>
#include <qwqdsp/convert.hpp>

namespace green_vocoder::dsp {

void STFTVocoder::Init(float fs) {
    sample_rate_ = fs;
    SetFFTSize(1024);
}

void STFTVocoder::SetFFTSize(size_t size) {
    fft_size_ = size;
    fft_.init(size);
    hann_window_.resize(size);
    window_.resize(size);
    temp_main_.resize(size * 2);
    temp_side_.resize(size * 2);
    hop_size_ = size / 4;
    for (size_t i = 0; i < size; ++i) {
        hann_window_[i] = 0.5f - 0.5f * std::cos(2.0f* std::numbers::pi_v<float> * static_cast<float>(i) / static_cast<float>(size));
    }
    size_t num_bins = fft_.ComplexSize(size);
    gains_.resize(num_bins);
    gains2_.resize(num_bins);
    real_main_.resize(num_bins);
    imag_main_.resize(num_bins);
    real_side_.resize(num_bins);
    imag_side_.resize(num_bins);
    SetRelease(release_ms_);
    SetBandwidth(bandwidth_);
}

void STFTVocoder::SetRelease(float ms) {
    release_ms_ = ms;
    decay_ = qwqdsp::convert::Ms2DecayDb((ms + attack_ms_), sample_rate_ / static_cast<float>(hop_size_), -60.0f);
}

void STFTVocoder::SetAttack(float ms) {
    attack_ms_ = ms;
    attck_ = qwqdsp::convert::Ms2DecayDb(ms, sample_rate_, -60.0f);
    decay_ = qwqdsp::convert::Ms2DecayDb((ms + attack_ms_), sample_rate_ / static_cast<float>(hop_size_), -60.0f);
}

void STFTVocoder::SetBlend(float blend) {
    blend_ = blend;
}

void STFTVocoder::Process(
    qwqdsp_simd_element::PackFloat<2>* main,
    qwqdsp_simd_element::PackFloat<2>* side,
    size_t num_samples
) {
    // -------------------- doing left --------------------
    std::copy_n(main, num_samples, main_inputBuffer_.begin() + static_cast<int>(numInput_));
    std::copy_n(side, num_samples, side_inputBuffer_.begin() + static_cast<int>(numInput_));
    numInput_ += num_samples;
    while (numInput_ >= fft_size_) {
        for (size_t i = 0; i < fft_size_; ++i) {
            temp_main_[i] = window_[i] * main_inputBuffer_[i][0];
            temp_main_[i + fft_size_] = window_[i] * main_inputBuffer_[i][1];
        }
        for (size_t i = 0; i < fft_size_; ++i) {
            temp_side_[i] = side_inputBuffer_[i][0];
            temp_side_[i + fft_size_] = side_inputBuffer_[i][1];
        }
        numInput_ -= hop_size_;
        for (size_t i = 0; i < numInput_; i++) {
            main_inputBuffer_[i] = main_inputBuffer_[i + hop_size_];
        }
        for (size_t i = 0; i < numInput_; i++) {
            side_inputBuffer_[i] = side_inputBuffer_[i + hop_size_];
        }

        // -------------------- left --------------------
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
        // -------------------- right --------------------
        fft_.fft(temp_main_.data() + fft_size_, real_main_.data(), imag_main_.data());
        fft_.fft(temp_side_.data() + fft_size_, real_side_.data(), imag_side_.data());
        // spectral processing
        for (size_t i = 0; i < num_bins; ++i) {
            float power = std::abs(real_main_[i] * real_main_[i] + imag_main_[i] * imag_main_[i]);
            float gain = std::sqrt(power) * window_gain_;
            gain = Blend(gain);

            if (gain > gains2_[i]) {
                gains2_[i] = attck_ * gains2_[i] + (1 - attck_) * gain;
            }
            else {
                gains2_[i] = decay_ * gains2_[i] + (1 - decay_) * gain;
            }
            
            real_side_[i] *= gains2_[i];
            imag_side_[i] *= gains2_[i];
        }
        fft_.ifft(temp_main_.data() + fft_size_, real_side_.data(), imag_side_.data());

        // overlay add
        for (size_t i = 0; i < fft_size_; i++) {
            float left = temp_main_[i] * hann_window_[i];
            float right = temp_main_[i + fft_size_] * hann_window_[i];  
            main_outputBuffer_[i + writeAddBegin_] += {left, right};
        }
        writeEnd_ = writeAddBegin_ + fft_size_;
        writeAddBegin_ += hop_size_;
    }
    // -------------------- output --------------------
    if (writeAddBegin_ >= num_samples) {
        // extract output
        size_t extractSize = num_samples;
        for (size_t i = 0; i < extractSize; ++i) {
            main[i] = main_outputBuffer_[i] * 4.0f;
        }
        
        // shift output buffer
        size_t shiftSize = writeEnd_ - extractSize;
        for (size_t i = 0; i < shiftSize; i++) {
            main_outputBuffer_[i] = main_outputBuffer_[i + extractSize];
        }
        writeAddBegin_ -= extractSize;
        size_t newWriteEnd = writeEnd_ - extractSize;
        // zero shifed buffer
        for (size_t i = newWriteEnd; i < writeEnd_; ++i) {
            main_outputBuffer_[i].Broadcast(0);
        }
        writeEnd_ = newWriteEnd;
    }
    else {
        // zero buffer
        std::fill_n(main, num_samples, qwqdsp_simd_element::PackFloat<2>{});
    }
}

void STFTVocoder::SetBandwidth(float bw) {
    bandwidth_ = bw;
    // generate sinc window
    float f0 = bandwidth_ * static_cast<float>(fft_size_) / 1024.0f;
    for (size_t i = 0; i < fft_size_; i++) {
        float x = (2 * std::numbers::pi_v<float> * f0 * (static_cast<float>(i) - static_cast<float>(fft_size_) / 2.0f)) / static_cast<float>(fft_size_);
        float sinc = std::abs(x) < 1e-6 ? 1.0f : std::sin(x) / x;
        window_[i] = sinc * hann_window_[i];
    }
    window_gain_ = 2.0f / std::accumulate(window_.begin(), window_.end(), 0.0f);
}

float STFTVocoder::Blend(float x) {
    x = 2.0f * x - 1.0f;
    x = (blend_ + x) / (1 + blend_ * x);
    x = 0.5f * x + 0.5f;
    return x;
}

}
