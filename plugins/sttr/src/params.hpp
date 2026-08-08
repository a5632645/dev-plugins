#pragma once
#include <atomic>
#include <cmath>

#include "dsp/SttrProcessor.hpp"
#include "pluginshared/wrap_parameters.hpp"

class Params : public juce::AudioProcessorParameter::Listener {
public:
    pluginshared::FloatParam mix{
        "Mix", {0.0f, 1.0f, 0.01f},
         0.5f
    };
    pluginshared::FloatParam hop_ms{
        "HopMs", {0.0f, 500.0f, 0.01f, 0.3f},
         4.0f
    };
    pluginshared::FloatParam dry_delay{
        "DryDelay", {0.0f, 1.0f, 0.01f},
         0.5f
    };
    pluginshared::IntParam mul{
        "nGrains", 1, 4,
         2
    };
    pluginshared::FloatParam beta{
        "Harmonic", {2.0f, 16.0f, 0.01f},
         8.0f
    };
    pluginshared::FloatParam formant{
        "Formant", {-10.0f, 10.0f, 0.1f},
         0.0f
    };

    void BuildLayout(juce::AudioProcessorValueTreeState::ParameterLayout& layout) {
        layout += mix;
        layout += hop_ms;
        layout += dry_delay;
        layout += mul;
        layout += beta;
        layout += formant;
    }

    [[nodiscard]] SttrProcessor::Parameters ToSttrParam() {
        SttrProcessor::Parameters p;
        p.mix = mix.Get();
        p.hopMs = hop_ms.Get();
        p.dryDelay = dry_delay.Get();
        // stretch ratio derived from formant (semitones)
        p.stretch = std::exp2f(formant.Get() / 12.0f);
        p.windowMul = mul.Get();
        p.windowBeta = beta.Get();
        return p;
    }

    void BeginListening() {
        mix.ptr_->addListener(this);
        hop_ms.ptr_->addListener(this);
        dry_delay.ptr_->addListener(this);
        mul.ptr_->addListener(this);
        beta.ptr_->addListener(this);
        formant.ptr_->addListener(this);
    }

    void EndListening() {
        mix.ptr_->removeListener(this);
        hop_ms.ptr_->removeListener(this);
        dry_delay.ptr_->removeListener(this);
        mul.ptr_->removeListener(this);
        beta.ptr_->removeListener(this);
        formant.ptr_->removeListener(this);
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
