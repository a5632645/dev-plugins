#pragma once
#include <array>
#include <span>

#include <qwqdsp/polymath.hpp>
// #include <qwqdsp/simd_element/delay_line_multiple.hpp>
// #include <qwqdsp/simd_element/simd_pack.hpp>
#include "pluginshared/dsp/delay_line_multiple.hpp"

using SimdType = simd::Float128;

class VitalChorus {
public:


    void Init(float fs) {

    }

    void Reset() noexcept {

    }

    void Process(std::span<float> left, std::span<float> right) noexcept {
        
    }

    void SyncLFOPhase(float phase) noexcept {
        phase_ = phase;
    }

    // -------------------- params --------------------

    void SetRate(float freq) noexcept {
        phase_inc_ = freq / fs_;
    }
    void SetFilter(float low_w, float high_w) noexcept {
        lowpass_coeff_ = ParalleOnePoleTPT::ComputeCoeff(low_w);
        highpass_coeff_ = ParalleOnePoleTPT::ComputeCoeff(high_w);
    }
    void SetNumVoices(size_t num_voices) noexcept {
        for (size_t i = num_voices_ / simd::LaneSize<SimdType>; i < num_voices / simd::LaneSize<SimdType>; ++i) {
            lowpass_[i].Reset();
            highpass_[i].Reset();
            delays_[i].Reset();
        }
        num_voices_ = num_voices;
    }

    // -------------------- lookup --------------------
private:

};
