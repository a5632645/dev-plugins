#pragma once
#include "dsp/idsp.hpp"
#include "pluginshared/wrap_parameters.hpp"

class Params {
public:
    pluginshared::FloatParam chorus_amount{
        "chorus amount", {0.0f, 1.0f, 0.01f},
         0.05f
    };
    pluginshared::FloatParam chorus_freq{
        "chorus freq", {0.003f, 8.0f, 0.001f, 0.4f},
         0.25f
    };
    pluginshared::FloatParam wet{
        "mix", {0.0f, 1.0f, 0.01f},
         0.25f
    };
    pluginshared::FloatParam pre_lowpass{
        "pre lowpass", {0.0f, 130.0f, 0.01f},
         0.0f
    };
    pluginshared::FloatParam pre_highpass{
        "pre highpass", {0.0f, 130.0f, 0.01f},
         110.0f
    };
    pluginshared::FloatParam low_damp{
        "low damp", {0.0f, 130.0f, 0.01f},
         0.0f
    };
    pluginshared::FloatParam high_damp{
        "high damp", {0.0f, 130.0f, 0.01f},
         90.0f
    };
    pluginshared::FloatParam low_gain{
        "low gain", {-6.0f, 0.0f, 0.01f},
         0.0f
    };
    pluginshared::FloatParam high_gain{
        "high gain", {-6.0f, 0.0f, 0.01f},
         -1.0f
    };
    pluginshared::FloatParam size{
        "size", {0.0f, 1.0f, 0.01f},
         0.5f
    };
    pluginshared::FloatParam decay{
        "decay", {15.0f, 64000.0f, 1.0f, 0.4f},
         1000.0f
    };
    pluginshared::FloatParam predelay{
        "predelay", {0.0f, 300.0f, 1.0f, 0.4f},
         0.0f
    };
    pluginshared::BoolParam freeze{"freeze", false};

    void BuildLayout(juce::AudioProcessorValueTreeState::ParameterLayout& layout) {
        layout += chorus_amount;
        layout += chorus_freq;
        layout += wet;
        layout += pre_lowpass;
        layout += pre_highpass;
        layout += low_damp;
        layout += high_damp;
        layout += low_gain;
        layout += high_gain;
        layout += size;
        layout += decay;
        layout += predelay;
        layout += freeze;
    }

    [[nodiscard]] vital_reverb::Param ToDspParam() {
        vital_reverb::Param p;
        p.chorus_amount = chorus_amount.Get();
        p.chorus_freq = chorus_freq.Get();
        p.wet = wet.Get();
        p.pre_lowpass = pre_lowpass.Get();
        p.pre_highpass = pre_highpass.Get();
        p.low_damp_pitch = low_damp.Get();
        p.high_damp_pitch = high_damp.Get();
        p.low_damp_db = low_gain.Get();
        p.high_damp_db = high_gain.Get();
        p.size = size.Get();
        p.decay_ms = decay.Get();
        p.pre_delay = predelay.Get();
        p.freeze = freeze.Get();
        return p;
    }
};
