#pragma once
#include <atomic>
#include <juce_audio_processors/juce_audio_processors.h>
#include "pluginshared/wrap_parameters.hpp"

namespace green_vocoder {

// --------------------------------------------------------------------------------
// enums
// --------------------------------------------------------------------------------
enum eVocoderType {
    eVocoderType_LeakyBurgLPC = 0,
    eVocoderType_BlockBurgLPC,
    eVocoderType_STFTVocoder,
    eVocoderType_ChannelVocoder,
    eVocoderType_NumVocoderTypes
};

static const juce::StringArray kVocoderNames{
    "Leaky LPC",
    "Block LPC",
    "STFT",
    "Bandpass",
};

static const juce::StringArray kChannelVocoderMapNames{"linear", "mel", "log"};

// --------------------------------------------------------------------------------
// 参数变化监听：每个模块一个 Listener，把参数变化标记到 Params 对应的 atomic bool
// --------------------------------------------------------------------------------
class TiltFlagListener : public juce::AudioProcessorParameter::Listener {
public:
    explicit TiltFlagListener(std::atomic<bool>& flag)
        : flag_(flag) {}
    void parameterValueChanged(int parameterIndex, float newValue) override {
        juce::ignoreUnused(parameterIndex, newValue);
        flag_.store(true, std::memory_order_release);
    }
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {
        juce::ignoreUnused(parameterIndex, gestureIsStarting);
    }
private:
    std::atomic<bool>& flag_;
};

class LeakyLpcFlagListener : public juce::AudioProcessorParameter::Listener {
public:
    explicit LeakyLpcFlagListener(std::atomic<bool>& leaky, std::atomic<bool>* block = nullptr)
        : leaky_(leaky), block_(block) {}
    void parameterValueChanged(int parameterIndex, float newValue) override {
        juce::ignoreUnused(parameterIndex, newValue);
        leaky_.store(true, std::memory_order_release);
        if (block_ != nullptr) block_->store(true, std::memory_order_release);
    }
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {
        juce::ignoreUnused(parameterIndex, gestureIsStarting);
    }
private:
    std::atomic<bool>& leaky_;
    std::atomic<bool>* block_;
};

class StftFlagListener : public juce::AudioProcessorParameter::Listener {
public:
    explicit StftFlagListener(std::atomic<bool>& stft, std::atomic<bool>* block = nullptr)
        : stft_(stft), block_(block) {}
    void parameterValueChanged(int parameterIndex, float newValue) override {
        juce::ignoreUnused(parameterIndex, newValue);
        stft_.store(true, std::memory_order_release);
        if (block_ != nullptr) block_->store(true, std::memory_order_release);
    }
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {
        juce::ignoreUnused(parameterIndex, gestureIsStarting);
    }
private:
    std::atomic<bool>& stft_;
    std::atomic<bool>* block_;
};

class ChannelVocoderFlagListener : public juce::AudioProcessorParameter::Listener {
public:
    explicit ChannelVocoderFlagListener(std::atomic<bool>& flag)
        : flag_(flag) {}
    void parameterValueChanged(int parameterIndex, float newValue) override {
        juce::ignoreUnused(parameterIndex, newValue);
        flag_.store(true, std::memory_order_release);
    }
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {
        juce::ignoreUnused(parameterIndex, gestureIsStarting);
    }
private:
    std::atomic<bool>& flag_;
};

class PitchOscFlagListener : public juce::AudioProcessorParameter::Listener {
public:
    explicit PitchOscFlagListener(std::atomic<bool>& flag)
        : flag_(flag) {}
    void parameterValueChanged(int parameterIndex, float newValue) override {
        juce::ignoreUnused(parameterIndex, newValue);
        flag_.store(true, std::memory_order_release);
    }
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {
        juce::ignoreUnused(parameterIndex, gestureIsStarting);
    }
private:
    std::atomic<bool>& flag_;
};

class FormantShiftFlagListener : public juce::AudioProcessorParameter::Listener {
public:
    FormantShiftFlagListener(std::atomic<bool>& leaky, std::atomic<bool>& block,
                             std::atomic<bool>& stft, std::atomic<bool>& cv)
        : leaky_(leaky), block_(block), stft_(stft), cv_(cv) {}
    void parameterValueChanged(int parameterIndex, float newValue) override {
        juce::ignoreUnused(parameterIndex, newValue);
        // formant shift 同时作用于所有 vocoder 模块
        leaky_.store(true, std::memory_order_release);
        block_.store(true, std::memory_order_release);
        stft_.store(true, std::memory_order_release);
        cv_.store(true, std::memory_order_release);
    }
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {
        juce::ignoreUnused(parameterIndex, gestureIsStarting);
    }
private:
    std::atomic<bool>& leaky_;
    std::atomic<bool>& block_;
    std::atomic<bool>& stft_;
    std::atomic<bool>& cv_;
};

// --------------------------------------------------------------------------------
// params class — all plugin parameters + shared DSP control flags
// --------------------------------------------------------------------------------
class Params {
public:
    // pre fx
    pluginshared::FloatParam pre_tilt{
        "pre_tilt", {0.0f, 20.0f, 1.0f},
         10.0f
    };
    pluginshared::BoolParam channel_swap{"ch_swap", false};
    pluginshared::ChoiceParam pitch_channel{
        "pitch_ch", juce::StringArray{"Off", "Main L", "Main R", "Side L", "Side R"},
         "Off"
    };

