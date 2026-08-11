#include "stft_morph.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace {

// 精细结构比（ra）上限：A 的包络比最多放大 2 倍（6 dB），防稀疏谱/深谷处爆炸
constexpr float kMaxRa = 2.0f;
// A 包络相对本帧 |A|max 的下界，低于此按无精细结构处理（ra=1）
constexpr float kEaFloorRel = 1e-6f;

} // namespace

namespace green_vocoder::dsp {

void STFTMorph::Init(STFT& self) {
    SetParam(Params{}, self);
}

void STFTMorph::SetParam(const Params& p, STFT& self) {
    int const fft_size = self.fft_size_;
    if (fft_size != fft_size_) {
        fft_size_ = fft_size;
        num_bins_ = fft_size / 2 + 1;
        cep_fft_.Init(static_cast<size_t>(fft_size));
        shifted_.resize(static_cast<size_t>(fft_size));
        quefrency_.resize(static_cast<size_t>(fft_size));
        spectral_.resize(static_cast<size_t>(fft_size));
        mag_a_.resize(static_cast<size_t>(num_bins_));
        mag_b_.resize(static_cast<size_t>(num_bins_));
        envelope_a_.resize(static_cast<size_t>(num_bins_));
        envelope_b_.resize(static_cast<size_t>(num_bins_));
        out_re_.resize(static_cast<size_t>(num_bins_));
        out_im_.resize(static_cast<size_t>(num_bins_));
    }

    morph_ = std::clamp(p.morph, 0.0f, 1.0f);
    direction_ab_ = p.direction_ab;

    // 包络窗：宽度随 morph（x³）与 fft 尺寸变化时重算
    float const width_f =
        std::clamp(static_cast<float>(fft_size_) * morph_ * morph_ * morph_, 2.0f, static_cast<float>(fft_size_));
    int const width = static_cast<int>(std::lround(width_f));
    int const center = fft_size_ / 2;
    left_ = std::max(0, center - width / 2);
    right_ = std::min(fft_size_, center + width / 2 + 1);
    int const win_len = std::max(2, right_ - left_ - 1);
    int const win_count = right_ - left_;
    envelope_window_.resize(static_cast<size_t>(win_count));
    for (int q = 0; q < win_count; ++q) {
        float const arg = 2.0f * std::numbers::pi_v<float> * static_cast<float>(q) / static_cast<float>(win_len);
        envelope_window_[static_cast<size_t>(q)] =
            0.42f - 0.5f * std::cos(arg) + 0.08f * std::cos(2.0f * arg);
    }
}

void STFTMorph::ExtractEnvelope(std::span<const float> magnitude, std::span<float> envelope) {
    int const fft_size = fft_size_;
    int const half = fft_size / 2;

    // 正频幅度按 π 相位斜坡（(-1)^k）放入全尺寸复谱，其余置零
    for (int k = 0; k < half; ++k) {
        float const sgn = (k & 1) ? -1.0f : 1.0f;
        shifted_[static_cast<size_t>(k)] = {magnitude[static_cast<size_t>(k)] * sgn, 0.0f};
    }
    for (int k = half; k < fft_size; ++k)
        shifted_[static_cast<size_t>(k)] = {0.0f, 0.0f};

    // 倒谱域：IFFT → 取实部 → 中部 Blackman 加窗 → FFT
    cep_fft_.IFFT(quefrency_, shifted_);
    for (int k = 0; k < fft_size; ++k)
        quefrency_[static_cast<size_t>(k)] = {quefrency_[static_cast<size_t>(k)].real(), 0.0f};
    for (int k = 0; k < left_; ++k)
        quefrency_[static_cast<size_t>(k)] = {0.0f, 0.0f};
    for (int k = right_; k < fft_size; ++k)
        quefrency_[static_cast<size_t>(k)] = {0.0f, 0.0f};
    for (int k = left_; k < right_; ++k)
        quefrency_[static_cast<size_t>(k)] *= envelope_window_[static_cast<size_t>(k - left_)];

    cep_fft_.FFT(quefrency_, spectral_);
    for (int k = 0; k < half; ++k)
        envelope[static_cast<size_t>(k)] = 2.0f * std::abs(spectral_[static_cast<size_t>(k)]);
    envelope[static_cast<size_t>(half)] = envelope[0];
}

void STFTMorph::operator()(STFT& self, std::span<const float> real_in, std::span<const float> imag_in,
                           std::span<float> real_out, std::span<float> imag_out, int channel) {
    auto& gains = channel == 0 ? self.gains_ : self.gains2_;
    int const num_bins = num_bins_;

    // A/B 角色：direction 决定 A→B 还是 B→A（交换输入谱）
    std::span<const float> re_a = direction_ab_ ? real_in : real_out;
    std::span<const float> im_a = direction_ab_ ? imag_in : imag_out;
    std::span<const float> re_b = direction_ab_ ? real_out : real_in;
    std::span<const float> im_b = direction_ab_ ? imag_out : imag_in;

    for (int k = 0; k < num_bins; ++k) {
        float const r_a = re_a[static_cast<size_t>(k)];
        float const i_a = im_a[static_cast<size_t>(k)];
        float const r_b = re_b[static_cast<size_t>(k)];
        float const i_b = im_b[static_cast<size_t>(k)];
        mag_a_[static_cast<size_t>(k)] = std::sqrt(r_a * r_a + i_a * i_a);
        mag_b_[static_cast<size_t>(k)] = std::sqrt(r_b * r_b + i_b * i_b);
    }

    // cepstral 包络分离
    ExtractEnvelope(mag_a_, envelope_a_);
    ExtractEnvelope(mag_b_, envelope_b_);

    float const a11 = std::pow(morph_, 11.0f);
    float const b11 = std::pow(1.0f - morph_, 11.0f);
    float const w_cross = (1.0f - b11) * (1.0f - a11);

    // 本帧输入最大幅度：软限幅阈值与包络相对下界的参考
    float max_ma = 0.0f;
    float max_mb = 0.0f;
    for (int k = 0; k < num_bins; ++k) {
        max_ma = std::max(max_ma, mag_a_[static_cast<size_t>(k)]);
        max_mb = std::max(max_mb, mag_b_[static_cast<size_t>(k)]);
    }

    for (int k = 0; k < num_bins; ++k) {
        float const ma = mag_a_[static_cast<size_t>(k)];
        float const ea = envelope_a_[static_cast<size_t>(k)];
        float const eb = envelope_b_[static_cast<size_t>(k)];

        // 精细结构/残差（包络比）：包络低于相对下界按无精细结构（ra=1），
        // 上限 4（12 dB）防稀疏谱/深谷处病态放大；NaN/INF 防护
        float ra = 1.0f;
        if (std::isfinite(ma) && std::isfinite(ea) && ea > kEaFloorRel * max_ma)
            ra = std::min(ma / ea, kMaxRa);

        // A 的单位相位（|A|=0 时取 0）
        float const ca_re = ma > 0.0f ? re_a[static_cast<size_t>(k)] / ma : 0.0f;
        float const ca_im = ma > 0.0f ? im_a[static_cast<size_t>(k)] / ma : 0.0f;

        // cross = A 的精细结构 × B 的包络（保持 A 相位）；无交叉淡化（y=0）
        float const cross_re = (ra * eb) * ca_re;
        float const cross_im = (ra * eb) * ca_im;

        // result = wa*wb*cross + b11*A + a11*B
        float const r_a = re_a[static_cast<size_t>(k)];
        float const i_a = im_a[static_cast<size_t>(k)];
        float const r_b = re_b[static_cast<size_t>(k)];
        float const i_b = im_b[static_cast<size_t>(k)];
        out_re_[static_cast<size_t>(k)] = w_cross * cross_re + b11 * r_a + a11 * r_b;
        out_im_[static_cast<size_t>(k)] = w_cross * cross_im + b11 * i_a + a11 * i_b;
    }

    // 写回输出（替换载波频谱）并更新 GUI 显示（取软限幅后幅度）
    for (int k = 0; k < num_bins; ++k) {
        real_out[static_cast<size_t>(k)] = out_re_[static_cast<size_t>(k)];
        imag_out[static_cast<size_t>(k)] = out_im_[static_cast<size_t>(k)];
        gains[static_cast<size_t>(k)] = std::hypot(out_re_[static_cast<size_t>(k)], out_im_[static_cast<size_t>(k)]);
    }
    gains[static_cast<size_t>(num_bins)] = gains[0];
}

} // namespace green_vocoder::dsp
