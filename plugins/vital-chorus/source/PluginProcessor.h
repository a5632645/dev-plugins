#pragma once
#include "../../shared/juce_param_listener.hpp"

#include "qwqdsp/fx/delay_line.hpp"
#include "qwqdsp/psimd/vec4.hpp"
#include "qwqdsp/polymath.hpp"
#include "qwqdsp/fx/delay_line_simd.hpp"

using SimdType = qwqdsp::psimd::Vec4f32;

class OnePoleFilter {
public:
    void Reset() noexcept {
        latch1_ = latch1_.FromSingle(0);
    }
    
    void SetLPF(float w) noexcept {
        auto k = std::tan(w / 2);
        b0_ = SimdType::FromSingle(k / (1 + k));
        b1_ = b0_;
        a1_ = SimdType::FromSingle((k - 1) / (k + 1));
    }

    void SetHPF(float w) noexcept {
        auto k = std::tan(w / 2);
        b0_ = SimdType::FromSingle(1 / (1 + k));
        b1_ = b0_ * SimdType::FromSingle(-1);
        a1_ = SimdType::FromSingle((k - 1) / (k + 1));
    }

    void CopyFrom(const OnePoleFilter& other) noexcept {
        b0_ = other.b0_;
        b1_ = other.b1_;
        a1_ = other.a1_;
    }

    SimdType Tick(SimdType in) noexcept {
        auto t = in - a1_ * latch1_;
        auto y = t * b0_ + b1_ * latch1_;
        latch1_ = t;
        return y;
    }
private:
    SimdType b0_{};
    SimdType b1_{};
    SimdType a1_{};
    SimdType latch1_{};
};

class VitalChorus {
public:
    static constexpr float kMaxModulationMs = 30;
    static constexpr float kMaxStaticDelayMs = 20;
    static constexpr float kMaxDelayMs = kMaxStaticDelayMs + kMaxModulationMs * 1.5f + 1;
    static constexpr size_t kMaxNumChorus = 16;

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
        float const g = 1.0f / std::sqrt(static_cast<float>(num_voices_ / SimdType::kSize));
        size_t const num_samples = left.size();
        size_t offset = 0;
        float* left_ptr = left.data();
        float* right_ptr = right.data();

        std::array<SimdType, 256> temp_in;
        std::array<SimdType, 256> temp_out;

