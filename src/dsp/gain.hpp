#pragma once
#include <span>
#include <cmath>

namespace dsp {

class Gain {
public:
    static constexpr float kDecayTime = 5.0f;
    static constexpr float kMinDb = -60.0f;
    void Init(float sample_rate, int block) {
        decay_ = std::exp(-1.0f / ((sample_rate / block) * kDecayTime / 1000.0f));
    }

    void Process(std::span<float> block) {
        for (float& s : block) {
            s *= gain_;
        }

        for (float s : block) {
            if (std::abs(s) > peak_) peak_ = std::abs(s);
        }

        peak_ *= decay_;
    }

    void SetGain(float db) {
        if (db < kMinDb) gain_ = 0.0f;
        else gain_ = pow(10.0f, db / 20.0f);
    }
    
    float peak_{};
private:
    float decay_{};
    float gain_{};
};

}