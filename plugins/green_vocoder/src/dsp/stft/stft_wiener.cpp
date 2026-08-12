#include "stft_wiener.hpp"

#include <cmath>

namespace green_vocoder::dsp {

void STFTWiener::Init(STFT& self) {
    SetParam(Params{}, self);
}

void STFTWiener::SetParam(const Params& p, STFT& self) {
    fft_size_ = self.fft_size_;
    num_bins_ = fft_size_ / 2 + 1;
    glitch_ = p.glitch;
    direction_ab_ = p.direction_ab;
}

void STFTWiener::operator()(STFT& self, std::span<const float> re_mod, std::span<const float> im_mod,
                            std::span<float> re_carry_win, std::span<float> im_carry_win, std::span<float> re_carry,
                            std::span<float> im_carry, int channel) {
    auto& gains = channel == 0 ? self.gains_ : self.gains2_;
    int const num_bins = num_bins_;

    // A/B 角色：direction 决定 a=A(调制器)、b=B(载波) 还是交换
    std::span<const float> re_a = direction_ab_ ? re_mod : re_carry_win;
    std::span<const float> im_a = direction_ab_ ? im_mod : im_carry_win;
    std::span<const float> re_b = direction_ab_ ? re_carry_win : re_mod;
    std::span<const float> im_b = direction_ab_ ? im_carry_win : im_mod;

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
