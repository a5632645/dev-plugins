#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../shared/juce_param_listener.hpp"

#include <span>

#include "qwqdsp/filter/iir_hilbert.hpp"
#include "qwqdsp/convert.hpp"
#include "qwqdsp/filter/int_delay.hpp"

class ARExpSmoother {
public:
    void Reset() noexcept {
        lag_ = 0;
    }

    static float GetSmoothFactor(float ms, float fs) noexcept {
        float samples = fs * ms / 1000.0f;
        if (samples < 1.0f) {
            return 0;
        }
        else {
            return std::exp(-1.0f / (fs * ms / 1000.0f));
        }
    }

    float Tick(float x) noexcept {
        if (x > lag_) {
            lag_ *= attack_a_;
            lag_ += (1 - attack_a_) * x;
        }
        else {
            lag_ *= release_a_;
            lag_ += (1 - release_a_) * x;
        }
        return lag_;
    }

    void SetAttack(float ms, float fs) noexcept {
        attack_a_ = GetSmoothFactor(ms, fs);
    }

    void SetRelease(float ms, float fs) noexcept {
        release_a_ = GetSmoothFactor(ms, fs);
    }
private:
    float lag_{};
    float attack_a_{};
    float release_a_{};
};

class Compressor {
public:
    void Init(float fs) noexcept {
        env_release_factor_ = ARExpSmoother::GetSmoothFactor(5.0f, fs);
        env2_factor_ = ARExpSmoother::GetSmoothFactor(1.0f, fs);
        x_delay_ = 1.0f * fs / 1000.0f * 2;
        delay_.Init(x_delay_);
        Reset();
    }

    void Reset() noexcept {
        smoother_.Reset();
        env_lag_ = 0;
        env2_lag_ = 0;
        delay_.Reset();
    }

    void Process(std::span<float> block) noexcept {
        for (auto& x : block) {
            float const envx = std::abs(x);
            if (envx > env_lag_) {
                env_lag_ = envx;
            }
            else {
                env_lag_ *= env_release_factor_;
                env_lag_ += (1 - env_release_factor_) * envx;
            }

            env2_lag_ *= env2_factor_;
            env2_lag_ += (1 - env2_factor_) * env_lag_;

            float apply_gain = 1;
            if (env2_lag_ < threshould_) {
                // passthrogh
            }
            else {
                float const env_db = qwqdsp::convert::Gain2Db(env2_lag_);
                float const compress_db = GetCompressDb(env_db);
                float const apply_db = compress_db - env_db;
                apply_gain = qwqdsp::convert::Db2Gain(apply_db);
            }

            apply_gain = smoother_.Tick(apply_gain);
            delay_.Push(x);
            x = delay_.GetAfterPush(x_delay_);
            x *= apply_gain;
        }
    }

    void SetAttack(float ms, float fs) noexcept {
        attack_ms_ = ms;
        smoother_.SetAttack(ms, fs);
    }

    void SetRelease(float ms, float fs) noexcept {
        release_ms_ = ms;
        smoother_.SetRelease(ms, fs);
    }

    // <0
    void SetThreshould(float db) noexcept {
        threshould_db_ = db;
        threshould_ = qwqdsp::convert::Db2Gain(db);
    }

    float ratio_{}; // 1:3 -> 1/3
    float knee_db_{}; // 6
private:
    float GetCompressDb(float indb) noexcept {
        float const cut0 = threshould_db_ - knee_db_ / 2;
        float const cut1 = threshould_db_ + knee_db_ / 2;
        if (indb <= cut0) {
            return indb;
        }
        else if (indb >= cut1) {
            return threshould_db_ + (indb - threshould_db_) * ratio_;
        }
        else {
            float t = indb - threshould_db_ + knee_db_ / 2;
            t *= t;
            return indb + (ratio_ - 1) * t / (2 * knee_db_);
        }
    }

    float env_lag_{};
    float env_release_factor_{};
    float env2_lag_{};
    float env2_factor_{};
    float threshould_db_{}; // -24
    float threshould_{};
    ARExpSmoother smoother_;
    float attack_ms_{};
    float release_ms_{};
    qwqdsp::filter::IntDelay delay_;
    size_t x_delay_{};
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

    Compressor dsp_;

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SteepFlangerAudioProcessor)
};