    // vocoder type
    pluginshared::ChoiceParam vocoder_type{"vocoder_type", kVocoderNames, "STFT"};

    // pitch shifter
    pluginshared::FloatParam shift_pitch{
        "shift_pitch", {-24.0f, 24.0f, 1.0f},
         0.0f
    };

    // channel vocoder
    pluginshared::ChoiceParam cv_filter_bank_mode{
        "cv_fbank_mode",
        juce::StringArray{"bandpass 12", "stack butterworth 24", "stack butterworth 36", "flat butterworth 24",
                          "flat butterworth 36", "chebyshev 24", "chebyshev 36", "Elliptic 24", "Elliptic 36"},
        "stack butterworth 24"
    };
    pluginshared::FloatParam cv_attack{
        "cv_attack", {1.0f, 1000.0f, 1.0f},
         10.0f
    };
    pluginshared::FloatParam cv_gate{
        "cv_gate", {-100.0f, 0.0f, 1.0f},
         -100.0f
    };
    pluginshared::FloatParam cv_release{
        "cv_release", {10.0f, 32000.0f, 1.0f, 0.4f},
         150.0f
    };
    pluginshared::FloatParam cv_freq_begin{
        "cv_fbegin", {20.0f, 2000.0f, 1.0f},
         40.0f
    };
    pluginshared::FloatParam cv_freq_end{
        "cv_fend", {4000.0f, 20000.0f, 1.0f},
         12000.0f
    };
    pluginshared::FloatParam cv_nbands{
        "cv_nbands", {4.0f, 100.0f, 4.0f},
         20.0f
    };
    pluginshared::FloatParam cv_scale{
        "cv_scale", {0.1f, 2.0f, 0.1f},
         1.0f
    };
    pluginshared::FloatParam cv_ripple{
        "cv_ripple", {0.1f, 10.0f, 0.1f},
         1.0f
    };
    pluginshared::FloatParam cv_carry_scale{
        "cv_carry_scale", {0.1f, 2.0f, 0.1f},
         1.0f
    };
    pluginshared::ChoiceParam cv_map{"cv_map", kChannelVocoderMapNames, "mel"};

    // lpc
    pluginshared::FloatParam lpc_forget{
        "lpc_forget", {5.0f, 200.0f, 1.0f, 0.4f},
         10.0f
    };
    pluginshared::FloatParam lpc_smooth{
        "lpc_smooth", {0.0f, 50.0f, 1.0f, 0.4f},
         1.0f
    };
    pluginshared::FloatParam lpc_gain_attack{
        "lpc_attack", {10.0f, 100.0f, 1.0f, 0.4f},
         10.0f
    };
    pluginshared::FloatParam lpc_gain_hold{
        "lpc_hold", {1.0f, 100.0f, 1.0f, 0.4f},
         10.0f
    };
    pluginshared::FloatParam lpc_gain_release{
        "lpc_release", {5.0f, 200.0f, 1.0f, 0.4f},
         20.0f
    };
    pluginshared::FloatParam lpc_order{
        "lpc_order", {4.0f, 80.0f, 4.0f},
         36.0f
    };

