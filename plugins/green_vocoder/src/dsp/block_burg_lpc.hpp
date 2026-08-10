#pragma once
#include <array>
#include <span>
#include <vector>

#include <qwqdsp/oscillator/noise.hpp>
#include <qwqdsp/simd_element/simd_pack.hpp>

#include "block_ola.hpp"
#include "../global.hpp"

namespace green_vocoder::dsp {

// ------------------------------------------------------------
// BlockBurgLPC：块驱动 Burg LPC 声码器
// ------------------------------------------------------------
// 基于 BlockOLA 框架；每攒够一帧即执行 Burg LPC 格型分析/合成，
// 处理后的帧交回 BlockOLA 做合成加窗与重叠相加。
class BlockBurgLPC {
public:
    void Init(float fs);
    void Process(qwqdsp_simd_element::PackFloat<2>* main, qwqdsp_simd_element::PackFloat<2>* side, size_t num_samples);

    struct Params {
        size_t block_size{1024};
        size_t poles{36};
        float smear{1.0f};
        float attack{10.0f};
        float formant_shift{0.0f};
    };

    void SetParam(const Params& p);

    void CopyLatticeCoeffient(std::span<float> buffer, size_t order);

    // BlockOLA 帧处理回调：处理一帧并返回待重叠相加的帧
    std::span<qwqdsp_simd_element::PackFloat<2> const> operator()(
        std::span<qwqdsp_simd_element::PackFloat<2> const> main,
        std::span<qwqdsp_simd_element::PackFloat<2> const> side);

private:
    qwqdsp_oscillator::WhiteNoise noise_;
    BlockOLA<qwqdsp_simd_element::PackFloat<2>> ola_;
    std::vector<qwqdsp_simd_element::PackFloat<2>> eb_;
    std::vector<qwqdsp_simd_element::PackFloat<2>> ef_;
    std::array<qwqdsp_simd_element::PackFloat<2>, global::kMaxPoles> latticek_{};
    float fir_allpass_coeff_{};
    size_t fft_size_{};
    size_t num_poles_{};
    float sample_rate_{};
    float update_rate_{};
    float smear_factor_{};
    qwqdsp_simd_element::PackFloat<2> gain_lag_{};
    float attack_factor_{};
};

} // namespace green_vocoder::dsp
