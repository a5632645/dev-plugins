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
    static constexpr int kNumFade = 128;
    static constexpr int kDelayMask = kNumDelay - 1;

    void Process(std::span<float> block);
    float ProcessSingle(float x);
    void SetPitchShift(float pitch);

    float GetDelay1(float delay);
    float GetBufferNoInterpolation(int delay);
private:
    float delay_pos_{ kNumDelay / 2.0f };
    float new_delay_pos_{};
    int fading_counter_{};
    int wpos_{};
    float phase_inc_{};
    std::array<float, kNumDelay> delay_{};

    float last_out_{};
    float delta_out_{};
};

}
