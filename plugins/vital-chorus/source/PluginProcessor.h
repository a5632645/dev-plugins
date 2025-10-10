#pragma once
#include "../../shared/juce_param_listener.hpp"

#include "qwqdsp/fx/delay_line.hpp"
#include "qwqdsp/filter/one_pole.hpp"
#include "qwqdsp/psimd/vec4.hpp"
#include "qwqdsp/polymath.hpp"

class VitalChorus {
public:
    static constexpr float kMaxModulationMs = 30;
    static constexpr float kMaxStaticDelayMs = 20;
    static constexpr float kMaxDelayMs = kMaxStaticDelayMs + kMaxModulationMs * 1.5f + 1;
    static constexpr size_t kMaxNumChorus = 64;

    void Init(float fs) {
        float const samples = fs * kMaxDelayMs / 1000.0f;
        for (auto& d : delays_) {
            d.Init(static_cast<size_t>(std::ceil(samples)));
        }
        fs_ = fs;
    }

    void Reset() noexcept {
        for (auto& d : delays_) {
            d.Reset();
        }
        for (auto& f : lowpass_) {
            f.Reset();
        }
        for (auto& f : highpass_) {
            f.Reset();
        }
    }

    void Process(std::span<float> left, std::span<float> right) noexcept {
        size_t const num_samples = left.size();
        phase_ += phase_inc_ * num_samples;
        phase_ -= std::floor(phase_);
        float const avg_delay = (delay1 + delay2) * 0.5f;
        size_t const num_pairs = num_voices_ / 4;
        for (size_t i = 0; i < num_pairs; ++i) {
            auto static_a = qwqdsp::psimd::Vec4f32{delay1, delay2, delay1, delay2};
            auto static_b = qwqdsp::psimd::Vec4f32::FromSingle(avg_delay);
            auto static_delay = static_a + qwqdsp::psimd::Vec4f32::FromSingle(i / std::max(1.0f, (num_pairs - 1.0f))) * (static_b - static_a);
            auto const offsetp = qwqdsp::psimd::Vec4f32{0.0f, 0.25f, 0.5f, 0.75f};
            auto sin_mod = qwqdsp::psimd::Vec4f32::FromSingle(phase_) + offsetp + qwqdsp::psimd::Vec4f32::FromSingle(0.25f * i / num_pairs);
            sin_mod = sin_mod.Frac();
            for (size_t j = 0; j < 4; ++j) {
                sin_mod.x[j] = qwqdsp::polymath::SinParabola(sin_mod.x[j] * 2 * std::numbers::pi_v<float> - std::numbers::pi_v<float>);
            }
            sin_mod = sin_mod * qwqdsp::psimd::Vec4f32::FromSingle(0.5f) + qwqdsp::psimd::Vec4f32::FromSingle(1.0f);
            auto delay_ms = qwqdsp::psimd::Vec4f32::FromSingle(depth * kMaxModulationMs) * sin_mod + static_delay;
            delay_ms *= qwqdsp::psimd::Vec4f32::FromSingle(fs_ / 1000.0f);
            std::copy_n(delay_ms.x, 4, i * 4 + delay_samples_.data());
        }

        size_t offset = 0;
        float temp_left[256];
        float temp_right[256];
        float g = 1.0f / std::sqrt(num_voices_);
        while (offset != num_samples) {
            size_t const cando = std::min(256ull, num_samples - offset);
            ProcessSingle<false>(temp_left, temp_right, left.data() + offset, right.data() + offset, cando, 0, 1);
            for (size_t i = 2; i < num_voices_; i += 2) {
                ProcessSingle<true>(temp_left, temp_right, left.data() + offset, right.data() + offset, cando, i, i + 1);
            }
            for (size_t i = 0; i < cando; ++i) {
                size_t idx = i + offset;
                left[idx] = std::lerp(left[idx], temp_left[i] * g, mix);
                right[idx] = std::lerp(right[idx], temp_right[i] * g, mix);
            }
            offset += cando;
        }
    }

