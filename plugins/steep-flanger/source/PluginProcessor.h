#pragma once
#include "../../shared/juce_param_listener.hpp"
#include "shared.hpp"

#include "qwqdsp/misc/smoother.hpp"
#include "qwqdsp/spectral/complex_fft.hpp"
#include "qwqdsp/osciilor/vic_sine_osc.hpp"
#include "qwqdsp/psimd/align_allocator.hpp"
#include "qwqdsp/psimd/vec4.hpp"
#include "qwqdsp/filter/one_pole_tpt_simd.hpp"
#include "qwqdsp/filter/iir_cpx_hilbert_stereo_simd.hpp"
#include "x86/sse2.h"

using SimdType = qwqdsp::psimd::Vec4f32;
using SimdIntType = qwqdsp::psimd::Vec4i32;

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

    #if JUCE_CLANG
    [[clang::always_inline]]
    #elif JUCE_GCC
    [[gnu::always_inline]]
    #elif JUCE_MSVC
    __forceinline
    #else
    inline
    #endif
    SimdType GetAfterPush(SimdType delay_samples) const noexcept {
        SimdType frpos = SimdType::FromSingle(static_cast<float>(wpos_ + mask_)) - delay_samples;
        SimdIntType rpos = frpos.ToInt();
        SimdIntType mask = SimdIntType::FromSingle(mask_);
        SimdIntType irpos = rpos & mask;
        SimdType frac = frpos.Frac();

        SimdType interp0;
        simde_mm_store_ps(interp0.x, simde_mm_loadu_ps(buffer_.data() + irpos.x[0]));
        SimdType interp1;
        simde_mm_store_ps(interp1.x, simde_mm_loadu_ps(buffer_.data() + irpos.x[1]));
        SimdType interp2;
        simde_mm_store_ps(interp2.x, simde_mm_loadu_ps(buffer_.data() + irpos.x[2]));
        SimdType interp3;
        simde_mm_store_ps(interp3.x, simde_mm_loadu_ps(buffer_.data() + irpos.x[3]));

        SimdType y0;
        y0.x[0] = interp0.x[0];
        y0.x[1] = interp1.x[0];
        y0.x[2] = interp2.x[0];
        y0.x[3] = interp3.x[0];
        SimdType y1;
        y1.x[0] = interp0.x[1];
        y1.x[1] = interp1.x[1];
        y1.x[2] = interp2.x[1];
        y1.x[3] = interp3.x[1];
        SimdType y2;
        y2.x[0] = interp0.x[2];
        y2.x[1] = interp1.x[2];
        y2.x[2] = interp2.x[2];
        y2.x[3] = interp3.x[2];
        SimdType y3;
        y3.x[0] = interp0.x[3];
        y3.x[1] = interp1.x[3];
        y3.x[2] = interp2.x[3];
        y3.x[3] = interp3.x[3];

        SimdType d1 = frac - SimdType::FromSingle(1.0f);
        SimdType d2 = frac - SimdType::FromSingle(2.0f);
        SimdType d3 = frac - SimdType::FromSingle(3.0f);

        auto c1 = d1 * d2 * d3 / SimdType::FromSingle(-6.0f);
        auto c2 = d2 * d3 * SimdType::FromSingle(0.5f);
        auto c3 = d1 * d3 * SimdType::FromSingle(-0.5f);
        auto c4 = d1 * d2 / SimdType::FromSingle(6.0f);

        return y0 * c1 + frac * (y1 * c2 + y2 * c3 + y3 * c4);
    }

    void Push(float x) noexcept {
        buffer_[static_cast<size_t>(wpos_++)] = x;
        wpos_ &= mask_;
    }

    void WrapBuffer() noexcept {
        auto a = simde_mm_load_ps(buffer_.data());
        simde_mm_store_ps(buffer_.data() + delay_length_, a);
    }

private:
    std::vector<float, qwqdsp::psimd::AlignedAllocator<float, 16>> buffer_;
    int delay_length_{};
    int wpos_{};
    int mask_{};
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

    std::atomic<bool> should_update_fir_{};
    std::atomic<bool> have_new_coeff_{};

    static constexpr size_t kSIMDMaxCoeffLen = ((kMaxCoeffLen + 3) / 4) * 4;

    Vec4DelayLine delay_left_;
    Vec4DelayLine delay_right_;
    // fir
    alignas(16) std::array<float, kSIMDMaxCoeffLen> coeffs_{};
    std::array<SimdType, kSIMDMaxCoeffLen / 4> last_coeffs_{};
    std::array<float, kMaxCoeffLen> custom_coeffs_{};
    std::array<float, kMaxCoeffLen> custom_spectral_gains{};
    size_t coeff_len_{};
    size_t coeff_len_div_4_{};

    // delay time lfo
    float phase_{};
    SimdType last_exp_delay_samples_{};
    SimdType last_delay_samples_{};

    // feedback
    float left_fb_{};
    float right_fb_{};
    qwqdsp::filter::OnePoleTPTSimd<SimdType> damp_;
    float damp_lowpass_coeff_{1.0f};
    float last_damp_lowpass_coeff_{1.0f};

    // barberpole
    qwqdsp::filter::StereoIIRHilbertDeeperCpx<SimdType> hilbert_complex_;
    qwqdsp::misc::ExpSmoother barber_phase_smoother_;
    qwqdsp::oscillor::VicSineOsc barber_oscillator_;
    size_t barber_osc_keep_amp_counter_{};
    size_t barber_osc_keep_amp_need_{};

    std::atomic<bool> is_using_custom_{};
    qwqdsp::spectral::ComplexFFT complex_fft_;
    
    void Panic();
private:
    void UpdateCoeff();

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SteepFlangerAudioProcessor)
};
