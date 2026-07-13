#pragma once
#include <span>
#include <array>
#include <qwqdsp/oscillator/noise.hpp>
#include <qwqdsp/simd_element/simd_pack.hpp>
#include <qwqdsp/simd_element/envelope_follower.hpp>

namespace green_vocoder::dsp {
class LeakyBurgLPC {
public:
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
    void SetGainHold(float ms);
    void SetFormantShift(float shift);

    void CopyLatticeCoeffient(std::span<float> buffer, size_t order);
private:
    template<size_t kDicimate>
    void ProcessWithDicimate(
        std::span<qwqdsp_simd_element::PackFloat<2>> main,
        std::span<qwqdsp_simd_element::PackFloat<2>> side
    );

    qwqdsp_simd_element::EnevelopeFollower<2> gain_smooth_;
    float gain_attack_{};
    float gain_release_{};

    qwqdsp_oscillator::WhiteNoise noise_;
    float sample_rate_{};
    float forget_{};
    float forget_ms_{};
    float smooth_{};
    float smooth_ms_{};
    int lpc_order_{};

    // FIR lattice
    float fir_allpass_coeff_{};
    std::array<qwqdsp_simd_element::PackFloat<2>, kNumPoles> ebsum_{};
    std::array<qwqdsp_simd_element::PackFloat<2>, kNumPoles> fir_allpass_s_{};
    std::array<qwqdsp_simd_element::PackFloat<2>, kNumPoles> efsum_{};
    std::array<qwqdsp_simd_element::PackFloat<2>, kNumPoles> lattice_k_{};
    // IIR
    std::array<qwqdsp_simd_element::PackFloat<2>, kNumPoles> iir_k_{};
    std::array<qwqdsp_simd_element::PackFloat<2>, kNumPoles + 1> iir_s_;
    qwqdsp_simd_element::PackFloat<2> residual_gain_{};
};
}
