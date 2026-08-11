#pragma once
#include <complex>
#include <span>
#include <vector>

#include <qwqdsp/spectral/complex_fft_adv.hpp>

#include "stft.hpp"

namespace green_vocoder::dsp {

// ------------------------------------------------------------
// STFTMorph：Prosoniq 风格频谱变形声码器算法
// ------------------------------------------------------------
// hann 分析窗。对调制器(A)与载波(B)复频谱做 cepstral 包络分离后，
// 按 morph∈[0,1] 做"包络交叉"变形（无交叉淡化）：
//   result = wa*wb*(A 精细结构 × B 包络) + b11*A + a11*B
//   a11 = morph^11，b11 = (1-morph)^11，wa = 1-b11，wb = 1-a11
// 精细结构比 ra=ma/ea 带相对下界 + 上限 4（12 dB）保护，防稀疏谱/深谷处的
// 病态放大；最终输出做 tanh 软限幅防削波。
// 开关 direction 控制 A→B 还是 B→A（交换 A/B 角色）。
struct STFTMorph {
    struct Params {
        float morph{0.5f};       // 变形量 [0,1]（0=A，1=B）
        bool direction_ab{true}; // true = A→B，false = B→A
    };

    void Init(STFT& self);
    void SetParam(const Params& p, STFT& self);
    void operator()(STFT& self, std::span<const float> real_in, std::span<const float> imag_in,
                    std::span<float> real_out, std::span<float> imag_out, int channel);

private:
    // cepstral 包络提取（全尺寸复 FFT）
    void ExtractEnvelope(std::span<const float> magnitude, std::span<float> envelope);

    qwqdsp_spectral::ComplexFftAdv cep_fft_;
    std::vector<std::complex<float>> shifted_;
    std::vector<std::complex<float>> quefrency_;
    std::vector<std::complex<float>> spectral_;
    std::vector<float> envelope_window_; // 预计算 Blackman 窗（长度 right_-left_）
    std::vector<float> mag_a_;
    std::vector<float> mag_b_;
    std::vector<float> envelope_a_;
    std::vector<float> envelope_b_;
    std::vector<float> out_re_;
    std::vector<float> out_im_;
    int fft_size_{};
    int num_bins_{};
    int left_{};
    int right_{};
    float morph_{0.5f};
    bool direction_ab_{true};
};

} // namespace green_vocoder::dsp
