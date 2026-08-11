#pragma once
#include <span>
#include <vector>

#include "spectrum_smoother.hpp"
#include "stft.hpp"

namespace green_vocoder::dsp {

// ------------------------------------------------------------
// STFTSmooth：OCT/ERB 平滑包络 STFT 声码器算法
// ------------------------------------------------------------
// 基于 STFT 引擎，hann 分析窗；对调制器功率谱做 OCT（倍频程）或
// ERB 平滑得到频谱包络，再按包络逐频点计算增益（同 Standard：Blend
// + 攻击/释放平滑 + 共振峰搬移）。平滑器来自
// zldsp::analyzer::SpectrumSmoother（AGPL-3.0，见 spectrum_smoother.hpp）。
struct STFTSmooth {
    enum class SmoothType {
        OCT,
        ERB,
    };

    struct Params {
        SmoothType type{SmoothType::ERB};
        float amount{1.0f};
    };

    void Init(STFT& self);
    void SetParam(const Params& p, STFT& self);
    void operator()(STFT& self,
                    std::span<const float> real_in, std::span<const float> imag_in,
                    std::span<float> real_out, std::span<float> imag_out, int channel);

private:
    zldsp::analyzer::SpectrumSmoother smoother_;
    std::vector<float> power_{};
    int fft_size_{};
    SmoothType type_{SmoothType::ERB};
    float amount_{};
};

} // namespace green_vocoder::dsp
