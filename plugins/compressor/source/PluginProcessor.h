#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../shared/juce_param_listener.hpp"

#include <span>

#include "qwqdsp/filter/iir_hilbert.hpp"
#include "qwqdsp/convert.hpp"
#include "qwqdsp/filter/int_delay.hpp"

// ---------------------------------------- simple ----------------------------------------
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
            return std::exp(-1.0f / samples);
        }
    }

    static float GetSmoothFactor(float samples) noexcept {
        if (samples < 1.0f) {
            return 0;
        }
        else {
            return std::exp(-1.0f / samples);
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
        rms_factor_ = ARExpSmoother::GetSmoothFactor(10.0f, fs);
        Reset();
    }

    void Reset() noexcept {
        smoother_.Reset();
        env_lag_ = 0;
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

            float apply_gain = 1;
            if (env_lag_ < threshould_) {
                // passthrogh
            }
            else {
                float const env_db = qwqdsp::convert::Gain2Db(env_lag_);
                float const compress_db = GetCompressDb(env_db);
                float const apply_db = compress_db - env_db;
                apply_gain = qwqdsp::convert::Db2Gain(apply_db);
            }

            apply_gain = smoother_.Tick(apply_gain);
            x *= apply_gain;
        }
    }

    void ProcessRMS(std::span<float> block) noexcept {
        for (auto& x : block) {
            float const s = x * x;
            rms_lag_ *= rms_factor_;
            rms_lag_ += (1 - rms_factor_) * s;

            float apply_gain = 1;
            if (rms_lag_ < threshould_) {
                // passthrogh
            }
            else {
                float const env_db = qwqdsp::convert::Gain2Db(rms_lag_);
                float const compress_db = GetCompressDb(env_db);
                float const apply_db = compress_db - env_db;
                apply_gain = qwqdsp::convert::Db2Gain(apply_db);
            }

            apply_gain = smoother_.Tick(apply_gain);
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
    float threshould_db_{}; // -24
    float threshould_{};
    ARExpSmoother smoother_;
    float attack_ms_{};
    float release_ms_{};

    float rms_lag_{};
    float rms_factor_{};
};

// ---------------------------------------- vital's ----------------------------------------
class VitalCompressor {
public:
    void Init(float fs) noexcept {
        fs_ = fs;
    }

    void Reset() noexcept {
        up_rms_ = 0;
        down_rms_ = 0;
    }

    void Process(std::span<float> block) noexcept {
        float attack_samples = attack_ms_ * fs_ / 1000.0f;
        float release_samples = release_ms_ * fs_ / 1000.0f;
        attack_samples = std::max(attack_samples, 5.0f);
        release_samples = std::max(release_samples, 5.0f);

        float const attack_scale = 1.0f / (attack_samples + 1.0f);
        float const release_scale = 1.0f / (release_samples + 1.0f);

        float up_rms_threshold = qwqdsp::convert::Db2Gain(up_threshold_db_);
        float down_rms_threshold = qwqdsp::convert::Db2Gain(down_threshold_db_);
        up_rms_threshold *= up_rms_threshold;
        down_rms_threshold *= down_rms_threshold;

        for (auto& x : block) {
            float const rms = x * x;
            // this is
            //              x^2 + rms_state * L
            // rms_state = ---------------------
            //                      1 + L
            // L = num_samples
            if (rms > up_rms_) {
                up_rms_ = (rms + up_rms_ * attack_samples) * attack_scale;
            }
            else {
                up_rms_ = (rms + up_rms_ * release_samples) * release_scale;
            }
            if (rms > down_rms_) {
                down_rms_ = (rms + down_rms_ * attack_samples) * attack_scale;
            }
            else {
                down_rms_ = (rms + down_rms_ * release_samples) * release_scale;
            }

            // mag_delta always >= 1
            // ratio > 0
            // this will like a log function
            // so louder mul smaller
            up_rms_ = std::max(up_rms_, up_rms_threshold);
            float const up_mag_delta = up_rms_threshold / up_rms_;
            float const up_mul = std::pow(up_mag_delta, up_ratio_);

            // mag_delta always <= 1
            // ratio > 0 like a x^high_order function
            // ratio < 0 like a 1/x function
            // so smaller mul louder
            down_rms_ = std::min(down_rms_, down_rms_threshold);
            float const down_mag_delta = down_rms_threshold / down_rms_;
            float const down_mul = std::pow(down_mag_delta, down_ratio_);

            float const gain = std::clamp(up_mul * down_mul, 0.0f, 32.0f);
            x *= gain;
        }
    }

    // settings
    // 0~0.5
    void SetUpRatio(float ratio) noexcept {
        up_ratio_ = ratio;
    }
    // -0.5~0.5
    void SetDownRatio(float ratio) noexcept {
        down_ratio_ = ratio;
    }
    // -100~12
    void SetUpThreshold(float db) noexcept {
        up_threshold_db_ = db;
    }
    // -100~12
    void SetDownThreshold(float db) noexcept {
        down_threshold_db_ = db;
    }
    // > 0
    void SetAttack(float ms) noexcept {
        attack_ms_ = ms;
    }
    // > 0
    void SetRelease(float ms) noexcept {
        release_ms_ = ms;
    }
private:
    // params
    float fs_{};
    float attack_ms_{};
    float release_ms_{};
    float up_ratio_{};
    float down_ratio_{};
    float up_threshold_db_{};
    float down_threshold_db_{};
    // state
    float up_rms_{};
    float down_rms_{};
};

// ---------------------------------------- juce processor ----------------------------------------
class EmptyAudioProcessor final : public juce::AudioProcessor
{
public:
    //==============================================================================
    EmptyAudioProcessor();
    ~EmptyAudioProcessor() override;

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
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EmptyAudioProcessor)
};
