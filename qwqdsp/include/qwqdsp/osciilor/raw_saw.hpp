#pragma once
#include <span>

namespace qwqdsp::oscillor {
class RawSaw {
public:
    void Init(float fs) {
        fs_ = fs;
    }

    void SetFreq(float freq) {
        inc_ = freq / fs_;
    }

    float Tick() {
        phase_ += inc_;
        if (phase_ > 1.0f) {
            phase_ -= 1.0f;
        }
        return phase_ * 2.0f - 1.0f;
    }

    void Process(std::span<float> block) {
        for (auto& s : block) {
            s = Tick();
        }
    }
private:
    float fs_{};
    float inc_{};
    float phase_{};
};
}