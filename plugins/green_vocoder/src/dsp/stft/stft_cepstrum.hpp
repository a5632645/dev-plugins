#pragma once
#include <span>
#include <vector>

#include <qwqdsp/spectral/complex_fft_adv.hpp>

#include "stft.hpp"

namespace green_vocoder::dsp {

// ------------------------------------------------------------
// STFTCepstrum：倒谱 STFT 声码器算法
// ------------------------------------------------------------
// 基于 STFT 引擎，hann 分析窗；频谱经对数幅度 → 倒谱低通
// lifter → 指数恢复 → Blend → 平滑 → 共振峰搬移。
struct STFTCepstrum {
    struct Params {
        float detail{0.3f};
    };

    void Init(STFT& self);
    void SetParam(const Params& p, STFT& self);
    void operator()(STFT& self,
                    std::span<const float> real_in, std::span<const float> imag_in,
                    std::span<float> real_out, std::span<float> imag_out, int channel);

private:
    qwqdsp_spectral::ComplexFftAdv cep_fft_;
    std::vector<float> cep_window_{};
    std::vector<float> cep_window_fft_{};
    std::vector<float> temp_;
    std::vector<float> re1_;
    std::vector<float> phase_;
    float norm_detail_{};
    float detail_{};
};

} // namespace green_vocoder::dsp
