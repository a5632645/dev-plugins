#pragma once
#include "pluginshared/juce_param_listener.hpp"
#include "pluginshared/preset_manager.hpp"
#include "pluginshared/bpm_sync_lfo.hpp"
#include "shared.hpp"

#include "qwqdsp/misc/smoother.hpp"
#include "qwqdsp/spectral/complex_fft.hpp"
#include "qwqdsp/osciilor/vic_sine_osc.hpp"
#include "qwqdsp/psimd/vec4.hpp"
#include "qwqdsp/filter/one_pole_tpt_simd.hpp"
#include "qwqdsp/filter/iir_cpx_hilbert_stereo_simd.hpp"
#include "qwqdsp/force_inline.hpp"
#include "qwqdsp/convert.hpp"

using SimdType = qwqdsp::psimd::Vec4f32;
using SimdIntType = qwqdsp::psimd::Vec4i32;

class AllpassBuffer2 {
public:
    static constexpr size_t kRealNumApf = 512;
    static constexpr size_t kNumApf = kRealNumApf / SimdType::kSize;
    static constexpr size_t kIndexSize = kNumApf * SimdType::kSize;
    static constexpr size_t kIndexMask = kNumApf * SimdType::kSize - 1;
    static constexpr float kMaxIndex = kIndexSize;

    void Reset() noexcept {
        std::ranges::fill(output_buffer_, float{});
        std::ranges::fill(xlags_, float{});
    }

    QWQDSP_FORCE_INLINE
    auto GetAfterPush(SimdIntType rpos) const noexcept {
        SimdIntType mask = SimdIntType::FromSingle(kIndexMask);
        SimdIntType irpos = rpos & mask;

        struct LRSimdType {
            SimdType left;
            SimdType right;
        };

        float const* buffer = xlags_.data();
        SimdType y0;
        y0.x[0] = buffer[irpos.x[0] * 2];
        y0.x[1] = buffer[irpos.x[1] * 2];
        y0.x[2] = buffer[irpos.x[2] * 2];
        y0.x[3] = buffer[irpos.x[3] * 2];
        SimdType y1;
        y1.x[0] = buffer[irpos.x[0] * 2 + 1];
        y1.x[1] = buffer[irpos.x[1] * 2 + 1];
        y1.x[2] = buffer[irpos.x[2] * 2 + 1];
        y1.x[3] = buffer[irpos.x[3] * 2 + 1];
        return LRSimdType{y0,y1};
    }

    QWQDSP_FORCE_INLINE
    void Push(float left_x, float rightx, float left_coeff, float right_coeff, size_t num_cascade) noexcept {
        float* xlag_ptr = xlags_.data();
        float* ylag_ptr = output_buffer_.data();
        for (size_t i = 0; i < num_cascade; ++i) {
            float left_y = xlag_ptr[0] + left_coeff * (left_x - ylag_ptr[0]);
            float righty = xlag_ptr[1] + right_coeff * (rightx - ylag_ptr[1]);
            xlag_ptr[0] = left_x;
            xlag_ptr[1] = rightx;
            ylag_ptr[0] = left_y;
            ylag_ptr[1] = righty;
            left_x = left_y;
            rightx = righty;
            xlag_ptr += 2;
            ylag_ptr += 2;
        }
    }

    void SetZero(size_t last_num, size_t curr_num) noexcept {
        size_t const begin_idx = last_num * 2;
        size_t const end_idx = curr_num * 2;
        for (size_t i = begin_idx; i < end_idx; ++i) {
            output_buffer_[i] = 0;
        }
        for (size_t i = begin_idx; i < end_idx; ++i) {
            xlags_[i] = 0;
        }
    }

    /**
     * @note won't check w
     * @param w [0~pi]
     */
    static float ComputeCoeff(float w) noexcept {
        auto k = std::tan(w / 2);
        return (k - 1) / (k + 1);
    }

    static float RevertCoeff2Omega(float coeff) noexcept {
        return std::atan((1 - coeff) / (1 + coeff));
    }

    SimdType GetLR(size_t filter_idx) const noexcept {
        return SimdType{xlags_[filter_idx * 2], xlags_[filter_idx * 2 + 1]};
    }
private:
    alignas(16) std::array<float, kRealNumApf * 2> output_buffer_{};
    alignas(16) std::array<float, kRealNumApf * 2> xlags_{};
};

// ---------------------------------------- juce processor ----------------------------------------
class DeepPhaserAudioProcessor final : public juce::AudioProcessor
{
public:
    //==============================================================================
    DeepPhaserAudioProcessor();
    ~DeepPhaserAudioProcessor() override;

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
    std::unique_ptr<pluginshared::PresetManager> preset_manager_;

    juce::AudioParameterInt* param_state_;
    juce::AudioParameterFloat* param_fir_cutoff_;
    juce::AudioParameterFloat* param_fir_coeff_len_;
    juce::AudioParameterFloat* param_fir_side_lobe_;
    juce::AudioParameterBool* param_fir_min_phase_;
    juce::AudioParameterBool* param_fir_highpass_;
    juce::AudioParameterFloat* param_feedback_;
    juce::AudioParameterFloat* param_damp_pitch_;
    juce::AudioParameterFloat* param_barber_phase_;
    juce::AudioParameterBool* param_barber_enable_;
    // true: from fir, false: from apf
    juce::AudioParameterBool* param_feedback_style_;
    juce::AudioParameterFloat* param_allpass_blend_;
    juce::AudioParameterFloat* param_barber_stereo_;
    juce::AudioParameterFloat* param_blend_range_;
    juce::AudioParameterFloat* param_blend_phase_;
    juce::AudioParameterFloat* param_drywet_;

    std::atomic<bool> should_update_fir_{};
    std::atomic<bool> have_new_coeff_{};

    static constexpr size_t kSIMDMaxCoeffLen = ((kMaxCoeffLen + 3) / 4) * 4;

    AllpassBuffer2 delay_;
    // fir
    alignas(16) std::array<float, kSIMDMaxCoeffLen> coeffs_{};
    std::array<SimdType, kSIMDMaxCoeffLen / 4> last_coeffs_{};
    std::array<float, kMaxCoeffLen> custom_coeffs_{};
    std::array<float, kMaxCoeffLen> custom_spectral_gains{};
    size_t coeff_len_{};
    size_t coeff_len_div_4_{};
    float last_drywet_{1.0f};

    // allpass, basicly this is onepole allpass pole
    inline static float const kMinPitch = qwqdsp::convert::Freq2Pitch(20.0f);
    inline static float const kMaxPitch = qwqdsp::convert::Freq2Pitch(20000.0f);
    float last_left_allpass_coeff_{};
    float last_right_allpass_coeff_{};
    size_t last_num_calc_apfs_{};
    float blend_lfo_phase_{};

    // feedback
    SimdType feedback_lag_{};
    float feedback_mul_from_apf_{};
    float feedback_mul_from_fir_{1.0f};
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

    pluginshared::BpmSyncLFO<true> barber_lfo_state_;
    pluginshared::BpmSyncLFO<false> blend_lfo_state_;
    
    void Panic();
private:
    void UpdateCoeff();

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DeepPhaserAudioProcessor)
};
