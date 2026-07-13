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

static float GetFixGain(size_t fft_size) noexcept {
    float db = 0;
    switch (fft_size) {
        case 256:
        case 512:
            db = 5;
            break;
        case 1024:
            db = 10;
            break;
        case 2048:
            db = 13;
            break;
        case 4096:
            db = 16;
            break;
        default:
            assert(false);
    }
    return qwqdsp::convert::Db2Gain(db);
}

void STFTVocoder::Init(float fs) {
    sample_rate_ = fs;
    SetFFTSize(1024);
}

void STFTVocoder::Reset() {
    std::ranges::fill(temp_main_, 0.0f);
    std::ranges::fill(temp_side_, 0.0f);
    std::ranges::fill(real_main_, 0.0f);
    std::ranges::fill(real_side_, 0.0f);
    std::ranges::fill(imag_main_, 0.0f);
    std::ranges::fill(imag_side_, 0.0f);
    std::ranges::fill(main_inputBuffer_, qwqdsp_simd_element::PackFloat<2>{});
    std::ranges::fill(side_inputBuffer_, qwqdsp_simd_element::PackFloat<2>{});
    std::ranges::fill(main_outputBuffer_, qwqdsp_simd_element::PackFloat<2>{});
    numInput_ = 0;
    writeEnd_ = 0;
    writeAddBegin_ = 0;

    std::ranges::fill(gains_, 0.0f);
    std::ranges::fill(gains2_, 0.0f);

    std::ranges::fill(mfcc_gains_, 0.0f);
    std::ranges::fill(mfcc_gains2_, 0.0f);
}

