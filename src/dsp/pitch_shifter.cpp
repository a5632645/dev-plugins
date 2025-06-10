#include "pitch_shifter.hpp"
#include <cmath>
#include <numbers>
#include <cassert>

namespace dsp {
PitchShifter::PitchShifter() {
    constexpr float pi = std::numbers::pi_v<float>;
    for (int i = 0; i < kNumDelay; ++i) {
        window_[i] = 0.5f * (1.0f - std::cos(2.0f * pi * i / (kNumDelay - 1)));
    }
}

void PitchShifter::Process(std::span<float> block) {
    for (float& s : block) {
        s = ProcessSingle(s);
    }
}

float PitchShifter::ProcessSingle(float x) {
    delay_[wpos_] = x;
    delay_.back() = delay_.front();

    float sum = 0.0f;
    for (int i = 0; i < kOverlap; ++i) {
        sum += GetDelay1(i);
    }

    phase_ += phase_inc_;
    if (phase_ >= kNumDelay) {
        phase_ -= kNumDelay;
    }
    if (phase_ < 0) {
        phase_ += kNumDelay;
    }
    wpos_ = (wpos_ + 1) & kDelayMask;

    return sum;
}

void PitchShifter::SetPitchShift(float pitch) {
    float a = std::exp(pitch / 12.0f);
    phase_inc_ = 1.0f - a;
}

float PitchShifter::GetDelay1(int idx) {
    float delay = phase_ + idx * kDelayDiff;
    float rpos = wpos_ - delay;
    int irpos = static_cast<int>(rpos) & kDelayMask;
    int inext = irpos + 1;
    float frac = rpos - static_cast<int>(rpos);
    int iwpos = static_cast<int>(delay) & kDelayMask;
    int iwnext = (iwpos + 1) & kDelayMask;
    float fwfrac = delay - static_cast<int>(delay);
    float v = std::lerp(delay_[irpos], delay_[inext], frac);
    float w = std::lerp(window_[iwpos], window_[iwnext], fwfrac);
    return w * v;
}

}
