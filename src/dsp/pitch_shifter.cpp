#include "pitch_shifter.hpp"
#include <array>
#include <cmath>
#include <cassert>
#include <numbers>

namespace dsp {
PitchShifter::PitchShifter() {
    constexpr float pi = std::numbers::pi_v<float>;
    for (int i = 0; i < kNumDelay; ++i) {
        window_[i] = 0.5f * (1.0f - std::cos(2.0f * pi * i / (kNumDelay - 1.0f)));
    }
}

void PitchShifter::Process(std::span<float> block) {
    for (float& s : block) {
        s = ProcessSingle(s);
    }
}

float PitchShifter::ProcessSingle(float x) {
    delay_[wpos_] = x;
    float out = GetDelay1(delay_pos_);
    out += GetDelay1(delay_pos_ + kNumDelay / 2.0f);
    delay_pos_ += phase_inc_;
    if (delay_pos_ >= kNumDelay) {
        delay_pos_ -= kNumDelay;
    }
    if (delay_pos_ < 0) {
        delay_pos_ += kNumDelay;
    }
    wpos_ = (wpos_ + 1) & kDelayMask;
    return out;
}

void PitchShifter::SetPitchShift(float pitch) {
    float a = std::exp(pitch / 12.0f);
    phase_inc_ = 1.0f - a;
}

float PitchShifter::GetDelay1(float delay) {
    float rpos = wpos_ - delay;
    int irpos = static_cast<int>(rpos) & kDelayMask;
    int inext = (irpos + 1) & kDelayMask;
    float frac = rpos - static_cast<int>(rpos);
    float v = std::lerp(delay_[irpos], delay_[inext], frac);

    int iwrpos = static_cast<int>(delay) & kDelayMask;
    int iwnext = (iwrpos + 1) & kDelayMask;
    float w = std::lerp(window_[iwrpos], window_[iwnext], frac);
    return v * w;
}
}