void STFTVocoder::Process(qwqdsp_simd_element::PackFloat<2>* main, qwqdsp_simd_element::PackFloat<2>* side,
                          int num_samples) {
    while (num_samples != 0) {
        // try copy some make a fft frame
        int require_frames = fft_size_ - numInput_;
        require_frames = std::clamp(require_frames, 0, num_samples);
        std::copy_n(main, require_frames, main_inputBuffer_.begin() + numInput_);
        std::copy_n(side, require_frames, side_inputBuffer_.begin() + numInput_);
        numInput_ += require_frames;
        num_samples -= require_frames;

        if (numInput_ >= fft_size_) {
            // standard mode have a special window
            if (mode_ == Mode::Standard) {
                for (int i = 0; i < fft_size_; ++i) {
                    temp_main_[i] = window_[i] * main_inputBuffer_[i][0];
                    temp_main_[i + fft_size_] = window_[i] * main_inputBuffer_[i][1];
                }
                for (int i = 0; i < fft_size_; ++i) {
                    temp_side_[i] = side_inputBuffer_[i][0];
                    temp_side_[i + fft_size_] = side_inputBuffer_[i][1];
                }
                numInput_ -= hop_size_;
                for (int i = 0; i < numInput_; i++) {
                    main_inputBuffer_[i] = main_inputBuffer_[i + hop_size_];
                }
                for (int i = 0; i < numInput_; i++) {
                    side_inputBuffer_[i] = side_inputBuffer_[i + hop_size_];
                }
            }
            else {
                for (int i = 0; i < fft_size_; ++i) {
                    temp_main_[i] = hann_window_[i] * main_inputBuffer_[i][0];
                    temp_main_[i + fft_size_] = hann_window_[i] * main_inputBuffer_[i][1];
                }
                for (int i = 0; i < fft_size_; ++i) {
                    temp_side_[i] = side_inputBuffer_[i][0];
                    temp_side_[i + fft_size_] = side_inputBuffer_[i][1];
                }
                numInput_ -= hop_size_;
                for (int i = 0; i < numInput_; i++) {
                    main_inputBuffer_[i] = main_inputBuffer_[i + hop_size_];
                }
                for (int i = 0; i < numInput_; i++) {
                    side_inputBuffer_[i] = side_inputBuffer_[i + hop_size_];
                }
            }

            // -------------------- left --------------------
            fft_.fft(temp_main_.data(), real_main_.data(), imag_main_.data());
            fft_.fft(temp_side_.data(), real_side_.data(), imag_side_.data());
            switch (mode_) {
                case Mode::Standard:
                    SpectralProcess_Standard(real_main_, imag_main_, real_side_, imag_side_, gains_);
                    break;
                case Mode::Cepstrum:
                    SpectralProcess_Cepstrum(real_main_, imag_main_, real_side_, imag_side_, gains_);
                    break;
                case Mode::MFCC:
                    SpectralProcess_MFCC(real_main_, imag_main_, real_side_, imag_side_, mfcc_gains_);
                    break;
            }
            fft_.ifft(temp_main_.data(), real_side_.data(), imag_side_.data());

            // -------------------- right --------------------
            fft_.fft(temp_main_.data() + fft_size_, real_main_.data(), imag_main_.data());
            fft_.fft(temp_side_.data() + fft_size_, real_side_.data(), imag_side_.data());
            switch (mode_) {
                case Mode::Standard:
                    SpectralProcess_Standard(real_main_, imag_main_, real_side_, imag_side_, gains_);
                    break;
                case Mode::Cepstrum:
                    SpectralProcess_Cepstrum(real_main_, imag_main_, real_side_, imag_side_, gains_);
                    break;
                case Mode::MFCC:
                    SpectralProcess_MFCC(real_main_, imag_main_, real_side_, imag_side_, mfcc_gains2_);
                    break;
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
        if (writeAddBegin_ >= require_frames) {
            // extract output
            size_t extractSize = require_frames;
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
            std::fill_n(main, require_frames, qwqdsp_simd_element::PackFloat<2>{});
        }

        main += require_frames;
        side += require_frames;
    }
}

// -------------------- commmon setter --------------------

void STFTVocoder::SetRelease(float ms) {
    release_ms_ = ms;
    decay_ = qwqdsp::convert::Ms2DecayDb((ms + attack_ms_), sample_rate_ / static_cast<float>(hop_size_), -60.0f);
}

void STFTVocoder::SetAttack(float ms) {
    attack_ms_ = ms;
    attck_ = qwqdsp::convert::Ms2DecayDb(ms, sample_rate_, -60.0f);
    decay_ = qwqdsp::convert::Ms2DecayDb((ms + attack_ms_), sample_rate_ / static_cast<float>(hop_size_), -60.0f);
}

void STFTVocoder::SetFFTSize(int size) {
    fft_size_ = size;
    fft_.init(size);
    cep_fft_.Init(size);
    main_inputBuffer_.resize(fft_size_);
    side_inputBuffer_.resize(fft_size_);
    main_outputBuffer_.resize(fft_size_ * 4);
    hann_window_.resize(size);
    window_.resize(size);
    temp_main_.resize(size * 2);
    temp_side_.resize(size * 2);
    hop_size_ = size / 4;
    for (int i = 0; i < size; ++i) {
        hann_window_[i] =
            0.5f - 0.5f * std::cos(2.0f * std::numbers::pi_v<float> * static_cast<float>(i) / static_cast<float>(size));
    }
    int num_bins = fft_.ComplexSize(size);
    gains_.resize(num_bins + kExtraGainSize);
    gains2_.resize(num_bins + kExtraGainSize);
    fill_gains_.resize(num_bins + kExtraGainSize);
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
    SetDetail(norm_detail_);
    SetNumMfcc(num_mfcc_);
}

void STFTVocoder::SetBlend(float blend) {
    blend_ = blend;
}

void STFTVocoder::SetFormantShift(float formant_shift) {
    formant_mul_ = std::exp2(-formant_shift / 12.0f);
}

void STFTVocoder::SetMode(Mode mode) {
    mode_ = mode;
}

// -------------------- standard setter --------------------

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

// -------------------- cepstrum setter --------------------

void STFTVocoder::SetDetail(float detail) {
    norm_detail_ = detail;
    detail_ = norm_detail_ * 1024.0f / fft_size_;
    detail_ = std::min(detail_, 1.0f);
    qwqdsp_filter::WindowFIR::Lowpass(cep_window_, detail * std::numbers::pi_v<float> * 0.5f);
    qwqdsp_window::Hann::ApplyWindow(cep_window_, false);
    cep_fft_.FFTGainPhase(cep_window_, cep_window_fft_);
}

// -------------------- mfcc setter --------------------

void STFTVocoder::SetNumMfcc(int num_mfcc) {
    num_mfcc_ = std::clamp(num_mfcc, kMinNumMfcc, kMaxNumMfcc);
    float begin_mel = qwqdsp::convert::Freq2Mel(0);
    float end_mel = qwqdsp::convert::Freq2Mel(sample_rate_ / 2);
    float interval_mel = (end_mel - begin_mel) / static_cast<float>(num_mfcc_);
    for (int i = 0; i < num_mfcc_; ++i) {
        float mel = begin_mel + static_cast<float>(i) * interval_mel;
        float freq = qwqdsp::convert::Mel2Freq(mel);
        int bin = static_cast<int>(std::floor(freq / static_cast<float>(sample_rate_) * static_cast<float>(fft_size_)));
        bin = std::min(bin, fft_size_ / 2);
        mfcc_indexs_[i] = bin;
    }
    mfcc_indexs_[0] = 0;
    mfcc_indexs_[num_mfcc_] = fft_size_ / 2;
}

// -------------------- private --------------------

float STFTVocoder::Blend(float x) {
    x = 2.0f * x - 1.0f;
    x = (blend_ + x) / (1 + blend_ * x);
    x = 0.5f * x + 0.5f;
    return x;
}

void STFTVocoder::SpectralProcess_Standard(std::vector<float>& real_in, std::vector<float>& imag_in,
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

void STFTVocoder::SpectralProcess_Cepstrum(std::vector<float>& real_in, std::vector<float>& imag_in,
                                           std::vector<float>& real_out, std::vector<float>& imag_out,
                                           std::vector<float>& gains) {
    float window_gain = GetFixGain(fft_size_) * 2.0f / fft_size_;
    size_t num_bins = fft_size_ / 2 + 1;
    for (size_t i = 0; i < fft_size_ / 2; ++i) {
        float re = real_in[i];
        float im = imag_in[i];
        float pow = std::sqrt(re * re + im * im) * window_gain;
        pow = std::log(pow + 1e-12f);
        temp_[i] = pow;
        temp_[fft_size_ - i] = pow;
    }
    {
        size_t i = fft_size_ / 2;
        float re = real_in[i];
        float im = imag_in[i];
        float pow = std::sqrt(re * re + im * im) * window_gain;
        pow = std::log(pow + 1e-12f);
        temp_[i] = pow;
    }

    std::fill_n(phase_.begin(), fft_size_, 0.0f);
    cep_fft_.IFFT(re1_, {temp_.data(), static_cast<size_t>(fft_size_)}, phase_);
    for (size_t i = 0; i < fft_size_; ++i) {
        re1_[i] *= cep_window_fft_[i];
    }
    cep_fft_.FFT(re1_, {temp_.data(), static_cast<size_t>(fft_size_)}, phase_);

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

void STFTVocoder::SpectralProcess_MFCC(std::vector<float>& real_in, std::vector<float>& imag_in,
                                       std::vector<float>& real_out, std::vector<float>& imag_out,
                                       std::array<float, kMaxNumMfcc>& gains) {
    for (size_t mcff_idx = 0; mcff_idx < num_mfcc_; ++mcff_idx) {
        size_t begin = mfcc_indexs_[mcff_idx];
        size_t end = mfcc_indexs_[mcff_idx + 1];

        // take an averge RMS as vocoder band gain
        float sum = 0.0f;
        for (size_t i = begin; i < end; ++i) {
            sum += real_in[i] * real_in[i] + imag_in[i] * imag_in[i];
        }
        sum /= static_cast<float>(end - begin + 1);
        sum = std::sqrt(sum);

        float gain = sum * window_gain_;
        if (gain > gains[mcff_idx]) {
            gains[mcff_idx] = attck_ * gains[mcff_idx] + (1 - attck_) * gain;
        }
        else {
            gains[mcff_idx] = decay_ * gains[mcff_idx] + (1 - decay_) * gain;
        }

        for (size_t i = begin; i < end; ++i) {
            fill_gains_[i] = gains[mcff_idx];
        }
    }

    size_t num_bins = fft_size_ / 2 + 1;
    fill_gains_[num_bins] = fill_gains_[0];
    // apply formant
    for (size_t i = 0; i < num_bins; ++i) {
        float idx = static_cast<float>(i) * formant_mul_;
        float frac = idx - std::floor(idx);
        size_t iidx = static_cast<size_t>(idx);

        float g = 0;
        if (iidx < num_bins) {
            g = qwqdsp::Interpolation::Linear(fill_gains_[iidx], fill_gains_[iidx + 1], frac);
        }

        real_out[i] *= g;
        imag_out[i] *= g;
    }
}

} // namespace green_vocoder::dsp