    // stft
    pluginshared::FloatParam stft_bandwidth{
        "stft_bandwidth", {0.0f, 5.0f, 0.1f},
         2.0f
    };
    pluginshared::FloatParam mfcc_nbands{
        "mfcc_nbands", {8.0f, 80.0f, 4.0f},
         20.0f
    };
    pluginshared::FloatParam stft_release{
        "stft_release", {1.0f, 1000.0f, 1.0f, 0.4f},
         100.0f
    };
    pluginshared::FloatParam stft_attack{
        "stft_attack", {1.0f, 1000.0f, 1.0f, 0.4f},
         1.0f
    };
    pluginshared::FloatParam stft_blend{
        "stft_blend", {0.0f, 0.99f, 0.01f},
         0.6f
    };
    pluginshared::ChoiceParam stft_size{
        "stft_size", juce::StringArray{"256", "512", "1024", "2048", "4096"},
         "1024"
    };
    pluginshared::FloatParam stft_detail{
        "stft_detail", {0.01f, 1.0f, 0.01f},
         0.3f
    };
    pluginshared::ChoiceParam stft_type{
        "stft_type", juce::StringArray{"Standard", "Cepstrum", "MFCC", "Smooth", "Welch", "Morph"},
         "Cepstrum"
    };
    pluginshared::BoolParam stft_smooth_erb{
        "stft_smooth_erb", true // true = ERB，false = OCT
    };
    pluginshared::FloatParam stft_smooth{
        "stft_smooth", {0.1f, 2.0f, 0.05f},
         1.0f
    };
    pluginshared::FloatParam stft_welch{
        "stft_welch", {1.0f, 16.0f, 1.0f},
         4.0f
    };
    pluginshared::FloatParam stft_floor{
        "stft_floor", {-120.0f, -20.0f, 1.0f},
         -80.0f
    };
    pluginshared::FloatParam stft_morph{
        "stft_morph", {0.0f, 1.0f, 0.01f},
         0.5f
    };
    pluginshared::BoolParam stft_morph_ab{
        "stft_morph_ab", true // true = A→B，false = B→A
    };

    // pitch tracking
    pluginshared::FloatParam track_low{
        "track_low", {20.0f, 300.0f, 1.0f},
         80.0f
    };
    pluginshared::FloatParam track_high{
        "track_high", {300.0f, 800.0f, 1.0f},
         500.0f
    };
    pluginshared::FloatParam track_pitch{
        "track_pitch", {-36.0f, 36.0f, 1.0f},
         0.0f
    };
    pluginshared::FloatParam track_pwm{
        "track_pwm", {0.01f, 0.99f, 0.01f},
         0.5f
    };
    pluginshared::FloatParam track_noise{
        "track_noise", {0.0f, 1.0f, 0.01f},
         0.5f
    };
    pluginshared::ChoiceParam track_waveform{
        "track_waveform", juce::StringArray{"saw", "pwm"},
         "saw"
    };
    pluginshared::FloatParam track_glide{
        "track_glide", {1.0f, 1000.0f, 1.0f, 0.4f},
         1.0f
    };

    // 共享 DSP 控制状态（UI 线程写入，音频线程在 Engine::Update* 中读取/清除）
    std::atomic<bool> should_update_tilt_{};
    std::atomic<bool> should_update_leaky_lpc_{};
    std::atomic<bool> should_update_block_lpc_{};
    std::atomic<bool> should_update_stft_{};
    std::atomic<bool> should_update_channel_vocoder_{};
    std::atomic<bool> should_update_pitch_osc_{};

