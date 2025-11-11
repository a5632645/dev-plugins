#pragma once
#include <qwqdsp/fx/plat_reverb.hpp>

namespace analogsynth {
class Reverb {
public:
    void Init(float fs) {
        reverb_.setSampleRate(fs);
    }

    void Reset() noexcept {
        reverb_.reset();
    }

    void Process(float* left, float* right, size_t num_samples) noexcept {
        reverb_.setMix(mix);
        reverb_.setPredelay(predelay);
        reverb_.setLowpass(lowpass);
        reverb_.setDecay(decay);
        reverb_.setSize(size);
        reverb_.setDamping(damping);
        reverb_.process(left, right, num_samples);
    }

    float mix{};      // 0~1
    float predelay{}; // 0~0.1
    float lowpass{};  // 20~20000
    float decay{};    // 0~1
    float size{};     // 0~2
    float damping{};  // 20~20000
private:
    qwqdsp::fx::PlateReverb reverb_;
};
}