        while (offset != num_samples) {
            size_t const cando = std::min(256ull, num_samples - offset);

            // update delay time
            phase_ += phase_inc_ * static_cast<float>(cando);
            phase_ -= std::floor(phase_);
            float const avg_delay = (delay1 + delay2) * 0.5f;
            size_t const num_pairs = num_voices_ / 4;
            for (size_t i = 0; i < num_pairs; ++i) {
                auto static_a = qwqdsp::psimd::Vec4f32{delay1, delay2, delay1, delay2};
                auto static_b = qwqdsp::psimd::Vec4f32::FromSingle(avg_delay);
                auto static_delay = static_a + qwqdsp::psimd::Vec4f32::FromSingle(static_cast<float>(i) / std::max(1.0f, (static_cast<float>(num_pairs) - 1.0f))) * (static_b - static_a);
                auto const offsetp = qwqdsp::psimd::Vec4f32{0.0f, 0.25f, 0.5f, 0.75f};
                auto sin_mod = qwqdsp::psimd::Vec4f32::FromSingle(phase_) + offsetp + qwqdsp::psimd::Vec4f32::FromSingle(0.25f * static_cast<float>(i) / static_cast<float>(num_pairs));
                sin_mod = sin_mod.Frac();
                for (size_t j = 0; j < 4; ++j) {
                    sin_mod.x[j] = qwqdsp::polymath::SinParabola(sin_mod.x[j] * 2 * std::numbers::pi_v<float> - std::numbers::pi_v<float>);
                }
                sin_mod = sin_mod * qwqdsp::psimd::Vec4f32::FromSingle(0.5f) + qwqdsp::psimd::Vec4f32::FromSingle(1.0f);
                auto delay_ms = qwqdsp::psimd::Vec4f32::FromSingle(depth * kMaxModulationMs) * sin_mod + static_delay;
                delay_samples_[i] = delay_ms * qwqdsp::psimd::Vec4f32::FromSingle(fs_ / 1000.0f);
            }

            // shuffle
            for (size_t j = 0; j < cando; ++j) {
                temp_in[j].x[0] = *left_ptr;
                temp_in[j].x[1] = *right_ptr;
                temp_in[j].x[2] = *left_ptr;
                temp_in[j].x[3] = *right_ptr;
                ++left_ptr;
                ++right_ptr;
            }

            // first 4 direct set
            auto fb_mul = SimdType::FromSingle(feedback);
            for (size_t j = 0; j < cando; ++j) {
                auto read = delays_[0].GetAfterPush(delay_samples_[0]);
                auto write = temp_in[j] + read * fb_mul;
                write = lowpass_[0].Tick(write);
                write = highpass_[0].Tick(write);
                delays_[0].Push(write);
                temp_out[j] = read;
            }

            // last add into
            for (size_t i = 1; i < num_pairs; ++i) {
                for (size_t j = 0; j < cando; ++j) {
                    auto read = delays_[i].GetAfterPush(delay_samples_[i]);
                    auto write = temp_in[j] + read * fb_mul;
                    write = lowpass_[i].Tick(write);
                    write = highpass_[i].Tick(write);
                    delays_[i].Push(write);
                    temp_out[j] += read;
                }
            }

            // shuffle back
            left_ptr -= cando;
            right_ptr -= cando;
            SimdType dry = SimdType::FromSingle(qwqdsp::polymath::CosPi(mix * std::numbers::pi_v<float> * 0.5f));
            SimdType wet = SimdType::FromSingle(g * qwqdsp::polymath::SinPi(mix * std::numbers::pi_v<float> * 0.5f));
            for (size_t j = 0; j < cando; ++j) {
                SimdType t = dry * temp_in[j] + wet * temp_out[j];
                *left_ptr = t.x[0] + t.x[2];
                *right_ptr = t.x[1] + t.x[3];
                ++left_ptr;
                ++right_ptr;
            }

            // ProcessSingle<false>(temp_left, temp_right, left.data() + offset, right.data() + offset, cando, 0, 1);
            // for (size_t i = 2; i < num_voices_; i += 2) {
            //     ProcessSingle<true>(temp_left, temp_right, left.data() + offset, right.data() + offset, cando, i, i + 1);
            // }
            // for (size_t i = 0; i < cando; ++i) {
            //     size_t idx = i + offset;
            //     left[idx] = std::lerp(left[idx], temp_left[i] * g, mix);
            //     right[idx] = std::lerp(right[idx], temp_right[i] * g, mix);
            // }
            offset += cando;
        }
    }

    // template<bool AddInto>
    // void ProcessSingle(float* dst_left, float* dst_right, float* src_left, float* src_right, size_t len, size_t left_idx, size_t right_idx) noexcept {
    //     float delay_samples = delay_samples_[left_idx];
    //     float fb = fb_values_[left_idx];
    //     for (size_t i = 0; i < len; ++i) {
    //         delays_[left_idx].Push(*src_left + fb);
    //         float read = delays_[left_idx].GetAfterPush(delay_samples);
    //         fb = feedback * lowpass_[left_idx].Tick(highpass_[left_idx].Tick(read));
    //         if constexpr (AddInto) {
    //             *dst_left += read;
    //         }
    //         else {
    //             *dst_left = read;
    //         }
    //         ++src_left;
    //         ++dst_left;
    //     }
    //     fb_values_[left_idx] = fb;

    //     delay_samples = delay_samples_[right_idx];
    //     fb = fb_values_[right_idx];
    //     for (size_t i = 0; i < len; ++i) {
    //         delays_[right_idx].Push(*src_right + fb);
    //         float read = delays_[right_idx].GetAfterPush(delay_samples);
    //         fb = feedback * lowpass_[right_idx].Tick(highpass_[right_idx].Tick(read));
    //         if constexpr (AddInto) {
    //             *dst_right += read;
    //         }
    //         else {
    //             *dst_right = read;
    //         }
    //         ++dst_right;
    //         ++src_right;
    //     }
    //     fb_values_[right_idx] = fb;
    // }

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
        for (size_t i = 1; i < kMaxNumChorus / SimdType::kSize; ++i) {
            lowpass_[i].CopyFrom(lowpass_[0]);
            highpass_[i].CopyFrom(highpass_[0]);
        }
    }
    void SetNumVoices(size_t num_voices) noexcept {
        for (size_t i = num_voices_ / SimdType::kSize; i < num_voices / SimdType::kSize; ++i) {
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
    std::array<SimdType, kMaxNumChorus / SimdType::kSize> delay_samples_{};
    std::array<OnePoleFilter, kMaxNumChorus / SimdType::kSize> lowpass_;
    std::array<OnePoleFilter, kMaxNumChorus / SimdType::kSize> highpass_;
    std::array<qwqdsp::fx::DelayLineSIMD<SimdType, qwqdsp::fx::DelayLineInterpSIMD::PCHIP>, kMaxNumChorus> delays_;
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
