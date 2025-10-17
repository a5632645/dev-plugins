#pragma once
#include "../../shared/juce_param_listener.hpp"
#include "shared.hpp"

#include "qwqdsp/misc/smoother.hpp"
#include "qwqdsp/spectral/complex_fft.hpp"
#include "qwqdsp/osciilor/vic_sine_osc.hpp"
#include "qwqdsp/psimd/vec4.hpp"
#include "qwqdsp/filter/one_pole_tpt_simd.hpp"
#include "qwqdsp/filter/iir_cpx_hilbert_stereo_simd.hpp"
#include "qwqdsp/force_inline.hpp"
#include "x86/sse2.h"

using SimdType = qwqdsp::psimd::Vec4f32;
using SimdIntType = qwqdsp::psimd::Vec4i32;

class CascadeOnepoleAllpassTPT {
public:
    void Reset() noexcept {
        lag_ = SimdType::FromSingle(0);
    }
    
    static float ComputeCoeff(float w) noexcept {
        constexpr float kMaxOmega = std::numbers::pi_v<float> - 1e-5f;
        [[unlikely]]
        if (w < 0.0f) {
            return 0.0f;
        }
        else if (w > kMaxOmega) {
            return 1.0f;
        }
        else [[likely]] {
            auto k = std::tan(w / 2);
            return k / (1 + k);
        }
    }

    QWQDSP_FORCE_INLINE
    SimdType TickAllpass(float x, float coeff) noexcept {
        SimdType delta;
        SimdType y;
        for (size_t i = 0; i < SimdType::kSize; ++i) {
            delta.x[i] = coeff * (x - lag_.x[i]);
            float lp = lag_.x[i] + delta.x[i];
            x -= lp;
            x -= lp;
            y.x[i] = x;
        }
        lag_ += delta;
        lag_ += delta;
        return y;
    }
private:
    SimdType lag_{};
};

class CascadeOnepoleAllpassDF1 {
public:
    void Reset() noexcept {
        xlag_ = SimdType::FromSingle(0);
        ylag_ = SimdType::FromSingle(0);
    }
    
    static float ComputeCoeff(float w) noexcept {
        constexpr float kMaxOmega = std::numbers::pi_v<float> - 1e-5f;
        [[unlikely]]
        if (w < 0.0f) {
            return 0.0f;
        }
        else if (w > kMaxOmega) {
            return 1.0f;
        }
        else [[likely]] {
            auto k = std::tan(w / 2);
            return k / (1 + k);
        }
    }

    QWQDSP_FORCE_INLINE
    SimdType TickAllpass(float x, float coeff) noexcept {
        for (size_t i = 0; i < SimdType::kSize; ++i) {
            float y = xlag_.x[i] + coeff * (x - ylag_.x[i]);
            xlag_.x[i] = x;
            ylag_.x[i] = y;
            x = y;
        }
        return ylag_;
    }
private:
    SimdType xlag_{};
    SimdType ylag_{};
};

class AllpassBuffer {
public:
    static constexpr size_t kNumApf = 128;
    static constexpr size_t kRealNumApf = kNumApf * SimdType::kSize;
    static constexpr size_t kIndexSize = kNumApf * SimdType::kSize;
    static constexpr size_t kIndexMask = kNumApf * SimdType::kSize - 1;
    static constexpr float kMaxIndex = kIndexSize;

    void Reset() noexcept {
        std::ranges::fill(output_buffer_, SimdType{});
        for (auto& f : lags_) {
            f.Reset();
        }
    }

    QWQDSP_FORCE_INLINE
    SimdType GetAfterPush(SimdIntType rpos) const noexcept {
        SimdIntType mask = SimdIntType::FromSingle(kIndexMask);
        SimdIntType irpos = rpos & mask;

        float const* buffer = reinterpret_cast<float const*>(output_buffer_.data());
        SimdType y0;
        y0.x[0] = buffer[irpos.x[0]];
        y0.x[1] = buffer[irpos.x[1]];
        y0.x[2] = buffer[irpos.x[2]];
        y0.x[3] = buffer[irpos.x[3]];
        return y0;
    }

    QWQDSP_FORCE_INLINE
    void Push(float x, float coeff, size_t num_cascade) noexcept {
        for (size_t i = 0; i < num_cascade; ++i) {
            output_buffer_[i] = lags_[i].TickAllpass(x, coeff);
            x = output_buffer_[i].x[3];
        }
    }

    QWQDSP_FORCE_INLINE
    float Get(size_t idx) const noexcept {
        float const* buffer = reinterpret_cast<float const*>(output_buffer_.data());
        return buffer[idx];
    }
private:
    std::array<SimdType, kNumApf * 2> output_buffer_{};
    std::array<CascadeOnepoleAllpassDF1, kNumApf> lags_;
};

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

        float const* buffer = output_buffer_.data();
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
    void Push(float left_x, float rightx, float coeff, size_t num_cascade) noexcept {
        num_cascade *= SimdType::kSize;
        float* xlag_ptr = xlags_.data();
        float* ylag_ptr = output_buffer_.data();
        for (size_t i = 0; i < num_cascade; ++i) {
            float left_y = xlag_ptr[0] + coeff * (left_x - ylag_ptr[0]);
            float righty = xlag_ptr[1] + coeff * (rightx - ylag_ptr[1]);
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

    juce::AudioParameterInt* param_state_;
    juce::AudioParameterFloat* param_fir_cutoff_;
    juce::AudioParameterFloat* param_fir_coeff_len_;
    juce::AudioParameterFloat* param_fir_side_lobe_;
    juce::AudioParameterBool* param_fir_min_phase_;
    juce::AudioParameterBool* param_fir_highpass_;
    juce::AudioParameterFloat* param_feedback_;
    juce::AudioParameterFloat* param_damp_pitch_;
    juce::AudioParameterFloat* param_barber_phase_;
    juce::AudioParameterFloat* param_barber_speed_;
    juce::AudioParameterBool* param_barber_enable_;
    juce::AudioParameterFloat* param_allpass_blend_;

    std::atomic<bool> should_update_fir_{};
    std::atomic<bool> have_new_coeff_{};

    static constexpr size_t kSIMDMaxCoeffLen = ((kMaxCoeffLen + 3) / 4) * 4;

    // AllpassBuffer delay_left_;
    // AllpassBuffer delay_right_;
    AllpassBuffer2 delay_;
    // fir
    alignas(16) std::array<float, kSIMDMaxCoeffLen> coeffs_{};
    std::array<SimdType, kSIMDMaxCoeffLen / 4> last_coeffs_{};
    std::array<float, kMaxCoeffLen> custom_coeffs_{};
    std::array<float, kMaxCoeffLen> custom_spectral_gains{};
    size_t coeff_len_{};
    size_t coeff_len_div_4_{};

    // allpass
    float last_allpass_coeff_{};

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
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DeepPhaserAudioProcessor)
};
