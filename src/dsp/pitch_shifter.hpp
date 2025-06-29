/*
 * TODO: enhance formant control
*/

#pragma once
#include <span>
#include <array>

namespace dsp {

class PitchShifter {
public:
    static constexpr int kNumDelay = 2048;
    static constexpr int kDelayMask = kNumDelay - 1;
    static constexpr int kOverlap = 2;
    static constexpr int kDelayDiff = kNumDelay / kOverlap;

    PitchShifter();
    void Process(std::span<float> block);
    float ProcessSingle(float x);
    void SetPitchShift(float pitch);

    float GetDelay1(int idx);
private:
    float phase_{};
    int wpos_{ kNumDelay / 2 };
    float phase_inc_{};
    std::array<float, kNumDelay + 1> delay_{};
    std::array<float, kNumDelay> window_{};
};

}
