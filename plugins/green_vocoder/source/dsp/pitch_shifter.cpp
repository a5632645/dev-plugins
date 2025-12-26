#include "pitch_shifter.hpp"
#include <array>
#include <cmath>
#include <cassert>
#include <numbers>

namespace green_vocoder::dsp {
PitchShifter::PitchShifter() {
    constexpr float pi = std::numbers::pi_v<float>;
    for (size_t i = 0; i < kNumDelay; ++i) {
        window_[i] = 0.5f * (1.0f - std::cos(2.0f * pi * static_cast<float>(i) / (kNumDelay - 0.0f)));
    }
}

void PitchShifter::Process(std::span<qwqdsp_simd_element::PackFloat<2>> block) {
    for (auto& s : block) {
        delay_[wpos_] = s;
        auto out = GetDelay(delay_pos_);
        out += GetDelay(delay_pos_ + kNumDelay / 2.0f);
        s = out;
        delay_pos_ += phase_inc_;
        if (delay_pos_ >= kNumDelay) {
            delay_pos_ -= kNumDelay;
        }
        if (delay_pos_ < 0) {
            delay_pos_ += kNumDelay;
        }
        wpos_ = (wpos_ + 1) & kDelayMask;
    }
}

void PitchShifter::SetPitchShift(float pitch) {
    float a = std::exp2(pitch / 12.0f);
    phase_inc_ = 1.0f - a;
}

qwqdsp_simd_element::PackFloat<2> PitchShifter::GetDelay(float delay) {
    float rpos = static_cast<float>(wpos_) - delay;
    size_t irpos = static_cast<size_t>(rpos) & kDelayMask;
    size_t inext = (irpos + 1) & kDelayMask;
    float frac = rpos - std::floor(rpos);
    auto v = qwqdsp_simd_element::PackOps::Lerp(delay_[irpos], delay_[inext], frac);

    size_t iwrpos = static_cast<size_t>(delay) & kDelayMask;
    size_t iwnext = (iwrpos + 1) & kDelayMask;
    float w = std::lerp(window_[iwrpos], window_[iwnext], frac);
    return v * w;
}
}
