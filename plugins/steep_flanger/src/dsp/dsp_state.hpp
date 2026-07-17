#pragma once
#include <qwqdsp/spectral/complex_fft.hpp>
#include <complex>
#include <numbers>
#include <atomic>
#include <juce_core/juce_core.h>
#include <qwqdsp/misc/smoother.hpp>
#include <qwqdsp/oscillator/vic_sine_osc.hpp>
#include "com/iirn_filter.hpp"
#include "com/xiir_delay.hpp"
#include "global.hpp"
#include "pluginshared/dsp/delay_line_1ch_4time.hpp"
#include "pluginshared/dsp/one_pole_tpt.hpp"
#include "pluginshared/dsp/stereo_iir_hilbert_cpx2.hpp"
#include "pluginshared/dsp/stereo_iir_hilbert_cpx.hpp"
#include "pluginshared/simd.hpp"

namespace dsp {
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

template <simd::IsSimdFloat SimdT>
struct DspStateN {
    // ----------------------------------------
    // fir part
    // ----------------------------------------
    pluginshared::dsp::DelayLineSingleChannelMultiTime<SimdT> delay_left_;
    pluginshared::dsp::DelayLineSingleChannelMultiTime<SimdT> delay_right_;

    // ----------------------------------------
    // iir part
    // ----------------------------------------
    com::IirNFilter<SimdT> iir_[global::kIirMaxNumFilters / simd::LaneSize<SimdT>];

    // -------------------- hilbert filter --------------------
    pluginshared::dsp::StereoIIRHilbertCpx hilbert_complex_;
};

struct DspState {
    DspStateN<simd::Float128> lane4;
    DspStateN<simd::Float256> lane8;

    juce::SpinLock coeffs_lock_;
    simd::Array256<float, global::kSIMDMaxCoeffLen> coeffs_{};
    simd::Array256<float, global::kSIMDMaxCoeffLen> last_coeffs_{};

    DspParam param{};
    std::atomic<bool> have_new_coeff_{}; // dsp_processor just update it's fir coeff
    qwqdsp_spectral::ComplexFFT complex_fft_;

    // -------------------- shared --------------------
    float fs_{};

    // fir
    float fir_gain_{1.0f};
    size_t coeff_len_{};

    // feedback
    float left_fb_{};
    float right_fb_{};
    pluginshared::dsp::OnePoleTPT<simd::Float128> damp_;
    pluginshared::dsp::OnePoleTPT<simd::Float128> dc_;
    float damp_lowpass_coeff_{1.0f};
    float last_damp_lowpass_coeff_{1.0f};

    // iir
    float iir_fir_k_{};
    bool last_iir_highpass_{false};

    // delay time lfo
    float phase_{};
    simd::Float128 last_exp_delay_samples_{};
    simd::Float128 last_delay_samples_{};

    // barberpole
    qwqdsp_misc::ExpSmoother barber_phase_smoother_;
    qwqdsp_oscillator::VicSineOsc barber_oscillator_;
    int barber_osc_keep_amp_counter_{};
    int barber_osc_keep_amp_need_{};
};

struct DspProcessor {
    void (*init)(DspState& state, float fs) noexcept;
    void (*reset)(DspState& state) noexcept;
    void (*update)(DspState& state, const DspParam& p) noexcept;
    void (*process)(DspState& state, float* left, float* right, int num_samples) noexcept;

    const char* name;

    bool IsValid() const noexcept {
        return init != nullptr;
    }
};

DspProcessor GetProcessorDsp() noexcept;
} // namespace dsp
