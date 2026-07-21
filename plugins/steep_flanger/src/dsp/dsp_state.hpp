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
#include "pluginshared/simd/simd.hpp"

namespace dsp {




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
} // namespace dsp
