#pragma once
#include <cstddef>
#include <numbers>
#include <utility>
#include "qwqdsp/osciilor/vic_sine_osc.hpp"

namespace qwqdsp::misc {
class CrossoverGain {
public:
    void Reset() noexcept {
        phase_ = 0;
    }

    void SetTime(size_t process_samples) noexcept {
        total_ = process_samples;
    }

    void SetTime(float ms, float fs) noexcept {
        total_ = static_cast<size_t>(std::round(ms * fs / 1000.0f));
    }

    /**
     * @return {old, new}
     */
    std::pair<float, float> Tick() noexcept {
        [[unlikely]]
        if (phase_ == total_) {
            --phase_;
            return {1.0f, 0.0f};
        }
        [[unlikely]]
        if (phase_ == 0) {
            return {0.0f, 1.0f};
        }
        float const a = static_cast<float>(phase_) / total_;
        --phase_;
        return {a, 1.0f - a};
    }

    bool IsEnd() const noexcept {
        return phase_ == 0;
    }

    void BeginNew() noexcept {
        phase_ = total_;
    }
private:
    size_t total_{};
    size_t phase_{};
};

class CrossoverPower {
public:
    void Reset() noexcept {
        osc_.Reset();
    }

    void SetTime(size_t process_samples) noexcept {
        osc_.SetFreq(std::numbers::pi_v<float> * 0.5f / process_samples);
    }

    void SetTime(float ms, float fs) noexcept {
        SetTime(static_cast<size_t>(std::round(ms * fs / 1000.0f)));
    }

    /**
     * @return {old, new}
     */
    std::pair<float, float> Tick() noexcept {
        osc_.Tick();
        return {osc_.Cosine(), osc_.Sine()};
    }

    bool IsEnd() const noexcept {
        return osc_.Cosine() <= 0.0f;
    }

    void BeginNew() noexcept {
        osc_.Reset();
    }
private:
    oscillor::VicSineOsc osc_;
};
}