    void BuildLayout(juce::AudioProcessorValueTreeState::ParameterLayout& layout) {
        layout += pre_tilt;
        layout += channel_swap;
        layout += pitch_channel;
        layout += vocoder_type;
        layout += shift_pitch;
        layout += cv_filter_bank_mode;
        layout += cv_attack;
        layout += cv_gate;
        layout += cv_release;
        layout += cv_freq_begin;
        layout += cv_freq_end;
        layout += cv_nbands;
        layout += cv_scale;
        layout += cv_ripple;
        layout += cv_carry_scale;
        layout += cv_map;
        layout += lpc_forget;
        layout += lpc_smooth;
        layout += lpc_gain_attack;
        layout += lpc_gain_hold;
        layout += lpc_gain_release;
        layout += lpc_order;
        layout += stft_bandwidth;
        layout += mfcc_nbands;
        layout += stft_release;
        layout += stft_attack;
        layout += stft_blend;
        layout += stft_size;
        layout += stft_detail;
        layout += stft_type;
        layout += stft_smooth_erb;
        layout += stft_smooth;
        layout += stft_welch;
        layout += stft_floor;
        layout += stft_morph;
        layout += stft_morph_ab;
        layout += track_low;
        layout += track_high;
        layout += track_pitch;
        layout += track_pwm;
        layout += track_noise;
        layout += track_waveform;
        layout += track_glide;
    }

    void BeginListening() {
        pre_tilt.ptr_->addListener(&tilt_listener_);
        shift_pitch.ptr_->addListener(&formant_shift_listener_);
        cv_filter_bank_mode.ptr_->addListener(&channel_vocoder_listener_);
        cv_attack.ptr_->addListener(&channel_vocoder_listener_);
        cv_gate.ptr_->addListener(&channel_vocoder_listener_);
        cv_release.ptr_->addListener(&channel_vocoder_listener_);
        cv_freq_begin.ptr_->addListener(&channel_vocoder_listener_);
        cv_freq_end.ptr_->addListener(&channel_vocoder_listener_);
        cv_nbands.ptr_->addListener(&channel_vocoder_listener_);
        cv_scale.ptr_->addListener(&channel_vocoder_listener_);
        cv_ripple.ptr_->addListener(&channel_vocoder_listener_);
        cv_carry_scale.ptr_->addListener(&channel_vocoder_listener_);
        cv_map.ptr_->addListener(&channel_vocoder_listener_);
        lpc_forget.ptr_->addListener(&leaky_lpc_listener_);
        lpc_gain_hold.ptr_->addListener(&leaky_lpc_listener_);
        lpc_gain_release.ptr_->addListener(&leaky_lpc_listener_);
        lpc_smooth.ptr_->addListener(&leaky_lpc_block_listener_);
        lpc_gain_attack.ptr_->addListener(&leaky_lpc_block_listener_);
        lpc_order.ptr_->addListener(&leaky_lpc_block_listener_);
        stft_bandwidth.ptr_->addListener(&stft_listener_);
        mfcc_nbands.ptr_->addListener(&stft_listener_);
        stft_release.ptr_->addListener(&stft_listener_);
        stft_attack.ptr_->addListener(&stft_listener_);
        stft_blend.ptr_->addListener(&stft_listener_);
        stft_detail.ptr_->addListener(&stft_listener_);
        stft_type.ptr_->addListener(&stft_listener_);
        stft_smooth_erb.ptr_->addListener(&stft_listener_);
        stft_smooth.ptr_->addListener(&stft_listener_);
        stft_welch.ptr_->addListener(&stft_listener_);
        stft_floor.ptr_->addListener(&stft_listener_);
        stft_morph.ptr_->addListener(&stft_listener_);
        stft_morph_ab.ptr_->addListener(&stft_listener_);
        stft_size.ptr_->addListener(&stft_block_listener_);
        track_low.ptr_->addListener(&pitch_osc_listener_);
        track_high.ptr_->addListener(&pitch_osc_listener_);
        track_pitch.ptr_->addListener(&pitch_osc_listener_);
        track_pwm.ptr_->addListener(&pitch_osc_listener_);
        track_noise.ptr_->addListener(&pitch_osc_listener_);
        track_waveform.ptr_->addListener(&pitch_osc_listener_);
        track_glide.ptr_->addListener(&pitch_osc_listener_);
    }

