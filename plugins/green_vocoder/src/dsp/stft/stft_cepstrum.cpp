#include "stft_cepstrum.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numbers>

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

void STFTCepstrum::Init(STFT& self) {
    SetParam(Params{}, self);
}

void STFTCepstrum::SetParam(const Params& p, STFT& self) {
    int const fft_size = self.fft_size_;

    cep_fft_.Init(static_cast<size_t>(fft_size));
    cep_window_.resize(static_cast<size_t>(fft_size));
    cep_window_fft_.resize(static_cast<size_t>(fft_size));
    temp_.resize(static_cast<size_t>(fft_size) + 1);
    re1_.resize(static_cast<size_t>(fft_size));
    phase_.resize(static_cast<size_t>(fft_size));

    norm_detail_ = p.detail;
    detail_ = norm_detail_ * 1024.0f / static_cast<float>(fft_size);
    detail_ = std::min(detail_, 1.0f);
    qwqdsp_filter::WindowFIR::Lowpass(cep_window_, p.detail * std::numbers::pi_v<float> * 0.5f);
    qwqdsp_window::Hann::ApplyWindow(cep_window_, false);
    cep_fft_.FFTGainPhase(cep_window_, cep_window_fft_);
}

void STFTCepstrum::operator()(STFT& self,
                              std::span<const float> real_in, std::span<const float> imag_in,
                              std::span<float> real_out, std::span<float> imag_out, int channel) {
    auto& gains = channel == 0 ? self.gains_ : self.gains2_;
    int const fft_size = self.fft_size_;
    float window_gain = GetFixGain(static_cast<size_t>(fft_size)) * 2.0f / static_cast<float>(fft_size);
    size_t num_bins = static_cast<size_t>(fft_size) / 2 + 1;
    for (size_t i = 0; i < static_cast<size_t>(fft_size) / 2; ++i) {
        float re = real_in[i];
        float im = imag_in[i];
        float pow = std::sqrt(re * re + im * im) * window_gain;
        pow = std::log(pow + 1e-12f);
        temp_[i] = pow;
        temp_[static_cast<size_t>(fft_size) - i] = pow;
    }
    {
        size_t i = static_cast<size_t>(fft_size) / 2;
        float re = real_in[i];
        float im = imag_in[i];
        float pow = std::sqrt(re * re + im * im) * window_gain;
        pow = std::log(pow + 1e-12f);
        temp_[i] = pow;
    }

    std::fill_n(phase_.begin(), static_cast<size_t>(fft_size), 0.0f);
    cep_fft_.IFFT(re1_, {temp_.data(), static_cast<size_t>(fft_size)}, phase_);
    for (size_t i = 0; i < static_cast<size_t>(fft_size); ++i) {
        re1_[i] *= cep_window_fft_[i];
    }
    cep_fft_.FFT(re1_, {temp_.data(), static_cast<size_t>(fft_size)}, phase_);

    for (size_t i = 0; i < num_bins; ++i) {
        float gain = std::exp(temp_[i]);
        gain = self.Blend(gain);

        if (gain > gains[i]) {
            gains[i] = self.attack_factor_ * gains[i] + (1.0f - self.attack_factor_) * gain;
        }
        else {
            gains[i] = self.decay_ * gains[i] + (1.0f - self.decay_) * gain;
        }
    }
    gains[num_bins] = gains[0];

    for (size_t i = 0; i < num_bins; ++i) {
        float idx = static_cast<float>(i) * self.formant_mul_;
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
