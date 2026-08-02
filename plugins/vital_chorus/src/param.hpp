#pragma once
#include <atomic>
#include <numbers>
#include "dsp/idsp.hpp"
#include "pluginshared/bpm_sync_lfo.hpp"
#include "pluginshared/wrap_parameters.hpp"
#include "qwqdsp/convert.hpp"
#include "global.hpp"

namespace vital_chorus {

class Params : public juce::AudioProcessorParameter::Listener {
public:
    pluginshared::FloatParam depth{
        "depth", {0.0f, 1.0f, 0.01f},
         0.5f
    };
    pluginshared::FloatParam delay1{
        "delay1", {0.976f, global::kMaxStaticDelayMs, 0.01f},
         1.953f
    };
    pluginshared::FloatParam delay2{
        "delay2", {0.976f, global::kMaxStaticDelayMs, 0.01f},
         7.812f
    };
    pluginshared::FloatParam feedback{
        "feedback", {-0.95f, 0.95f, 0.01f},
         0.4f
    };
    pluginshared::FloatParam mix{
        "mix", {0.0f, 1.0f, 0.01f},
         0.5f
    };
    pluginshared::FloatParam cutoff{
        "cutoff", {8.0f, 136.0f, 0.01f},
         60.0f
    };
    pluginshared::FloatParam spread{
        "spread", {0.0f, 1.0f, 0.01f},
         1.0f
    };
    pluginshared::FloatParam num_voices{
        "num_voices", {4.0f, static_cast<float>(global::kMaxNumChorus), 4.0f},
         16.0f
    };
    pluginshared::BpmSyncLFO freq{"freq", 0.0f, 8.0f, 0.01f, 1.0f, false, "0", "1/8", 0.0f, "4", false};

    void BuildLayout(juce::AudioProcessorValueTreeState::ParameterLayout& layout) {
        layout += depth;
        layout += delay1;
        layout += delay2;
        layout += feedback;
        layout += mix;
        layout += cutoff;
        layout += spread;
        layout += num_voices;
        freq += layout;
    }

    [[nodiscard]] vital_chorus::DspParam ToDspParam(float fs, juce::AudioPlayHead* ph) {
        vital_chorus::DspParam p;
        p.depth = depth.Get();
        p.delay1 = delay1.Get();
        p.delay2 = delay2.Get();
        p.feedback = feedback.Get();
        p.mix = mix.Get();
        p.num_voice = static_cast<int>(num_voices.Get());
        p.freq = freq.GetFreqHz(ph);

        float const filter_radius = spread.Get() * 8.0f * 12.0f;
        float const low_freq = qwqdsp::convert::Pitch2Freq(cutoff.Get() + filter_radius);
        float const high_freq = qwqdsp::convert::Pitch2Freq(cutoff.Get() - filter_radius);
        p.low_w = low_freq * std::numbers::pi_v<float> * 2.0f / fs;
        p.high_w = high_freq * std::numbers::pi_v<float> * 2.0f / fs;
        return p;
    }

    void BeginListening() {
        depth.ptr_->addListener(this);
        delay1.ptr_->addListener(this);
        delay2.ptr_->addListener(this);
        feedback.ptr_->addListener(this);
        mix.ptr_->addListener(this);
        cutoff.ptr_->addListener(this);
        spread.ptr_->addListener(this);
        num_voices.ptr_->addListener(this);
    }

    void EndListening() {
        depth.ptr_->removeListener(this);
        delay1.ptr_->removeListener(this);
        delay2.ptr_->removeListener(this);
        feedback.ptr_->removeListener(this);
        mix.ptr_->removeListener(this);
        cutoff.ptr_->removeListener(this);
        spread.ptr_->removeListener(this);
        num_voices.ptr_->removeListener(this);
    }

    bool IsParamChanged() noexcept {
        return changed_.exchange(false, std::memory_order_acquire);
    }

    void MarkChanged() noexcept {
        changed_.store(true, std::memory_order_release);
    }
private:
    std::atomic<bool> changed_{false};

    void parameterValueChanged(int parameterIndex, float newValue) override {
        juce::ignoreUnused(parameterIndex, newValue);
        changed_.store(true, std::memory_order_release);
    }

    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {
        juce::ignoreUnused(parameterIndex, gestureIsStarting);
    }
};

} // namespace vital_chorus
