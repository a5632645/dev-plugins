#pragma once
#include <span>
#include <array>
#include <qwqdsp/simd_element/simd_pack.hpp>

// --------------------------------------------------------------------------------
// Classic grain delay pitch shifter
// --------------------------------------------------------------------------------

namespace green_vocoder::dsp {

class PitchShifter {
public:
    static constexpr size_t kNumDelay = 2048;
    static constexpr size_t kLatency = kNumDelay / 2;
    static constexpr size_t kDelayMask = kNumDelay - 1;

    PitchShifter();
    void Process(std::span<qwqdsp_simd_element::PackFloat<2>> block);
    void SetPitchShift(float pitch);
private:
    qwqdsp_simd_element::PackFloat<2> GetDelay(float delay);

    float delay_pos_{};
    size_t wpos_{};
    float phase_inc_{};
    std::array<qwqdsp_simd_element::PackFloat<2>, kNumDelay> delay_{};
    std::array<float, kNumDelay> window_{};
};

}
