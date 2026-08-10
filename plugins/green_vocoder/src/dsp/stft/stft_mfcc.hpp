#pragma once
#include <array>
#include <span>
#include <vector>

#include "stft.hpp"

namespace green_vocoder::dsp {

// ------------------------------------------------------------
// STFTMFCC：MFCC 频带 STFT 声码器算法
// ------------------------------------------------------------
// 基于 STFT 引擎，hann 分析窗；按 Mel 频带对调制器频谱取 RMS
// 作为各带增益，平滑后应用到载波频谱。
struct STFTMFCC {
    struct Params {
        int num_mfcc{20};
    };

    void Init(STFT& self);
    void SetParam(const Params& p, STFT& self);
    void operator()(STFT& self,
                    std::span<const float> real_in, std::span<const float> imag_in,
                    std::span<float> real_out, std::span<float> imag_out, int channel);

private:
    std::array<int, global::kMaxNumMfcc + 1> mfcc_indexs_{};
    int fft_size_{};
    int num_mfcc_{};
    std::vector<float> fill_gains_{};
};

} // namespace green_vocoder::dsp
