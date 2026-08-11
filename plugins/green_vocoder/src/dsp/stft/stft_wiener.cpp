#include "stft_wiener.hpp"

#include <algorithm>
#include <cmath>

namespace {

// Standard 变体增益上限（≈ 24 dB）：防 a/(b+snr) 在静音/稀疏载波处病态放大
constexpr float kMaxGain = 16.0f;
// snr 下限：防除零
constexpr float kSnrMin = 1e-6f;

} // namespace

namespace green_vocoder::dsp {

void STFTWiener::Init(STFT& self) {
    SetParam(Params{}, self);
}

void STFTWiener::SetParam(const Params& p, STFT& self) {
    fft_size_ = self.fft_size_;
    num_bins_ = fft_size_ / 2 + 1;
    variant_ = p.variant;
    snr_ = std::max(p.snr, kSnrMin);
    direction_ab_ = p.direction_ab;
}

void STFTWiener::operator()(STFT& self, std::span<const float> real_in, std::span<const float> imag_in,
                            std::span<float> real_out, std::span<float> imag_out, int channel) {
    auto& gains = channel == 0 ? self.gains_ : self.gains2_;
    int const num_bins = num_bins_;

    // A/B 角色：direction 决定 a=A(调制器)、b=B(载波) 还是交换
    std::span<const float> re_a = direction_ab_ ? real_in : real_out;
    std::span<const float> im_a = direction_ab_ ? imag_in : imag_out;
    std::span<const float> re_b = direction_ab_ ? real_out : real_in;
    std::span<const float> im_b = direction_ab_ ? imag_out : imag_in;

    // a/b 的窗增益：a 取 mod 或 carry（按 direction），b 取相反
    float const ga = direction_ab_ ? self.mod_window_gain_ : self.carry_window_gain_;
    float const gb = direction_ab_ ? self.carry_window_gain_ : self.mod_window_gain_;
    float const ga2 = ga * ga;
    float const gb2 = gb * gb;

    for (int i = 0; i < num_bins; ++i) {
        // 归一化功率谱（× 窗增益²，使幅度可直接读出）
        float const a = (re_a[static_cast<size_t>(i)] * re_a[static_cast<size_t>(i)]
                       + im_a[static_cast<size_t>(i)] * im_a[static_cast<size_t>(i)])
                      * ga2;
        float const b = (re_b[static_cast<size_t>(i)] * re_b[static_cast<size_t>(i)]
                       + im_b[static_cast<size_t>(i)] * im_b[static_cast<size_t>(i)])
                      * gb2;

        // 维纳反卷积增益
        float g = 0.0f;
        if (variant_ == Variant::Standard)
            g = std::min(a / (b + snr_), kMaxGain);
        else
            g = (a - b) / (a + b + snr_);

        if (!std::isfinite(g))
            g = 0.0f; // NaN/INF 防护

        real_out[static_cast<size_t>(i)] *= g;
        imag_out[static_cast<size_t>(i)] *= g;

        // GUI 显示（增益幅度）
        gains[static_cast<size_t>(i)] = std::abs(g);
    }
    gains[static_cast<size_t>(num_bins)] = gains[0];
}

} // namespace green_vocoder::dsp
