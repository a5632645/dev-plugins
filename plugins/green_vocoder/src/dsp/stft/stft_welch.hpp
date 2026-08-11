#pragma once
#include <array>
#include <span>
#include <vector>

#include "stft.hpp"

namespace green_vocoder::dsp {

// ------------------------------------------------------------
// STFTWelch：Welch 多帧平均 STFT 声码器算法
// ------------------------------------------------------------
// 基于 STFT 引擎，hann 分析窗。经典周期图包络估计的改进：Welch 多帧
// 功率谱平均（降方差）+ log 域攻击/释放平滑（电平无关）+ 谱下限
// （抑制齿音/音乐噪声）。含 NaN/INF 防护。
struct STFTWelch {
    struct Params {
        int welch_frames{4};    // Welch 平均帧数（1 = 关闭）
        float floor_db{-80.0f}; // 增益谱下限（dB）
    };

    void Init(STFT& self);
    void SetParam(const Params& p, STFT& self);
    void operator()(STFT& self, std::span<const float> real_in, std::span<const float> imag_in,
                    std::span<float> real_out, std::span<float> imag_out, int channel);

private:
    std::vector<float> welch_buf_; // 2 * welch_frames * num_bins（每通道环）
    std::vector<float> welch_sum_; // 2 * num_bins（每通道增量求和）
    std::array<int, 2> welch_pos_{};
    std::array<int, 2> welch_count_{};
    std::array<std::vector<float>, 2> log_gains_; // log 域平滑增益状态
    int num_bins_{};
    int welch_frames_{1};
    float window_gain_{}; // hann 分析窗重建增益（≈ 4 / fft_size）
    float floor_db_{-80.0f};
};

} // namespace green_vocoder::dsp
