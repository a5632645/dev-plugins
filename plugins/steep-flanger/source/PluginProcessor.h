#pragma once
#include "../../shared/juce_param_listener.hpp"
#include "shared.hpp"

#include "qwqdsp/misc/smoother.hpp"
#include "qwqdsp/spectral/complex_fft.hpp"
#include "qwqdsp/osciilor/vic_sine_osc.hpp"
#include "qwqdsp/psimd/align_allocator.hpp"
#include "qwqdsp/psimd/vec4.hpp"
#include "qwqdsp/psimd/vec8.hpp"
#include "qwqdsp/filter/one_pole_tpt_simd.hpp"
#include "qwqdsp/filter/iir_cpx_hilbert_stereo_simd.hpp"
#include "qwqdsp/force_inline.hpp"
#include "qwqdsp/convert.hpp"
#include "qwqdsp/polymath.hpp"
#include "qwqdsp/filter/window_fir.hpp"
#include "qwqdsp/window/kaiser.hpp"
#include "simd_detector.h"
#include "x86/sse4.1.h"
#include "x86/avx.h"

using Float32x4 = qwqdsp::psimd::Vec4f32;
using Int32x4 = qwqdsp::psimd::Vec4i32;
using Float32x8 = qwqdsp::psimd::Vec8f32;
using Int32x8 = qwqdsp::psimd::Vec8i32;

struct Complex32x4 {
    Float32x4 re;
    Float32x4 im;

    QWQDSP_FORCE_INLINE
    constexpr Complex32x4& operator*=(const Complex32x4& a) noexcept;
};

struct Complex32x8 {
    Float32x8 re;
    Float32x8 im;

    QWQDSP_FORCE_INLINE
    constexpr Complex32x8& operator*=(const Complex32x8& a) noexcept;
};

class Vec4DelayLine {
public:
    void Init(float max_ms, float fs) {
        float d = max_ms * fs / 1000.0f;
        size_t i = static_cast<size_t>(std::ceil(d));
        Init(i);
    }

    void Init(size_t max_samples) {
        size_t a = 1;
        while (a < max_samples) {
            a *= 2;
        }
        mask_ = static_cast<int>(a - 1);
        delay_length_ = static_cast<int>(a);

        a += 4;
        if (buffer_.size() < a) {
            buffer_.resize(a);
        }
        Reset();
    }

    void Reset() noexcept {
        wpos_ = 0;
        std::fill(buffer_.begin(), buffer_.end(), 0.0f);
    }

    QWQDSP_FORCE_INLINE
    Float32x4 GetAfterPush(Float32x4 delay_samples) const noexcept;

    QWQDSP_FORCE_INLINE
    Float32x8 GetAfterPush(Float32x8 const& delay_samples) const noexcept;

    QWQDSP_FORCE_INLINE
    void Push(float x) noexcept {
        buffer_[static_cast<size_t>(wpos_++)] = x;
        wpos_ &= mask_;
    }

    QWQDSP_FORCE_INLINE
    void WrapBuffer() noexcept {
        auto a = simde_mm_load_ps(buffer_.data());
        simde_mm_store_ps(buffer_.data() + delay_length_, a);
    }

private:
    std::vector<float, qwqdsp::psimd::AlignedAllocator<float, 32>> buffer_;
    int delay_length_{};
    int wpos_{};
    int mask_{};
};

class SteepFlangerParameter {
public:
    float delay_ms;
    float depth_ms;
    float lfo_freq;
    float lfo_phase;
    float fir_cutoff;
    size_t fir_coeff_len;
    float fir_side_lobe;
    bool fir_min_phase;
    bool fir_highpass;
    float feedback;
    float damp_pitch;
    bool feedback_enable;
    float barber_phase;
    float barber_speed;
    bool barber_enable;
    std::atomic<bool> should_update_fir_{};
    std::atomic<bool> is_using_custom_{};
    std::array<float, kMaxCoeffLen> custom_coeffs_{};
    std::array<float, kMaxCoeffLen> custom_spectral_gains{};
};

