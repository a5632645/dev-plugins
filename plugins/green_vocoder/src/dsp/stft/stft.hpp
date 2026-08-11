#pragma once
#include <array>
#include <span>
#include <vector>

#include <qwqdsp/simd_element/simd_pack.hpp>
#include <qwqdsp/spectral/real_fft_adv.hpp>

#include "../../global.hpp"

namespace green_vocoder::dsp {

using PackFloat2 = qwqdsp_simd_element::PackFloat<2>;

// STFT 声码器模式（与 stft_type 参数索引对应）
enum class STFTMode {
    Standard,
    Cepstrum,
    MFCC,
    Smooth,
    Welch,
    Morph,
    Wiener,
};

// 参数变化检测：精确浮点相等比较（有意为之，局部抑制 -Wfloat-equal）
inline bool ParamChanged(float a, float b) noexcept {
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfloat-equal"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif
    bool const changed = a != b;
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
    return changed;
}

// ------------------------------------------------------------
// STFT：基础短时傅里叶变换帧处理引擎
// ------------------------------------------------------------
// 提供单帧 STFT 处理链路：分析加窗 → FFT → 频谱处理（算法 functor）
// → IFFT → 打包。帧驱动（分帧/hop/重叠相加）由外部共享 BlockOLA 承担。
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
        float bandwidth{2.0f}; // Standard 的 sinc 分析窗带宽
    };

    void Init(float fs);
    void Reset();
    void SetParam(const Params& p);
    int GetFFTSize() const noexcept {
        return fft_size_;
    }

    // 单帧 STFT 处理：分析加窗 → FFT → 频谱处理(alg) → IFFT → 打包；
    // window 为 mod 分析窗（hann 系用 hann_window_，Standard 用 window_）；carry 恒用 hann_window_。
    // alg 须提供 operator()(STFT&, real_in, imag_in, real_out, imag_out, channel)
    template <typename STFT_ALG>
    std::span<PackFloat2 const> Process(std::span<PackFloat2 const> main_frame, std::span<PackFloat2 const> side_frame,
                                        std::span<const float> window, STFT_ALG& alg) {
        // 左声道（mod 用模式分析窗，carry 恒用 hann）
        for (int i = 0; i < fft_size_; ++i) {
            temp_main_[static_cast<size_t>(i)] = window[static_cast<size_t>(i)] * main_frame[static_cast<size_t>(i)][0];
            temp_side_[static_cast<size_t>(i)] =
                hann_window_[static_cast<size_t>(i)] * side_frame[static_cast<size_t>(i)][0];
        }
        fft_.FFT({temp_main_.data(), static_cast<size_t>(fft_size_)}, real_main_, imag_main_);
        fft_.FFT({temp_side_.data(), static_cast<size_t>(fft_size_)}, real_side_, imag_side_);
        alg(*this, real_main_, imag_main_, real_side_, imag_side_, 0);
        fft_.IFFT({temp_main_.data(), static_cast<size_t>(fft_size_)}, real_side_, imag_side_);

        // 右声道（mod 用模式分析窗，carry 恒用 hann）
        for (int i = 0; i < fft_size_; ++i) {
            temp_main_[static_cast<size_t>(fft_size_ + i)] =
                window[static_cast<size_t>(i)] * main_frame[static_cast<size_t>(i)][1];
            temp_side_[static_cast<size_t>(fft_size_ + i)] =
                hann_window_[static_cast<size_t>(i)] * side_frame[static_cast<size_t>(i)][1];
        }
        fft_.FFT({temp_main_.data() + fft_size_, static_cast<size_t>(fft_size_)}, real_main_, imag_main_);
        fft_.FFT({temp_side_.data() + fft_size_, static_cast<size_t>(fft_size_)}, real_side_, imag_side_);
        alg(*this, real_main_, imag_main_, real_side_, imag_side_, 1);
        fft_.IFFT({temp_main_.data() + fft_size_, static_cast<size_t>(fft_size_)}, real_side_, imag_side_);

        // 打包输出帧
        for (int i = 0; i < fft_size_; ++i)
            output_frame_[static_cast<size_t>(i)] = {temp_main_[static_cast<size_t>(i)],
                                                     temp_main_[static_cast<size_t>(fft_size_ + i)]};
        return std::span<PackFloat2 const>{output_frame_};
    }

    // 公共派生状态（算法 functor 经 self 读取）
    int fft_size_{};
    float sample_rate_{};
    float attack_factor_{}; // 频谱增益攻击因子
    float decay_factor_{};  // 频谱增益释放因子
    float formant_mul_{};   // 共振峰搬移倍率
    float blend_{};
    float hann_window_gain_{};
    float hann_sinc_window_gain_{};

    // 窗（hann 分析与合成共用；window_ 仅 Standard 用作分析窗）
    std::vector<float> hann_window_{};
    std::vector<float> hann_sinc_window_{};

    // GUI 读取
    std::vector<float> gains_{};
    std::vector<float> gains2_{};
    std::array<float, global::kMaxNumMfcc> mfcc_gains_{};
    std::array<float, global::kMaxNumMfcc> mfcc_gains2_{};

    std::vector<float> const& GetGains() const noexcept {
        return gains_;
    }
    std::vector<float> const& GetGains2() const noexcept {
        return gains2_;
    }
    std::array<float, global::kMaxNumMfcc> const& GetMfccGains() const noexcept {
        return mfcc_gains_;
    }
    std::array<float, global::kMaxNumMfcc> const& GetMfccGains2() const noexcept {
        return mfcc_gains2_;
    }

    // 频谱平滑辅助
    float Blend(float x) noexcept;
private:
    qwqdsp_spectral::RealFftAdv fft_;
    float bandwidth_{}; // 上次应用的 bandwidth（用于判断是否需重算 sinc*hann 窗）
    std::vector<float> temp_main_;
    std::vector<float> temp_side_;
    std::vector<float> real_main_;
    std::vector<float> real_side_;
    std::vector<float> imag_main_;
    std::vector<float> imag_side_;
    std::vector<PackFloat2> output_frame_;
};

} // namespace green_vocoder::dsp
