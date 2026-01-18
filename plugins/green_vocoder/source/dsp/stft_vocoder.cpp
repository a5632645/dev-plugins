#include "stft_vocoder.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <numeric>
#include <qwqdsp/convert.hpp>
#include <qwqdsp/filter/window_fir.hpp>
#include <qwqdsp/window/window.hpp>
#include "qwqdsp/interpolation.hpp"

namespace green_vocoder::dsp {

void STFTVocoder::Init(float fs) {
    sample_rate_ = fs;
    SetFFTSize(1024);
}

void STFTVocoder::SetFFTSize(size_t size) {
    fft_size_ = size;
    fft_.init(size);
    cep_fft_.Init(size);
    hann_window_.resize(size);
    window_.resize(size);
    temp_main_.resize(size * 2);
    temp_side_.resize(size * 2);
    hop_size_ = size / 4;
    for (size_t i = 0; i < size; ++i) {
        hann_window_[i] =
            0.5f - 0.5f * std::cos(2.0f * std::numbers::pi_v<float> * static_cast<float>(i) / static_cast<float>(size));
    }
    size_t num_bins = fft_.ComplexSize(size);
    gains_.resize(num_bins + kExtraGainSize);
    gains2_.resize(num_bins + kExtraGainSize);
    real_main_.resize(num_bins);
    imag_main_.resize(num_bins);
    real_side_.resize(num_bins);
    imag_side_.resize(num_bins);
    cep_window_.resize(fft_size_);
    cep_window_fft_.resize(fft_size_);
    temp_.resize(fft_size_ + 1);
    re1_.resize(fft_size_);
    phase_.resize(fft_size_);
    SetRelease(release_ms_);
    SetBandwidth(bandwidth_);
    SetDetail(detail_);
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

void STFTVocoder::Process(qwqdsp_simd_element::PackFloat<2>* main, qwqdsp_simd_element::PackFloat<2>* side,
                          size_t num_samples) {
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
        fft_.fft(temp_main_.data(), real_main_.data(), imag_main_.data());
        fft_.fft(temp_side_.data(), real_side_.data(), imag_side_.data());
        if (use_v2_) {
            SpectralProcess2(real_main_, imag_main_, real_side_, imag_side_, gains_);
        }
        else {
            SpectralProcess(real_main_, imag_main_, real_side_, imag_side_, gains_);
        }
        fft_.ifft(temp_main_.data(), real_side_.data(), imag_side_.data());

        // -------------------- right --------------------
        fft_.fft(temp_main_.data() + fft_size_, real_main_.data(), imag_main_.data());
        fft_.fft(temp_side_.data() + fft_size_, real_side_.data(), imag_side_.data());
        if (use_v2_) {
            SpectralProcess2(real_main_, imag_main_, real_side_, imag_side_, gains_);
        }
        else {
            SpectralProcess(real_main_, imag_main_, real_side_, imag_side_, gains_);
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
        float x = (2 * std::numbers::pi_v<float> * f0 * (static_cast<float>(i) - static_cast<float>(fft_size_) / 2.0f))
                / static_cast<float>(fft_size_);
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

void STFTVocoder::SetFormantShift(float formant_shift) {
    formant_mul_ = std::exp2(-formant_shift / 12.0f);
}

void STFTVocoder::SetUseV2(bool use) {
    use_v2_ = use;
}

void STFTVocoder::SetDetail(float detail) {
    detail_ = detail;
    qwqdsp_filter::WindowFIR::Lowpass(cep_window_, detail * std::numbers::pi_v<float> * 0.5f);
    qwqdsp_window::Hann::ApplyWindow(cep_window_, false);
    cep_fft_.FFTGainPhase(cep_window_, cep_window_fft_);
}

void STFTVocoder::SpectralProcess(std::vector<float>& real_in, std::vector<float>& imag_in,
                                  std::vector<float>& real_out, std::vector<float>& imag_out,
                                  std::vector<float>& gains) {
    // a bad formant extra
    size_t num_bins = fft_.ComplexSize(fft_size_);
    for (size_t i = 0; i < num_bins; ++i) {
        float power = std::abs(real_in[i] * real_in[i] + imag_in[i] * imag_in[i]);
        float gain = std::sqrt(power) * window_gain_;
        gain = Blend(gain);

        if (gain > gains[i]) {
            gains[i] = attck_ * gains[i] + (1 - attck_) * gain;
        }
        else {
            gains[i] = decay_ * gains[i] + (1 - decay_) * gain;
        }
    }
    gains[num_bins] = gains[0];
    // apply formant
    for (size_t i = 0; i < num_bins; ++i) {
        float idx = static_cast<float>(i) * formant_mul_;
        float frac = idx - std::floor(idx);
        size_t iidx = static_cast<size_t>(idx);

        float g = 0;
        if (iidx < num_bins) {
            g = qwqdsp::Interpolation::Linear(gains[iidx], gains[iidx + 1], frac);
        }

        real_out[i] *= g;
        imag_out[i] *= g;
    }
}

void STFTVocoder::SpectralProcess2(std::vector<float>& real_in, std::vector<float>& imag_in,
                                   std::vector<float>& real_out, std::vector<float>& imag_out,
                                   std::vector<float>& gains) {
    size_t num_bins = fft_size_ / 2 + 1;
    for (size_t i = 0; i < fft_size_ / 2; ++i) {
        float re = real_in[i];
        float im = imag_in[i];
        float pow = std::sqrt(re * re + im * im) * window_gain_;
        pow = std::log(pow + 1e-12f);
        temp_[i] = pow;
        temp_[fft_size_ - i] = pow;
    }
    {
        size_t i = fft_size_ / 2;
        float re = real_in[i];
        float im = imag_in[i];
        float pow = std::sqrt(re * re + im * im) * window_gain_;
        pow = std::log(pow + 1e-12f);
        temp_[i] = pow;
    }

    std::fill_n(phase_.begin(), fft_size_, 0.0f);
    cep_fft_.IFFT(re1_, {temp_.data(), fft_size_}, phase_);
    for (size_t i = 0; i < fft_size_; ++i) {
        re1_[i] *= cep_window_fft_[i];
    }
    cep_fft_.FFT(re1_, {temp_.data(), fft_size_}, phase_);

    for (size_t i = 0; i < num_bins; ++i) {
        float gain = std::exp(temp_[i]);
        gain = Blend(gain);

        if (gain > gains[i]) {
            gains[i] = attck_ * gains[i] + (1 - attck_) * gain;
        }
        else {
            gains[i] = decay_ * gains[i] + (1 - decay_) * gain;
        }
    }
    gains[num_bins] = gains[0];
    
    for (size_t i = 0; i < num_bins; ++i) {
        float idx = static_cast<float>(i) * formant_mul_;
        float frac = idx - std::floor(idx);
        size_t iidx = static_cast<size_t>(idx);

        float g = 0;
        if (iidx < num_bins) {
            g = qwqdsp::Interpolation::Linear(gains[iidx], gains[iidx + 1], frac);
        }

        real_out[i] *= g;
        imag_out[i] *= g;
    }
}

} // namespace green_vocoder::dsp
