#pragma once
#include <span>
#include <vector>

#include "stft.hpp"

namespace green_vocoder::dsp {

// ------------------------------------------------------------
// STFTWiener：维纳滤波器反卷积 STFT 声码器算法
// ------------------------------------------------------------
// hann 分析窗。对调制器(A)与载波(B)的功率谱做维纳反卷积，得到逐 bin 增益 g：
//   g = a / (a + b)  （反卷积比）
// a/b 为 A/B 的功率谱；开关 direction 交换 A/B 角色。
// glitch=false → g 作用在 fft(carry*win)（常规）；glitch=true → g 作用在 fft(carry)（未加窗载波，
// 帧边界不连续产生碎裂/glitch 音色）。0/0（静音）等非有限值由 NaN/INF 防护归零。无时域平滑。
struct STFTWiener {
    struct Params {
        bool glitch{false};      // true = g 作用在 fft(carry)（未加窗载波）而非 fft(carry*win)
        bool direction_ab{true}; // true = a=A(调制器), b=B(载波)；false 交换
    };

    void Init(STFT& self);
    void SetParam(const Params& p, STFT& self);
    // Process2 专用：三谱输入（fft(mod*win)、fft(carry*win)、fft(carry)），由 glitch_ 决定修改哪个谱
    void operator()(STFT& self, std::span<const float> re_mod, std::span<const float> im_mod,
                    std::span<float> re_carry_win, std::span<float> im_carry_win, std::span<float> re_carry,
                    std::span<float> im_carry, int channel);
    bool GetGlitch() const noexcept {
        return glitch_;
    }

private:
    int fft_size_{};
    int num_bins_{};
    bool glitch_{false};
    bool direction_ab_{true};
    std::vector<float> g_temp_{}; // 逐 bin 维纳增益（供共振峰搬移插值）
};

} // namespace green_vocoder::dsp
