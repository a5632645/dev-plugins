#pragma once
#include "../../shared/juce_param_listener.hpp"
#include <span>

#include "qwqdsp/polymath.hpp"
#include "qwqdsp/fx/delay_line_simd.hpp"
#include "qwqdsp/psimd/vec4.hpp"

using SimdType = qwqdsp::psimd::Vec4f32;

class OnePoleFilter {
public:
    void Reset() noexcept {
        latch1_ = latch1_.FromSingle(0);
    }
    
    void SetLPF(float w) noexcept {
        auto k = std::tan(w / 2);
        b0_ = k / (1 + k);
        b1_ = b0_;
        a1_ = (k - 1) / (k + 1);
    }

    void SetHPF(float w) noexcept {
        auto k = std::tan(w / 2);
        b0_ = 1 / (1 + k);
        b1_ = -b0_;
        a1_ = (k - 1) / (k + 1);
    }

    void CopyFrom(const OnePoleFilter& other) noexcept {
        b0_ = other.b0_;
        b1_ = other.b1_;
        a1_ = other.a1_;
    }

    SimdType Tick(SimdType in) noexcept {
        auto t = in - SimdType::FromSingle(a1_) * latch1_;
        auto y = t * SimdType::FromSingle(b0_) + SimdType::FromSingle(b1_) * latch1_;
        latch1_ = t;
        return y;
    }

    struct ResponceCalc {
        float b0;
        float b1;
        float a1;

        std::complex<float> operator()(float w) const noexcept {
            auto z = std::polar(1.0f, w);
            auto up = z * b0 + b1;
            auto down = z + a1;
            return up / down;
        }
    };
    ResponceCalc GetResponceCalc() const noexcept {
        return ResponceCalc{
            b0_,
            b1_,
            a1_
        };
    }
private:
    float b0_{};
    float b1_{};
    float a1_{};
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
        size_t const num_pairs = num_voices_ / SimdType::kSize;
        float const g = 1.0f / std::sqrt(static_cast<float>(num_pairs));
        size_t const num_samples = left.size();
        size_t offset = 0;
        float* left_ptr = left.data();
        float* right_ptr = right.data();

        std::array<SimdType, 256> temp_in;
        std::array<SimdType, 256> temp_out;

        while (offset != num_samples) {
            size_t const cando = std::min(256ull, num_samples - offset);
            float const delay_time_smooth_factor = 1.0f - std::exp(-1.0f / (fs_ / static_cast<float>(cando) * 20.0f / 1000.0f));

            // update delay time
            // you can see visualizer here https://www.desmos.com/calculator/5ytjkkqtbb?lang=zh-CN
            phase_ += phase_inc_ * static_cast<float>(cando);
            phase_ -= std::floor(phase_);
            float const avg_delay = (delay1 + delay2) * 0.5f;
            for (size_t i = 0; i < num_pairs; ++i) {
                auto static_a = SimdType{delay1, delay1, delay2, delay2};
                auto static_b = SimdType::FromSingle(avg_delay);
                float const lerp = static_cast<float>(i) / std::max(1.0f, static_cast<float>(num_pairs) - 1.0f);
                auto static_delay = static_a + SimdType::FromSingle(lerp) * (static_b - static_a);
                auto offsetp = SimdType{0.0f, 0.25f, 0.5f, 0.75f};
                auto sin_mod = SimdType::FromSingle(phase_) + offsetp + SimdType::FromSingle(0.25f * static_cast<float>(i) / static_cast<float>(num_pairs));
                sin_mod = sin_mod.Frac();
                for (size_t j = 0; j < 4; ++j) {
                    sin_mod.x[j] = qwqdsp::polymath::SinParabola(sin_mod.x[j] * 2 * std::numbers::pi_v<float> - std::numbers::pi_v<float>);
                }
                sin_mod = sin_mod * SimdType::FromSingle(0.5f) + SimdType::FromSingle(1.0f);
                auto delay_ms = SimdType::FromSingle(depth * kMaxModulationMs) * sin_mod + static_delay;
                delay_ms_[i] = delay_ms;

                // additional exp smooth
                auto target_delay_samples = delay_ms * SimdType::FromSingle(fs_ / 1000.0f);
                delay_samples_[i] += SimdType::FromSingle(delay_time_smooth_factor) * (target_delay_samples - delay_samples_[i]);
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
            auto curr_delay_samples = last_delay_samples_[0];
            auto delta_delay_samples = (delay_samples_[0] - last_delay_samples_[0]) / SimdType::FromSingle(static_cast<float>(cando));
            auto const curr_feedback = last_feedback_;
            auto const delta_feedback = (feedback - last_feedback_) / static_cast<float>(cando);
            auto vcurr_feedback = SimdType::FromSingle(curr_feedback);
            auto vdelta_feedback = SimdType::FromSingle(delta_feedback);
            for (size_t j = 0; j < cando; ++j) {
                curr_delay_samples += delta_delay_samples;
                vcurr_feedback += vdelta_feedback;
                auto read = delays_[0].GetAfterPush(curr_delay_samples);
                auto write = temp_in[j] + read * vcurr_feedback;
                write = lowpass_[0].Tick(write);
                write = highpass_[0].Tick(write);
                delays_[0].Push(write);
                temp_out[j] = read;
            }
            last_delay_samples_[0] = curr_delay_samples;

            // last add into
            for (size_t i = 1; i < num_pairs; ++i) {
                curr_delay_samples = last_delay_samples_[i];
                delta_delay_samples = (delay_samples_[i] - last_delay_samples_[i]) / SimdType::FromSingle(static_cast<float>(cando));
                vcurr_feedback = SimdType::FromSingle(curr_feedback);
                vdelta_feedback = SimdType::FromSingle(delta_feedback);
                for (size_t j = 0; j < cando; ++j) {
                    curr_delay_samples += delta_delay_samples;
                    vcurr_feedback += vdelta_feedback;
                    auto read = delays_[i].GetAfterPush(curr_delay_samples);
                    auto write = temp_in[j] + read * vcurr_feedback;
                    write = lowpass_[i].Tick(write);
                    write = highpass_[i].Tick(write);
                    delays_[i].Push(write);
                    temp_out[j] += read;
                }
                last_delay_samples_[i] = curr_delay_samples;
            }

            last_feedback_ = feedback;

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
            offset += cando;
        }
    }

    void SyncLFOPhase(float phase) noexcept {
        phase_ = phase;
    }

    // -------------------- params --------------------
    float depth{};
    float delay1{};
    float delay2{};
    float feedback{};
    float mix{};
    void SetRate(float freq) noexcept {
        phase_inc_ = freq / fs_;
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

    // -------------------- lookup --------------------
    std::array<SimdType, kMaxNumChorus / SimdType::kSize> delay_ms_{};

    std::pair<OnePoleFilter::ResponceCalc, OnePoleFilter::ResponceCalc> GetFilterResponceCalc() const noexcept {
        return {lowpass_.front().GetResponceCalc(), highpass_.front().GetResponceCalc()};
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

    std::array<SimdType, kMaxNumChorus / SimdType::kSize> last_delay_samples_{};
    float last_feedback_{};
};

// ---------------------------------------- juce processor ----------------------------------------

enum class LFOTempoType {
    Free = 0,
    Sync,
    SyncDot,
    SyncTri,
    NumTypes
};

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
    juce::AudioParameterChoice* param_tempo_idx_;
    juce::AudioParameterInt* param_sync_type_;

    VitalChorus dsp_;

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VitalChorusAudioProcessor)
};