    template<bool AddInto>
    void ProcessSingle(float* dst_left, float* dst_right, float* src_left, float* src_right, size_t len, size_t left_idx, size_t right_idx) noexcept {
        float delay_samples = delay_samples_[left_idx];
        float fb = fb_values_[left_idx];
        for (size_t i = 0; i < len; ++i) {
            delays_[left_idx].Push(*src_left + fb);
            float read = delays_[left_idx].GetAfterPush(delay_samples);
            fb = feedback * lowpass_[left_idx].Tick(highpass_[left_idx].Tick(read));
            if constexpr (AddInto) {
                *dst_left += read;
            }
            else {
                *dst_left = read;
            }
            ++src_left;
            ++dst_left;
        }
        fb_values_[left_idx] = fb;

        delay_samples = delay_samples_[right_idx];
        fb = fb_values_[right_idx];
        for (size_t i = 0; i < len; ++i) {
            delays_[right_idx].Push(*src_right + fb);
            float read = delays_[right_idx].GetAfterPush(delay_samples);
            fb = feedback * lowpass_[right_idx].Tick(highpass_[right_idx].Tick(read));
            if constexpr (AddInto) {
                *dst_right += read;
            }
            else {
                *dst_right = read;
            }
            ++dst_right;
            ++src_right;
        }
        fb_values_[right_idx] = fb;
    }

    // -------------------- params --------------------
    float depth{};
    float delay1{};
    float delay2{};
    float feedback{};
    float mix{};
    void SetRate(float freq, float fs) noexcept {
        phase_inc_ = freq / fs;
    }
    void SetFilter(float low_w, float high_w) noexcept {
        lowpass_[0].SetLPF(low_w);
        highpass_[0].SetHPF(high_w);
        for (size_t i = 1; i < kMaxNumChorus; ++i) {
            lowpass_[i].CopyFrom(lowpass_[0]);
            highpass_[i].CopyFrom(highpass_[0]);
        }
    }
    void SetNumVoices(size_t num_voices) noexcept {
        for (size_t i = num_voices_; i < num_voices; ++i) {
            lowpass_[i].Reset();
            highpass_[i].Reset();
            delays_[i].Reset();
        }
        num_voices_ = num_voices;
    }
private:
    float fs_{};
    float phase_{};
    float phase_inc_{};
    size_t num_voices_{};
    std::array<float, kMaxNumChorus> delay_samples_{};
    std::array<float, kMaxNumChorus> fb_values_{};
    std::array<qwqdsp::filter::OnePoleFilter, kMaxNumChorus> lowpass_;
    std::array<qwqdsp::filter::OnePoleFilter, kMaxNumChorus> highpass_;
    using DelayLine = qwqdsp::fx::DelayLine<qwqdsp::fx::DelayLineInterp::PCHIP>;
    std::array<DelayLine, kMaxNumChorus> delays_;
};

// ---------------------------------------- juce processor ----------------------------------------
class VitalChorusAudioProcessor final : public juce::AudioProcessor
{
public:
    //==============================================================================
    VitalChorusAudioProcessor();
    ~VitalChorusAudioProcessor() override;

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

    juce::AudioParameterFloat* param_freq_;
    juce::AudioParameterFloat* param_depth_;
    juce::AudioParameterFloat* param_delay1_;
    juce::AudioParameterFloat* param_delay2_;
    juce::AudioParameterFloat* param_feedback_;
    juce::AudioParameterFloat* param_mix_;
    juce::AudioParameterFloat* param_cutoff_;
    juce::AudioParameterFloat* param_spread_;
    juce::AudioParameterFloat* param_num_voices_;
    juce::AudioParameterBool* param_bypass_;

    VitalChorus dsp_;

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VitalChorusAudioProcessor)
};
