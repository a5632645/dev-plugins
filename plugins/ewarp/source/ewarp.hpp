#pragma once
#include <span>

namespace ewarp {

class Ewarp {
public:
    void Init(float fs) {
        fs_ = fs;
    }

    void Reset() noexcept {
        sign_ = 1.0f;
        counter_ = 0;
    }

    void Process(
        std::span<float> left,
        std::span<float> right
    ) noexcept {
        size_t counter = counter_;
        float sign = sign_;
        
        auto it = left.begin();
        while (it != left.end()) {
            for (size_t i = counter; i < step_; ++i) {
                
            }
        }
    }

    void SetWarpFreq(float freq) noexcept {
        float step = fs_ / (freq * 2);
        if (step < 1) step = 1;
        step_ = static_cast<size_t>(step);
        counter_ %= step_;
    }

    void SetToggle(bool toggle) noexcept {
        toggle_ = toggle;
    }
private:
    float fs_{};
    size_t step_{};
    size_t counter_{};
    bool toggle_{};
    float sign_{};
};

} // namespace ewarp
