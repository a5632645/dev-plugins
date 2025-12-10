#include "mfcc_vocoder.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <qwqdsp/convert.hpp>

namespace green_vocoder::dsp {

void MFCCVocoder::Init(float fs) {
    sample_rate_ = fs;
    SetFFTSize(1024);
    SetNumMfcc(20);
}

void MFCCVocoder::SetFFTSize(size_t size) {
    fft_size_ = size;
    fft_.init(size);
    hann_window_.resize(size);
    temp_main_.resize(size * 2);
    temp_side_.resize(size * 2);
    hop_size_ = size / 4;
    for (size_t i = 0; i < size; ++i) {
        hann_window_[i] = 0.5f - 0.5f * std::cos(2.0f* std::numbers::pi_v<float> * static_cast<float>(i) / static_cast<float>(size));
    }
    size_t num_bins = fft_.ComplexSize(size);
    real_main_.resize(num_bins);
    imag_main_.resize(num_bins);
    real_side_.resize(num_bins);
    imag_side_.resize(num_bins);
    SetRelease(release_ms_);
    window_gain_ = 2.0f / static_cast<float>(size);
    SetNumMfcc(num_mfcc_);
}

void MFCCVocoder::SetRelease(float ms) {
    release_ms_ = ms;
    decay_ = qwqdsp::convert::Ms2DecayDb(ms, sample_rate_ / static_cast<float>(hop_size_), -60.0f);
}

void MFCCVocoder::SetNumMfcc(size_t num_mfcc) {
    num_mfcc_ = std::clamp(num_mfcc, kMinNumMfcc, kMaxNumMfcc);
    float begin_mel = qwqdsp::convert::Freq2Mel(0);
    float end_mel = qwqdsp::convert::Freq2Mel(sample_rate_ / 2);
    float interval_mel = (end_mel - begin_mel) / static_cast<float>(num_mfcc_);
    for (size_t i = 0; i < num_mfcc_; ++i) {
        float mel = begin_mel + static_cast<float>(i) * interval_mel;
        float freq = qwqdsp::convert::Mel2Freq(mel);
        size_t bin = static_cast<size_t>(std::floor(freq / static_cast<float>(sample_rate_) * static_cast<float>(fft_size_)));
        bin = std::min(bin, fft_size_ / 2);
        mfcc_indexs_[i] = bin;
    }
    mfcc_indexs_[0] = 0;
    mfcc_indexs_[num_mfcc_] = fft_size_ / 2;
}

void MFCCVocoder::Process(
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
            temp_main_[i] = main_inputBuffer_[i][0];
            temp_main_[i + fft_size_] = main_inputBuffer_[i][1];
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
        for (size_t mcff_idx = 0; mcff_idx < num_mfcc_; ++mcff_idx) {
            size_t begin = mfcc_indexs_[mcff_idx];
            size_t end = mfcc_indexs_[mcff_idx + 1];
            float sum = 0.0f;
            for (size_t i = begin; i < end; ++i) {
                sum += std::sqrt(real_main_[i] * real_main_[i] + imag_main_[i] * imag_main_[i]);
            }

            float gain = sum * window_gain_;
            if (gain > gains_[mcff_idx]) {
                gains_[mcff_idx] = attck_ * gains_[mcff_idx] + (1 - attck_) * gain;
            }
            else {
                gains_[mcff_idx] = decay_ * gains_[mcff_idx] + (1 - decay_) * gain;
            }

            for (size_t i = begin; i < end; ++i) {
                real_side_[i] *= gains_[mcff_idx];
                imag_side_[i] *= gains_[mcff_idx];
            }
        }
        fft_.ifft(temp_main_.data(), real_side_.data(), imag_side_.data());
        // -------------------- right --------------------
        fft_.fft(temp_main_.data() + fft_size_, real_main_.data(), imag_main_.data());
        fft_.fft(temp_side_.data() + fft_size_, real_side_.data(), imag_side_.data());
        // spectral processing
        for (size_t mcff_idx = 0; mcff_idx < num_mfcc_; ++mcff_idx) {
            size_t begin = mfcc_indexs_[mcff_idx];
            size_t end = mfcc_indexs_[mcff_idx + 1];
            float sum = 0.0f;
            for (size_t i = begin; i < end; ++i) {
                sum += std::sqrt(real_main_[i] * real_main_[i] + imag_main_[i] * imag_main_[i]);
            }

            float gain = sum * window_gain_;
            if (gain > gains2_[mcff_idx]) {
                gains2_[mcff_idx] = attck_ * gains2_[mcff_idx] + (1 - attck_) * gain;
            }
            else {
                gains2_[mcff_idx] = decay_ * gains2_[mcff_idx] + (1 - decay_) * gain;
            }

            for (size_t i = begin; i < end; ++i) {
                real_side_[i] *= gains2_[mcff_idx];
                imag_side_[i] *= gains2_[mcff_idx];
            }
        }
        // for (size_t i = 0; i < num_bins; ++i) {
        //     float power = std::abs(real_main_[i] * real_main_[i] + imag_main_[i] * imag_main_[i]);
        //     float gain = std::sqrt(power) * window_gain_;

        //     if (gain > gains_[i]) {
        //         gains_[i] = attck_ * gains_[i] + (1 - attck_) * gain;
        //     }
        //     else {
        //         gains_[i] = decay_ * gains_[i] + (1 - decay_) * gain;
        //     }
            
        //     real_side_[i] *= gains_[i];
        //     imag_side_[i] *= gains_[i];
        // }
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
            main[i] = main_outputBuffer_[i];
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

void MFCCVocoder::SetAttack(float ms) {
    attck_ = qwqdsp::convert::Ms2DecayDb(ms, sample_rate_, -60.0f);
}

}
