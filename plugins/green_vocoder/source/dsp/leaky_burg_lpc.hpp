#pragma once
#include <span>
#include <array>
#include "ExpSmoother.hpp"
#include <qwqdsp/simd_element/biquads.hpp>
#include <qwqdsp/oscillator/noise.hpp>
#include <qwqdsp/simd_element/simd_pack.hpp>

namespace green_vocoder::dsp {
class LeakyBurgLPC {
public:
    enum class Quality {
        Modern = 0,
        Legacy,
        Telephone,
    };
    static constexpr size_t kDicimateTable[] {
        1, 2, 8
    };

    static constexpr int kNumPoles = 80;
    static constexpr int kMaxDownsample = 8;
    static constexpr float kNoiseGain = 1e-5f;

    void Init(float sample_rate, size_t block_size);
    void Process(
        std::span<qwqdsp_simd_element::PackFloat<2>> main,
        std::span<qwqdsp_simd_element::PackFloat<2>> side
    );

    void SetForget(float forget);
    void SetSmooth(float smooth);
    void SetLPCOrder(int order);
    void SetGainAttack(float ms);
    void SetGainRelease(float ms);
    void SetQuality(Quality quality);

    int GetOrder() const { return lpc_order_; }
    void CopyLatticeCoeffient(std::span<float> buffer);
private:
    template<size_t kDicimate>
    void ProcessWithDicimate(
        std::span<qwqdsp_simd_element::PackFloat<2>> main,
        std::span<qwqdsp_simd_element::PackFloat<2>> side
    );
    qwqdsp_simd_element::Biquads<4> dicimate_filter_;
    qwqdsp_simd_element::PackFloat<2> upsample_latch_{};
    Quality quality_{Quality::Legacy};
    size_t dicimate_counter_{0};

    ExpSmoother<2> gain_smooth_;
    float gain_attack_{};
    float gain_release_{};

    qwqdsp_oscillator::WhiteNoise noise_;
    float sample_rate_{};
    float true_sample_rate_{};
    float forget_{};
    float forget_ms_{};
    float smooth_{};
    float smooth_ms_{};
    int lpc_order_{};

    std::array<qwqdsp_simd_element::PackFloat<2>, kNumPoles> lattice_k_{};
    std::array<qwqdsp_simd_element::PackFloat<2>, kNumPoles> iir_k_{};
    std::array<qwqdsp_simd_element::PackFloat<2>, kNumPoles> eb_lag_{};
    std::array<qwqdsp_simd_element::PackFloat<2>, kNumPoles> ebsum_{};
    std::array<qwqdsp_simd_element::PackFloat<2>, kNumPoles> efsum_{};
    std::array<qwqdsp_simd_element::PackFloat<2>, kNumPoles + 1> x_iir_{};
    std::array<qwqdsp_simd_element::PackFloat<2>, kNumPoles + 1> l_iir{};
};
}
