#include "pitch_shifter.hpp"
#include <cmath>
#include <cassert>
#include <limits>

namespace dsp {
void PitchShifter::Process(std::span<float> block) {
    for (float& s : block) {
        s = ProcessSingle(s);
    }
}

float PitchShifter::ProcessSingle(float x) {
    delay_[wpos_] = x;
    float out;
    if (fading_counter_ > 0) {
        float a = GetDelay1(delay_pos_);
        float b = GetDelay1(new_delay_pos_);
        out = std::lerp(b, a, fading_counter_ / static_cast<float>(kNumFade));
        --fading_counter_;
        if (fading_counter_ == 0) {
            delay_pos_ = new_delay_pos_;
        }
    } else {
        out = GetDelay1(delay_pos_);
        if (delay_pos_ >= kNumDelay - kNumFade * 2 || delay_pos_ < kNumFade * 2) {
            int min_pos = 0;
            float min_err = std::numeric_limits<float>::max();
            int start = delay_pos_ >= kNumDelay - kNumFade * 2 ? 0 : kNumDelay - kNumFade * 4;
            int end = delay_pos_ >= kNumDelay - kNumFade * 2 ? kNumDelay - kNumFade * 4 : kNumDelay;
            for (int lag = start; lag < end; ++lag) {
                float delta = delay_[(wpos_ - lag) & kDelayMask] - delay_[(wpos_ - lag - 1) & kDelayMask];
                if (delta * delta_out_ >= 0.0f) {
                    float err = std::abs(out - delay_[(wpos_ - lag) & kDelayMask]);
                    if (err < min_err) {
                        min_err = err + delta * delta_out_;
                        min_pos = lag;
                    }
                }
            }
            new_delay_pos_ = static_cast<float>(min_pos);
            fading_counter_ = kNumFade;
        }
    }
    
    delta_out_ = out - last_out_;
    last_out_ = out;
    delay_pos_ += phase_inc_;
    new_delay_pos_ += phase_inc_;
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
    return v;
}

float PitchShifter::GetBufferNoInterpolation(int delay) {
    return delay_[(wpos_ - delay) & kDelayMask];
}
}
