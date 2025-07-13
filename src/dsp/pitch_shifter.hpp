#pragma once
#include <span>
#include <array>

// --------------------------------------------------------------------------------
// Classic grain delay pitch shifter
// --------------------------------------------------------------------------------

namespace dsp {

class PitchShifter {
public:
    static constexpr int kNumDelay = 1024;
    static constexpr int kDelayMask = kNumDelay - 1;

    PitchShifter();
    void Process(std::span<float> block);
    float ProcessSingle(float x);
    void SetPitchShift(float pitch);

private:
    float GetDelay1(float delay);

    float delay_pos_{};
    int wpos_{};
    float phase_inc_{};
    std::array<float, kNumDelay> delay_{};
    std::array<float, kNumDelay> window_{};
};

}
