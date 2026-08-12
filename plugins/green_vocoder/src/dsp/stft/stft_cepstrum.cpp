#include "stft_cepstrum.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numbers>

#include "qwqdsp/interpolation.hpp"
#include <qwqdsp/convert.hpp>
#include <qwqdsp/filter/window_fir.hpp>
#include <qwqdsp/window/window.hpp>

namespace green_vocoder::dsp {

static float GetFixGain(int fft_size) noexcept {
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
    bool const size_changed = fft_size != fft_size_;
    // 仅当 fft_size 改变时才重建倒谱缓冲（分配开销大）
    if (size_changed) {
        fft_size_ = fft_size;
        cep_fft_.Init(static_cast<size_t>(fft_size));
        cep_window_.resize(static_cast<size_t>(fft_size));
        cep_window_fft_.resize(static_cast<size_t>(fft_size));
        temp_.resize(static_cast<size_t>(fft_size) + 1);
        re1_.resize(static_cast<size_t>(fft_size));
        phase_.resize(static_cast<size_t>(fft_size));
    }

    // 仅当 detail 或 fft_size 变化时才重算倒谱窗（WindowFIR 设计开销大）
    if (size_changed || ParamChanged(p.detail, norm_detail_)) {
        norm_detail_ = p.detail;
        detail_ = norm_detail_ * 1024.0f / static_cast<float>(fft_size);
        detail_ = std::min(detail_, 1.0f);
        qwqdsp_filter::WindowFIR::Lowpass(cep_window_, p.detail * std::numbers::pi_v<float> * 0.5f);
        qwqdsp_window::Hann::ApplyWindow(cep_window_, false);
        cep_fft_.FFTGainPhase(cep_window_, cep_window_fft_);
    }
}

void STFTCepstrum::operator()(STFT& self, std::span<const float> real_in, std::span<const float> imag_in,
                              std::span<float> real_out, std::span<float> imag_out, int channel) {
    auto& gains = channel == 0 ? self.gains_left_ : self.gains_right_;

    int const fft_size = self.fft_size_;
    // 分析窗不归一化（普通 hann），此处保留 2/N 修正 + 倒谱 lifter 补偿系数（GetFixGain 查表）
    float window_gain = GetFixGain(fft_size) * 2.0f / static_cast<float>(fft_size);
    int const num_bins = fft_size / 2 + 1;
    for (int i = 0; i < fft_size / 2; ++i) {
        float re = real_in[static_cast<size_t>(i)];
        float im = imag_in[static_cast<size_t>(i)];
        float pow = std::sqrt(re * re + im * im) * window_gain;
        pow = std::log(pow + 1e-12f);
        temp_[static_cast<size_t>(i)] = pow;
        temp_[static_cast<size_t>(fft_size) - static_cast<size_t>(i)] = pow;
    }
    {
        int const i = fft_size / 2;
        float re = real_in[static_cast<size_t>(i)];
        float im = imag_in[static_cast<size_t>(i)];
        float pow = std::sqrt(re * re + im * im) * window_gain;
        pow = std::log(pow + 1e-12f);
        temp_[static_cast<size_t>(i)] = pow;
    }

    std::fill_n(phase_.begin(), static_cast<size_t>(fft_size), 0.0f);
    cep_fft_.IFFT(re1_, {temp_.data(), static_cast<size_t>(fft_size)}, phase_);
    for (int i = 0; i < fft_size; ++i) {
        re1_[static_cast<size_t>(i)] *= cep_window_fft_[static_cast<size_t>(i)];
    }
    cep_fft_.FFT(re1_, {temp_.data(), static_cast<size_t>(fft_size)}, phase_);

    for (int i = 0; i < num_bins; ++i) {
        float gain = std::exp(temp_[static_cast<size_t>(i)]) * global::kStftModMakeup;
        gain = self.Blend(gain);

        if (gain > gains[static_cast<size_t>(i)]) {
            gains[static_cast<size_t>(i)] =
                self.attack_factor_ * gains[static_cast<size_t>(i)] + (1.0f - self.attack_factor_) * gain;
        }
        else {
            gains[static_cast<size_t>(i)] =
                self.decay_factor_ * gains[static_cast<size_t>(i)] + (1.0f - self.decay_factor_) * gain;
        }
    }
    // 末尾两个 bin 置零：idx clamp 到 num_bins 后插值平滑衰减到 0，而非维持末 bin 增益
    gains[static_cast<size_t>(num_bins)] = 0.0f;
    gains[static_cast<size_t>(num_bins + 1)] = 0.0f;

    // 共振峰搬移（idx 超出奈奎斯特时 clamp 到 num_bins，其增益为 0，顶部平滑衰减）
    for (int i = 0; i < num_bins; ++i) {
        float idx = std::min(static_cast<float>(i) * self.formant_mul_,
                             static_cast<float>(num_bins));
        float const frac = idx - std::floor(idx);
        int const iidx = static_cast<int>(idx);

        float const g = qwqdsp::Interpolation::Linear(gains[static_cast<size_t>(iidx)],
                                                      gains[static_cast<size_t>(iidx) + 1], frac);

        real_out[static_cast<size_t>(i)] *= g;
        imag_out[static_cast<size_t>(i)] *= g;
    }
}

} // namespace green_vocoder::dsp
