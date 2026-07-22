#pragma once
#include "dsp/idsp.hpp"
#include "global.hpp"
#include "pluginshared/wrap_parameters.hpp"

class Params : public juce::AudioProcessorParameter::Listener {
public:
    pluginshared::IntParam warp{"warp", 1, global::kMaxBands, 50};
    pluginshared::FloatParam f_high{
        "f_high", {4000.0f, 20010.0f, 0.4f},
         20010.0f
    };
    pluginshared::FloatParam scale{
        "scale", {0.1f, 3.0f, 0.01f},
         1.0f
    };
    pluginshared::FloatParam pitch{
        "pitch", {-24.0f, 24.0f, 0.01f},
         0.0f
    };
    pluginshared::BoolParam pitch_affect{"pitch_affect", true};
    pluginshared::BoolParam fill_gap{"fill_gap", false};
    pluginshared::FloatParam drywet{
        "drywet", {0.0f, 1.0f, 0.01f},
         1.0f
    };
    pluginshared::IntParam poles{"poles", 1, global::kMaxPoles, 2};
    pluginshared::ChoiceParam freq_mode{
        "freq_mode",
        juce::StringArray{
                          "voice: 0 + n", "voice: 1 + n",
                          "music: 0 + 2n", "music: 1 + 2n",
                          },
        "music: 0 + 2n"
    };

    void BuildLayout(juce::AudioProcessorValueTreeState::ParameterLayout& layout) {
        layout += warp;
        layout += f_high;
        layout += scale;
        layout += pitch;
        layout += pitch_affect;
        layout += fill_gap;
        layout += drywet;
        layout += poles;
        layout += freq_mode;
    }

    [[nodiscard]] warpcore::Param ToWarpcoreParam() {
        warpcore::Param p;
        p.bands = warp.Get();
        p.f_high = f_high.Get();
        p.filter_scale = scale.Get();
        p.filter_order = poles.Get();
        p.pitch_shift = pitch.Get();
        p.drywet = drywet.Get();
        p.pitch_affect = pitch_affect.Get();
        p.fill_gap = fill_gap.Get();
        p.freq_distribution = static_cast<warpcore::FreqDistrbution>(freq_mode.Get());
        return p;
    }

    void BeginListening() {
        warp.ptr_->addListener(this);
        f_high.ptr_->addListener(this);
        scale.ptr_->addListener(this);
        pitch.ptr_->addListener(this);
        pitch_affect.ptr_->addListener(this);
        fill_gap.ptr_->addListener(this);
        drywet.ptr_->addListener(this);
        poles.ptr_->addListener(this);
        freq_mode.ptr_->addListener(this);
    }

    void EndListening() {
        warp.ptr_->removeListener(this);
        f_high.ptr_->removeListener(this);
        scale.ptr_->removeListener(this);
        pitch.ptr_->removeListener(this);
        pitch_affect.ptr_->removeListener(this);
        fill_gap.ptr_->removeListener(this);
        drywet.ptr_->removeListener(this);
        poles.ptr_->removeListener(this);
        freq_mode.ptr_->removeListener(this);
    }

    bool IsParamChanged() noexcept {
        return changed_.exchange(false, std::memory_order_acquire);
    }

    void MarkChanged() noexcept {
        changed_.store(true, std::memory_order_release);
    }

    std::atomic<bool> changed_{false};
private:
    void parameterValueChanged(int parameterIndex, float newValue) override {
        juce::ignoreUnused(parameterIndex, newValue);
        changed_.store(true, std::memory_order_release);
    }

    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {
        juce::ignoreUnused(parameterIndex, gestureIsStarting);
    }
};
