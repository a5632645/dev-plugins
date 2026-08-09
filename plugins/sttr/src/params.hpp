#pragma once
#include <atomic>
#include <cmath>

#include "dsp/idsp.hpp"
#include "global.hpp"
#include "pluginshared/wrap_parameters.hpp"

class Params : public juce::AudioProcessorParameter::Listener {
public:
    pluginshared::FloatParam mix{
        "Mix", {0.0f, 1.0f, 0.01f},
         0.5f
    };
    pluginshared::FloatParam hop_ms{
        "HopMs", {0.0f, global::kMaxHopMs, 0.01f, 0.3f},
         4.0f
    };
    pluginshared::FloatParam dry_delay{
        "DryDelay", {0.0f, 1.0f, 0.01f},
         0.5f
    };
    pluginshared::IntParam mul{"nGrains", 1, global::kMaxGrains, 2};
    pluginshared::FloatParam beta{
        "Harmonic", {2.0f, 16.0f, 0.01f},
         8.0f
    };
    pluginshared::FloatParam formant{
        "Formant", {-global::kMaxFormantShift, global::kMaxFormantShift, 0.1f},
         0.0f
    };
    pluginshared::BoolParam reverse{"reverse", true};

    void BuildLayout(juce::AudioProcessorValueTreeState::ParameterLayout& layout) {
        layout += mix;
        layout += hop_ms;
        layout += dry_delay;
        layout += mul;
        layout += beta;
        layout += formant;
        layout += reverse;
    }

    [[nodiscard]] sttr::SttrParam ToSttrParam() {
        sttr::SttrParam p;
        p.mix = mix.Get();
        p.hop_ms = hop_ms.Get();
        p.dry_delay = dry_delay.Get();
        // stretch ratio = 2^(formant/12), formant in semitones
        p.stretch = std::exp2f(formant.Get() / 12.0f);
        p.window_mul = mul.Get();
        p.window_beta = beta.Get();
        p.reverse = reverse.Get();
        return p;
    }

    void BeginListening() {
        mix.ptr_->addListener(this);
        hop_ms.ptr_->addListener(this);
        dry_delay.ptr_->addListener(this);
        mul.ptr_->addListener(this);
        beta.ptr_->addListener(this);
        formant.ptr_->addListener(this);
        reverse.ptr_->addListener(this);
    }

    void EndListening() {
        mix.ptr_->removeListener(this);
        hop_ms.ptr_->removeListener(this);
        dry_delay.ptr_->removeListener(this);
        mul.ptr_->removeListener(this);
        beta.ptr_->removeListener(this);
        formant.ptr_->removeListener(this);
        reverse.ptr_->removeListener(this);
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