    void EndListening() {
        pre_tilt.ptr_->removeListener(&tilt_listener_);
        shift_pitch.ptr_->removeListener(&formant_shift_listener_);
        cv_filter_bank_mode.ptr_->removeListener(&channel_vocoder_listener_);
        cv_attack.ptr_->removeListener(&channel_vocoder_listener_);
        cv_gate.ptr_->removeListener(&channel_vocoder_listener_);
        cv_release.ptr_->removeListener(&channel_vocoder_listener_);
        cv_freq_begin.ptr_->removeListener(&channel_vocoder_listener_);
        cv_freq_end.ptr_->removeListener(&channel_vocoder_listener_);
        cv_nbands.ptr_->removeListener(&channel_vocoder_listener_);
        cv_scale.ptr_->removeListener(&channel_vocoder_listener_);
        cv_ripple.ptr_->removeListener(&channel_vocoder_listener_);
        cv_carry_scale.ptr_->removeListener(&channel_vocoder_listener_);
        cv_map.ptr_->removeListener(&channel_vocoder_listener_);
        lpc_forget.ptr_->removeListener(&leaky_lpc_listener_);
        lpc_gain_hold.ptr_->removeListener(&leaky_lpc_listener_);
        lpc_gain_release.ptr_->removeListener(&leaky_lpc_listener_);
        lpc_smooth.ptr_->removeListener(&leaky_lpc_block_listener_);
        lpc_gain_attack.ptr_->removeListener(&leaky_lpc_block_listener_);
        lpc_order.ptr_->removeListener(&leaky_lpc_block_listener_);
        stft_bandwidth.ptr_->removeListener(&stft_listener_);
        mfcc_nbands.ptr_->removeListener(&stft_listener_);
        stft_release.ptr_->removeListener(&stft_listener_);
        stft_attack.ptr_->removeListener(&stft_listener_);
        stft_blend.ptr_->removeListener(&stft_listener_);
        stft_detail.ptr_->removeListener(&stft_listener_);
        stft_type.ptr_->removeListener(&stft_listener_);
        stft_smooth_erb.ptr_->removeListener(&stft_listener_);
        stft_smooth.ptr_->removeListener(&stft_listener_);
        stft_welch.ptr_->removeListener(&stft_listener_);
        stft_floor.ptr_->removeListener(&stft_listener_);
        stft_morph.ptr_->removeListener(&stft_listener_);
        stft_morph_ab.ptr_->removeListener(&stft_listener_);
        stft_size.ptr_->removeListener(&stft_block_listener_);
        track_low.ptr_->removeListener(&pitch_osc_listener_);
        track_high.ptr_->removeListener(&pitch_osc_listener_);
        track_pitch.ptr_->removeListener(&pitch_osc_listener_);
        track_pwm.ptr_->removeListener(&pitch_osc_listener_);
        track_noise.ptr_->removeListener(&pitch_osc_listener_);
        track_waveform.ptr_->removeListener(&pitch_osc_listener_);
        track_glide.ptr_->removeListener(&pitch_osc_listener_);
    }

private:
    // 各模块参数变化监听（引用上面的共享 atomic bool）
    TiltFlagListener tilt_listener_{should_update_tilt_};
    LeakyLpcFlagListener leaky_lpc_listener_{should_update_leaky_lpc_};
    LeakyLpcFlagListener leaky_lpc_block_listener_{should_update_leaky_lpc_, &should_update_block_lpc_};
    StftFlagListener stft_listener_{should_update_stft_};
    StftFlagListener stft_block_listener_{should_update_stft_, &should_update_block_lpc_};
    ChannelVocoderFlagListener channel_vocoder_listener_{should_update_channel_vocoder_};
    PitchOscFlagListener pitch_osc_listener_{should_update_pitch_osc_};
    FormantShiftFlagListener formant_shift_listener_{should_update_leaky_lpc_, should_update_block_lpc_,
                                                     should_update_stft_, should_update_channel_vocoder_};
};

} // namespace green_vocoder
