#include "stft_wiener.hpp"

#include <algorithm>
#include <cmath>

#include "qwqdsp/interpolation.hpp"

namespace green_vocoder::dsp {

void STFTWiener::Init(STFT& self) {
    SetParam(Params{}, self);
}

void STFTWiener::SetParam(const Params& p, STFT& self) {
    fft_size_ = self.fft_size_;
    num_bins_ = fft_size_ / 2 + 1;
    g_temp_.resize(static_cast<size_t>(num_bins_) + 1);
    glitch_ = p.glitch;
    direction_ab_ = p.direction_ab;
}

void STFTWiener::operator()(STFT& self, std::span<const float> re_mod, std::span<const float> im_mod,
                            std::span<float> re_carry_win, std::span<float> im_carry_win, std::span<float> re_carry,
                            std::span<float> im_carry, int channel) {
    auto& gains = channel == 0 ? self.gains_left_ : self.gains_right_;
    int const num_bins = num_bins_;

    // A/B 角色：direction 决定 a=A(调制器)、b=B(载波) 还是交换
    std::span<const float> re_a = direction_ab_ ? re_mod : re_carry_win;
    std::span<const float> im_a = direction_ab_ ? im_mod : im_carry_win;
    std::span<const float> re_b = direction_ab_ ? re_carry_win : re_mod;
    std::span<const float> im_b = direction_ab_ ? im_carry_win : im_mod;

    // 逐 bin 计算维纳增益存入 g_temp_，随后做共振峰搬移
    for (int i = 0; i < num_bins; ++i) {
        float const a = (re_a[static_cast<size_t>(i)] * re_a[static_cast<size_t>(i)]
                         + im_a[static_cast<size_t>(i)] * im_a[static_cast<size_t>(i)]);
        float const b = (re_b[static_cast<size_t>(i)] * re_b[static_cast<size_t>(i)]
                         + im_b[static_cast<size_t>(i)] * im_b[static_cast<size_t>(i)]);

        // 维纳反卷积增益（a/(a+b)，0/0 由 NaN/INF 防护归零）
        float g = a / (a + b);
        if (!std::isfinite(g)) {
            g = 0;
        }
        g_temp_[static_cast<size_t>(i)] = g;
    }
    g_temp_[static_cast<size_t>(num_bins)] = g_temp_[0];

    // 共振峰搬移 + 应用：idx 超出奈奎斯特时 clamp 到末 bin
    for (int i = 0; i < num_bins; ++i) {
        float idx = std::min(static_cast<float>(i) * self.formant_mul_,
                             static_cast<float>(num_bins - 1));
        float const frac = idx - std::floor(idx);
        int const iidx = static_cast<int>(idx);

        float const g = qwqdsp::Interpolation::Linear(g_temp_[static_cast<size_t>(iidx)],
                                                      g_temp_[static_cast<size_t>(iidx) + 1], frac);

        // glitch=false → 作用在 fft(carry*win)（常规）；glitch=true → 作用在 fft(carry)（未加窗载波）
        if (glitch_) {
            re_carry[static_cast<size_t>(i)] *= g;
            im_carry[static_cast<size_t>(i)] *= g;
        }
        else {
            re_carry_win[static_cast<size_t>(i)] *= g;
            im_carry_win[static_cast<size_t>(i)] *= g;
        }

        // GUI 显示（增益幅度）
        gains[static_cast<size_t>(i)] = std::abs(g);
    }
    gains[static_cast<size_t>(num_bins)] = gains[0];
}

} // namespace green_vocoder::dsp
