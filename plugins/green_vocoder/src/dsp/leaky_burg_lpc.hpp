#pragma once
#include <array>
#include <qwqdsp/oscillator/noise.hpp>
#include <qwqdsp/simd_element/envelope_follower.hpp>
#include <qwqdsp/simd_element/simd_pack.hpp>
#include <span>

#include "../global.hpp"

namespace green_vocoder::dsp {
class LeakyBurgLPC {
public:
    struct Params {
        float forget{10.0f};
        float smooth{1.0f};
        int order{36};
        float gain_attack{10.0f};
        float gain_release{20.0f};
        float gain_hold{10.0f};
        float formant_shift{0.0f};
    };

    void Init(float sample_rate, size_t block_size);
    void Process(std::span<qwqdsp_simd_element::PackFloat<2>> main, std::span<qwqdsp_simd_element::PackFloat<2>> side);

    void SetParam(const Params& p);

    void CopyLatticeCoeffient(std::span<float> buffer, size_t order);
private:
    template <size_t kDicimate>
    void ProcessWithDicimate(std::span<qwqdsp_simd_element::PackFloat<2>> main,
                             std::span<qwqdsp_simd_element::PackFloat<2>> side);

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
    std::array<qwqdsp_simd_element::PackFloat<2>, global::kNumPoles> ebsum_{};
    std::array<qwqdsp_simd_element::PackFloat<2>, global::kNumPoles> fir_allpass_s_{};
    std::array<qwqdsp_simd_element::PackFloat<2>, global::kNumPoles> efsum_{};
    std::array<qwqdsp_simd_element::PackFloat<2>, global::kNumPoles> lattice_k_{};
    // IIR
    std::array<qwqdsp_simd_element::PackFloat<2>, global::kNumPoles> iir_k_{};
    std::array<qwqdsp_simd_element::PackFloat<2>, global::kNumPoles + 1> iir_s_;
    qwqdsp_simd_element::PackFloat<2> residual_gain_{};
};
} // namespace green_vocoder::dsp