class SteepFlanger {
public:
    SteepFlanger() {
        complex_fft_.Init(kFFTSize);

        cpu_arch_ = 0;
        if (simd_detector::is_supported(simd_detector::InstructionSet::PLUGIN_VEC8_DISPATCH_ISET)) {
            cpu_arch_ = 2;
        }
        else if (simd_detector::is_supported(simd_detector::InstructionSet::PLUGIN_VEC4_DISPATCH_ISET)) {
            cpu_arch_ = 1;
        }
    }

    void Init(float fs, float max_delay_ms) {
        fs_ = fs;
        float const samples_need = fs * max_delay_ms / 1000.0f;
        delay_left_.Init(static_cast<size_t>(samples_need * kMaxCoeffLen));
        delay_right_.Init(static_cast<size_t>(samples_need * kMaxCoeffLen));
        barber_phase_smoother_.SetSmoothTime(20.0f, fs);
        damp_.Reset();
        barber_oscillator_.Reset();
        barber_osc_keep_amp_counter_ = 0;
        // VIC正交振荡器衰减非常慢，设定为5分钟保持一次
        barber_osc_keep_amp_need_ = static_cast<size_t>(fs * 60 * 5);
    }

    void Reset() noexcept {
        delay_left_.Reset();
        delay_right_.Reset();
        left_fb_ = 0;
        right_fb_ = 0;
        damp_.Reset();
        hilbert_complex_.Reset();
    }

    void Process(
        float* left_ptr, float* right_ptr, size_t len,
        SteepFlangerParameter& param
    ) noexcept {
        if (cpu_arch_ == 1) {
            ProcessVec4(left_ptr, right_ptr, len, param);
        }
        else if (cpu_arch_ == 2) {
            ProcessVec8(left_ptr, right_ptr, len, param);
        }
    }

    void ProcessVec8(
        float* left_ptr, float* right_ptr, size_t len,
        SteepFlangerParameter& param
    ) noexcept;

    void ProcessVec4(
        float* left_ptr, float* right_ptr, size_t len,
        SteepFlangerParameter& param
    ) noexcept;

    /**
     * @param p [0, 1]
     */
    void SetLFOPhase(float p) noexcept {
        phase_ = p;
    }

    /**
     * @param p [0, 1]
     */
    void SetBarberLFOPhase(float p) noexcept {
        barber_oscillator_.Reset(p * std::numbers::pi_v<float> * 2);
    }

    // -------------------- lookup --------------------
    std::span<const float> GetUsingCoeffs() const noexcept {
        return {coeffs_.data(), coeff_len_};
    }

    int GetCpuArch() const noexcept {
        return cpu_arch_;
    }

    std::atomic<bool> have_new_coeff_{};
private:
    void UpdateCoeff(SteepFlangerParameter& param) noexcept {
        size_t coeff_len = static_cast<size_t>(param.fir_coeff_len);
        coeff_len_ = coeff_len;

        if (!param.is_using_custom_) {
            std::span<float> kernel{coeffs_.data(), coeff_len};
            float const cutoff_w = param.fir_cutoff;
            if (param.fir_highpass) {
                qwqdsp::filter::WindowFIR::Highpass(kernel, std::numbers::pi_v<float> - cutoff_w);
            }
            else {
                qwqdsp::filter::WindowFIR::Lowpass(kernel, cutoff_w);
            }
            float const beta = qwqdsp::window::Kaiser::Beta(param.fir_side_lobe);
            qwqdsp::window::Kaiser::ApplyWindow(kernel, beta, false);
        }
        else {
            std::copy_n(param.custom_coeffs_.begin(), coeff_len, coeffs_.begin());
        }

        if (cpu_arch_ == 1) {
            size_t const coeff_len_div_4 = (coeff_len + 3) / 4;
            size_t const idxend = coeff_len_div_4 * 4;
            for (size_t i = coeff_len; i < idxend; ++i) {
                coeffs_[i] = 0;
            }
        }
        else {
            size_t const coeff_len_div_8 = (coeff_len + 7) / 8;
            size_t const idxend = coeff_len_div_8 * 8;
            for (size_t i = coeff_len; i < idxend; ++i) {
                coeffs_[i] = 0;
            }
        }

        float pad[kFFTSize]{};
        std::span<float> kernel{coeffs_.data(), coeff_len};
        constexpr size_t num_bins = complex_fft_.NumBins(kFFTSize);
        std::array<float, num_bins> gains{};
        if (param.fir_min_phase || param.feedback_enable) {
            std::copy(kernel.begin(), kernel.end(), pad);
            complex_fft_.FFTGainPhase(pad, gains);
        }

        if (param.fir_min_phase) {
            float log_gains[num_bins]{};
            for (size_t i = 0; i < num_bins; ++i) {
                log_gains[i] = std::log(gains[i] + 1e-18f);
            }

            float phases[num_bins]{};
            complex_fft_.IFFT(pad, log_gains, phases);
            pad[0] = 0;
            pad[num_bins / 2] = 0;
            for (size_t i = num_bins / 2 + 1; i < num_bins; ++i) {
                pad[i] = -pad[i];
            }

            complex_fft_.FFT(pad, log_gains, phases);
            complex_fft_.IFFTGainPhase(pad, gains, phases);

            for (size_t i = 0; i < kernel.size(); ++i) {
                kernel[i] = pad[i];
            }
        }

        if (!param.feedback_enable) {
            float energy = 0;
            for (auto x : kernel) {
                energy += x * x;
            }
            float g = 1.0f / std::sqrt(energy + 1e-18f);
            for (auto& x : kernel) {
                x *= g;
            }
        }
        else {
            float const max_spectral_gain = *std::max_element(gains.begin(), gains.end());
            float const gain = 1.0f / (max_spectral_gain + 1e-10f);
            for (auto& x : kernel) {
                x *= gain;
            }
        }

        have_new_coeff_ = true;
    }

