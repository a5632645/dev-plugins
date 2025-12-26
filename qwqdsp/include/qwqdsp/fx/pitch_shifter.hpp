#pragma once
#include <span>
#include <array>
#include <numbers>
#include <cmath>

namespace qwqdsp_fx {

class PitchShifter {
public:
    static constexpr size_t kNumDelay = 2048;
    static constexpr size_t kLatency = kNumDelay / 2;
    static constexpr size_t kDelayMask = kNumDelay - 1;

    PitchShifter() noexcept {
        constexpr float pi = std::numbers::pi_v<float>;
        for (size_t i = 0; i < kNumDelay; ++i) {
            window_[i] = 0.5f * (1.0f - std::cos(2.0f * pi * static_cast<float>(i) / (kNumDelay - 0.0f)));
        }
    }

    void Process(std::span<float> block) noexcept {
        for (auto& s : block) {
            s = Tick(s);
        }
    }

    float Tick(float x) noexcept {
        delay_[wpos_] = x;
        auto out = GetDelay(delay_pos_);
        out += GetDelay(delay_pos_ + kNumDelay / 2.0f);
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

    void SetPitchShift(float pitch) noexcept {
        float a = std::exp2(pitch / 12.0f);
        phase_inc_ = 1.0f - a;
    }
private:
    float GetDelay(float delay) noexcept {
        float rpos = static_cast<float>(wpos_) - delay;
        size_t irpos = static_cast<size_t>(rpos) & kDelayMask;
        size_t inext = (irpos + 1) & kDelayMask;
        float frac = rpos - std::floor(rpos);
        auto v = std::lerp(delay_[irpos], delay_[inext], frac);

        size_t iwrpos = static_cast<size_t>(delay) & kDelayMask;
        size_t iwnext = (iwrpos + 1) & kDelayMask;
        float w = std::lerp(window_[iwrpos], window_[iwnext], frac);
        return v * w;
    }

    float delay_pos_{};
    size_t wpos_{};
    float phase_inc_{};
    std::array<float, kNumDelay> delay_{};
    std::array<float, kNumDelay> window_{};
};

} // namespace qwqdsp_fx
