#pragma once
#include <span>

#include "stft.hpp"

namespace green_vocoder::dsp {

// ------------------------------------------------------------
// STFTWiener：维纳滤波器反卷积 STFT 声码器算法
// ------------------------------------------------------------
// hann 分析窗。对调制器(A)与载波(B)的功率谱做维纳反卷积，得到逐 bin 增益 g：
//   Standard  : g = a / (b + snr)            （反卷积比，snr 为正则化下限）
//   Difference: g = (a - b) / (a + b + snr)  （对称差异比，|g| ≤ 1）
// a/b 为 A/B 的功率谱；开关 direction 交换 A/B 角色。Standard 变体增益带上限
// 保护（防稀疏/静音载波处病态放大），含 NaN/INF 防护。无时域平滑。
struct STFTWiener {
    enum class Variant {
        Standard,
        Difference,
    };

    struct Params {
        Variant variant{Variant::Standard};
        float snr{0.01f};        // 正则化下限（功率域）
        bool direction_ab{true}; // true = a=A(调制器), b=B(载波)；false 交换
    };

    void Init(STFT& self);
    void SetParam(const Params& p, STFT& self);
    void operator()(STFT& self, std::span<const float> real_in, std::span<const float> imag_in,
                    std::span<float> real_out, std::span<float> imag_out, int channel);

private:
    int fft_size_{};
    int num_bins_{};
    Variant variant_{Variant::Standard};
    float snr_{0.01f};
    bool direction_ab_{true};
};

} // namespace green_vocoder::dsp