    int cpu_arch_{};
    float fs_{};

    static constexpr size_t kSIMDMaxCoeffLen = ((kMaxCoeffLen + 7) / 8) * 8;
    static constexpr float kDelaySmoothMs = 20.0f;

    Vec4DelayLine delay_left_;
    Vec4DelayLine delay_right_;
    // fir
    alignas(32) std::array<float, kSIMDMaxCoeffLen> coeffs_{};
    alignas(32) std::array<float, kSIMDMaxCoeffLen> last_coeffs_{};

    size_t coeff_len_{};

    // delay time lfo
    float phase_{};
    Float32x4 last_exp_delay_samples_{};
    Float32x4 last_delay_samples_{};

    // feedback
    float left_fb_{};
    float right_fb_{};
    qwqdsp::filter::OnePoleTPTSimd<Float32x4> damp_;
    float damp_lowpass_coeff_{1.0f};
    float last_damp_lowpass_coeff_{1.0f};

    // barberpole
    qwqdsp::filter::StereoIIRHilbertDeeperCpx<Float32x4> hilbert_complex_;
    qwqdsp::misc::ExpSmoother barber_phase_smoother_;
    qwqdsp::oscillor::VicSineOsc barber_oscillator_;
    size_t barber_osc_keep_amp_counter_{};
    size_t barber_osc_keep_amp_need_{};

    qwqdsp::spectral::ComplexFFT complex_fft_;
};

// ---------------------------------------- juce processor ----------------------------------------
class SteepFlangerAudioProcessor final : public juce::AudioProcessor
{
public:
    //==============================================================================
    SteepFlangerAudioProcessor();
    ~SteepFlangerAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    JuceParamListener param_listener_;
    std::unique_ptr<juce::AudioProcessorValueTreeState> value_tree_;

    juce::AudioParameterFloat* param_delay_ms_;
    juce::AudioParameterFloat* param_delay_depth_ms_;
    juce::AudioParameterFloat* param_lfo_speed_;
    juce::AudioParameterFloat* param_lfo_phase_;
    juce::AudioParameterFloat* param_fir_cutoff_;
    juce::AudioParameterFloat* param_fir_coeff_len_;
    juce::AudioParameterFloat* param_fir_side_lobe_;
    juce::AudioParameterBool* param_fir_min_phase_;
    juce::AudioParameterBool* param_fir_highpass_;
    juce::AudioParameterFloat* param_feedback_;
    juce::AudioParameterFloat* param_damp_pitch_;
    juce::AudioParameterBool* param_feedback_enable_;
    juce::AudioParameterFloat* param_barber_phase_;
    juce::AudioParameterFloat* param_barber_speed_;
    juce::AudioParameterBool* param_barber_enable_;
    
    SteepFlanger dsp_;
    SteepFlangerParameter dsp_param_;

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SteepFlangerAudioProcessor)
};
