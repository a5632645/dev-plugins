#pragma once
#include <juce_core/juce_core.h>
#include <array>
#include <memory>
#include <numbers>
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
    float delay_ms{0.0f};  // >=0
    float depth_ms{0.0f};  // >=0
    float lfo_freq{0.0f};  // hz
    float lfo_phase{0.0f}; // 0~1 => 0~2pi
    float drywet{1.0f};    // 0~1

    // -------------------- fir design --------------------
    float fir_cutoff{std::numbers::pi_v<float> / 2}; // 0~pi
    size_t fir_coeff_len{8};                         // 4~kMaxCoeffLen
    float fir_side_lobe{40.0f};                      // >20
    bool fir_min_phase{false};
    bool fir_highpass{false};

    // -------------------- feedback --------------------
    float feedback{0.0f}; // unit is gain
    float damp_pitch{90.0f};

    // -------------------- barberpole --------------------
    float barber_phase{0.0f}; // 0~1 => 0~2pi
    float barber_speed{0.0f}; // hz
    bool barber_enable{false};
    float barber_stereo_phase{0.0f}; // 0~pi/2

    // -------------------- iir --------------------
    bool iir_mode{false};
    size_t iir_num_filters{4};
    // `iir cutoff` is using `fir_cutoff`
    float ripple{1.0f}; // >0
};

// 跨线程共享控制（含不可拷贝的 atomic / SpinLock）
struct DspControl {
    std::atomic<bool> should_update_fir_{};
    std::atomic<DspParam::FirSource> fir_source{DspParam::kWindowSinc};
    juce::SpinLock custom_coeffs_lock_;
    std::array<float, global::kMaxCoeffLen> custom_coeffs_{};
    std::array<float, global::kMaxCoeffLen> custom_spectral_gains{};
    std::atomic<bool> should_update_iir_{};
};

class Idsp {
public:
    virtual ~Idsp() = default;

    virtual void Init(float fs) = 0;
    virtual void Reset() = 0;
    virtual void Update(const DspParam& p, DspControl* control) = 0;
    virtual void Process(float* left, float* right, int num_samples) = 0;
    virtual std::string_view InstName() = 0;
    virtual void GetCoeffs(float* out, int n) = 0;
    virtual bool ExchangeNewCoeff() = 0;
    virtual void SyncPhase(float phase) = 0;
    virtual void SyncBarberPhase(float phase) = 0;
};

using DspHanle = std::unique_ptr<Idsp>;

template <simd::Inst inst>
std::unique_ptr<Idsp> CreateDspImpl();

std::unique_ptr<Idsp> CreateDsp();

} // namespace steep_flanger
