#pragma once
#include <span>
#include <qwqdsp/fx/plat_reverb.hpp>
#include <qwqdsp/simd_element/plate_reverb.hpp>

namespace analogsynth {
class Reverb {
public:
    void Init(float fs) {
        // reverb_.setSampleRate(fs);
        reverb_.Init(fs);
    }

    void Reset() noexcept {
        // reverb_.reset();
        reverb_.Reset();
    }

    // void Process(float* left, float* right, size_t num_samples) noexcept {
    //     reverb_.setMix(mix);
    //     reverb_.setPredelay(predelay);
    //     reverb_.setLowpass(lowpass);
    //     reverb_.setDecay(decay);
    //     reverb_.setSize(size);
    //     reverb_.setDamping(damping);
    //     reverb_.process(left, right, num_samples);
    // }
    void Process(std::span<qwqdsp::psimd::Float32x4> block) noexcept {
        reverb_.SetMix(mix);
        reverb_.SetPredelay(predelay);
        reverb_.SetLowpass(lowpass);
        reverb_.SetDecay(decay);
        reverb_.SetSize(size);
        reverb_.SetDamping(damping);
        for (auto& x : block) {
            x = reverb_.Tick(x);
        }
    }

    float mix{};      // 0~1
    float predelay{}; // 0~0.1
    float lowpass{};  // 20~20000
    float decay{};    // 0~1
    float size{};     // 0~2
    float damping{};  // 20~20000
private:
    // qwqdsp::fx::PlateReverb reverb_;
    qwqdsp::simd_element::PlateReverb reverb_;
};
}