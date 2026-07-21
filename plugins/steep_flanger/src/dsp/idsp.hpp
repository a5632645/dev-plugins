#pragma once
#include <juce_core/juce_core.h>
#include <array>
#include <memory>
#include <string_view>
#include "global.hpp"
#include "pluginshared/simd/inst.hpp"

namespace steep_flanger {

struct DspParam {
    static constexpr float kInitMaxMs = global::kMaxDelayMs + global::kModuDelayMs + 0.1f;

    enum FirSource {
        kWindowSinc,
        kTimeCoeff,
        kSpectralCoeff
    };

    // => is mapping internal
    // -------------------- delay time --------------------
    float delay_ms;  // >=0
    float depth_ms;  // >=0
    float lfo_freq;  // hz
    float lfo_phase; // 0~1 => 0~2pi
    float drywet;    // 0~1

    // -------------------- fir design --------------------
    float fir_cutoff;     // 0~pi
    size_t fir_coeff_len; // 4~kMaxCoeffLen
    float fir_side_lobe;  // >20
    bool fir_min_phase;
    bool fir_highpass;

    std::atomic<bool> should_update_fir_; // tell flanger to update coeffs
    std::atomic<FirSource> fir_source;
    juce::SpinLock custom_coeffs_lock_;
    std::array<float, global::kMaxCoeffLen> custom_coeffs_{};
    std::array<float, global::kMaxCoeffLen> custom_spectral_gains{};

    // -------------------- feedback --------------------
    float feedback; // unit is gain
    float damp_pitch;

    // -------------------- barberpole --------------------
    float barber_phase; // 0~1 => 0~2pi
    float barber_speed; // hz
    bool barber_enable;
    float barber_stereo_phase; // 0~pi/2

    // -------------------- iir --------------------
    bool iir_mode;
    bool should_update_iir_; // tell flanger to update coeffs
    size_t iir_num_filters;
    // `iir cutoff` is using `fir_cutoff`
    float ripple; // >0
};

class Idsp {
public:
    virtual ~Idsp() = default;

    virtual void Init(float fs) = 0;
    virtual void Reset() = 0;
    virtual void Update(const DspParam& p) = 0;
    virtual void Process(float* left, float* right, int num_samples) = 0;
    virtual std::string_view InstName() = 0;
};

using DspHanle = std::unique_ptr<Idsp>;

template <simd::Inst inst>
std::unique_ptr<Idsp> CreateDspImpl();

std::unique_ptr<Idsp> CreateDsp();

} // namespace steep_flanger
