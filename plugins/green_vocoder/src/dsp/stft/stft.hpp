#pragma once
#include <array>
#include <span>
#include <vector>

#include <qwqdsp/simd_element/simd_pack.hpp>
#include <qwqdsp/spectral/real_fft_adv.hpp>

#include "../block_ola.hpp"
#include "../../global.hpp"

namespace green_vocoder::dsp {

using PackFloat2 = qwqdsp_simd_element::PackFloat<2>;

// ------------------------------------------------------------
// STFT：基础短时傅里叶变换帧处理引擎
// ------------------------------------------------------------
// 在 BlockOLA 之上提供完整的块驱动 STFT 帧处理链路：
//   分析加窗 → FFT → 频谱处理（算法 functor） → IFFT → 重叠相加
// 频谱算法通过 Process 模板传入（须提供 operator()），分析窗也
// 作为参数显式传入（仅 Standard 用 sinc*hann，其余用 hann）。
class STFT {
public:
    struct Params {
        float attack{1.0f};
        float release{100.0f};
        int fft_size{1024};
        float blend{0.2f};
        float formant_shift{0.0f};
        float bandwidth{2.0f}; // Standard 的 sinc 分析窗带宽，同时决定 window_gain_
    };

    void Init(float fs);
    void Reset();
    void SetParam(const Params& p);
    int GetFFTSize() const noexcept { return fft_size_; }

    // 块驱动 STFT 帧处理；window 为分析窗，alg 须提供
    // operator()(STFT&, real_in, imag_in, real_out, imag_out, channel)
    template <typename STFT_ALG>
    void Process(PackFloat2* main, PackFloat2* side, int num_samples, std::span<const float> window, STFT_ALG& alg) {
        ola_.Process(main, side, static_cast<size_t>(num_samples),
                     [this, window, &alg](std::span<PackFloat2 const> main_frame,
                                          std::span<PackFloat2 const> side_frame) {
                         return (*this)(main_frame, side_frame, window, alg);
                     });
    }

    // 单帧 STFT 处理：分析加窗 → FFT → 频谱处理(alg) → IFFT → 打包
    template <typename STFT_ALG>
    std::span<PackFloat2 const> operator()(std::span<PackFloat2 const> main_frame,
                                           std::span<PackFloat2 const> side_frame,
                                           std::span<const float> window, STFT_ALG& alg) {
        // 左声道
        for (size_t i = 0; i < static_cast<size_t>(fft_size_); ++i) {
            temp_main_[i] = window[i] * main_frame[i][0];
            temp_side_[i] = side_frame[i][0];
        }
        fft_.FFT({temp_main_.data(), static_cast<size_t>(fft_size_)}, real_main_, imag_main_);
        fft_.FFT({temp_side_.data(), static_cast<size_t>(fft_size_)}, real_side_, imag_side_);
        alg(*this, real_main_, imag_main_, real_side_, imag_side_, 0);
        fft_.IFFT({temp_main_.data(), static_cast<size_t>(fft_size_)}, real_side_, imag_side_);

        // 右声道
        for (size_t i = 0; i < static_cast<size_t>(fft_size_); ++i) {
            temp_main_[static_cast<size_t>(fft_size_) + i] = window[i] * main_frame[i][1];
            temp_side_[static_cast<size_t>(fft_size_) + i] = side_frame[i][1];
        }
        fft_.FFT({temp_main_.data() + fft_size_, static_cast<size_t>(fft_size_)}, real_main_, imag_main_);
        fft_.FFT({temp_side_.data() + fft_size_, static_cast<size_t>(fft_size_)}, real_side_, imag_side_);
        alg(*this, real_main_, imag_main_, real_side_, imag_side_, 1);
        fft_.IFFT({temp_main_.data() + fft_size_, static_cast<size_t>(fft_size_)}, real_side_, imag_side_);

        // 打包输出帧
        for (size_t i = 0; i < static_cast<size_t>(fft_size_); ++i)
            output_frame_[i] = {temp_main_[i], temp_main_[static_cast<size_t>(fft_size_) + i]};
        return std::span<PackFloat2 const>{output_frame_};
    }

    // 公共派生状态（算法 functor 经 self 读取）
    int fft_size_{};
    float sample_rate_{};
    float attack_factor_{}; // 频谱增益攻击因子
    float decay_{};         // 频谱增益释放因子
    float formant_mul_{};   // 共振峰搬移倍率
    float blend_{};
    float window_gain_{}; // 分析窗重建增益

    // 窗（hann 分析与合成共用；window_ 仅 Standard 用作分析窗）
    std::vector<float> hann_window_{};
    std::vector<float> window_{};

    // GUI 读取
    std::vector<float> gains_{};
    std::vector<float> gains2_{};
    std::array<float, global::kMaxNumMfcc> mfcc_gains_{};
    std::array<float, global::kMaxNumMfcc> mfcc_gains2_{};

    // 频谱平滑辅助
    float Blend(float x) noexcept;

private:
    BlockOLA<PackFloat2> ola_;
    qwqdsp_spectral::RealFftAdv fft_;
    std::vector<float> temp_main_;
    std::vector<float> temp_side_;
    std::vector<float> real_main_;
    std::vector<float> real_side_;
    std::vector<float> imag_main_;
    std::vector<float> imag_side_;
    std::vector<PackFloat2> output_frame_;
};

} // namespace green_vocoder::dsp
