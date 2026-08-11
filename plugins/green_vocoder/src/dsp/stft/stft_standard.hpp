#pragma once
#include <span>

#include "stft.hpp"

namespace green_vocoder::dsp {

// ------------------------------------------------------------
// STFTStandard：标准 STFT 声码器算法（无自身状态）
// ------------------------------------------------------------
// 基于 STFT 引擎，分析窗为 sinc*hann（bandwidth）；逐频点功率
// 增益 + Blend + 攻击/释放平滑 + 共振峰搬移。
struct STFTStandard {
    void operator()(STFT& self, std::span<const float> real_in, std::span<const float> imag_in,
                    std::span<float> real_out, std::span<float> imag_out, int channel);
};

} // namespace green_vocoder::dsp
