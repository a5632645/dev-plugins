#pragma once
#include <array>
#include <span>
#include <vector>

#include <qwqdsp/simd_element/simd_pack.hpp>

#include "../global.hpp"

namespace green_vocoder::dsp {

// ------------------------------------------------------------
// BlockBurgLPC：块驱动 Burg LPC 声码器（帧处理算法）
// ------------------------------------------------------------
// 不持有 BlockOLA；作为 BlockFunc 由外部共享 ola 驱动，每攒够一帧
// 即执行 Burg LPC 格型分析/合成并返回待重叠相加的帧。反射系数
// 计算用岭回归（+kRidge）阻止病态，不再注入噪声。
class BlockBurgLPC {
public:
    void Init(float fs);

    struct Params {
        int block_size{1024};
        int poles{36};
        float smear{1.0f};
        float attack{10.0f};
        float formant_shift{0.0f};
    };
    void SetParam(const Params& p);
    void CopyLatticeCoeffient(std::span<float> buffer, int order);

    // BlockOLA 帧处理回调：处理一帧并返回待重叠相加的帧
    std::span<qwqdsp_simd_element::PackFloat<2> const> operator()(
        std::span<qwqdsp_simd_element::PackFloat<2> const> main,
        std::span<qwqdsp_simd_element::PackFloat<2> const> side);
private:
    std::vector<qwqdsp_simd_element::PackFloat<2>> eb_;
    std::vector<qwqdsp_simd_element::PackFloat<2>> ef_;
    std::array<qwqdsp_simd_element::PackFloat<2>, global::kMaxPoles> latticek_{};
    float fir_allpass_coeff_{};
    int fft_size_{};
    int num_poles_{};
    float sample_rate_{};
    float update_rate_{};
    float smear_factor_{};
    qwqdsp_simd_element::PackFloat<2> gain_lag_{};
    float attack_factor_{};
};

} // namespace green_vocoder::dsp
