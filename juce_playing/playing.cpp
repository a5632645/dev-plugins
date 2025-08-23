#include "playing.hpp"
#include "PluginProcessor.h"
#include "juce_audio_processors/juce_audio_processors.h"
#include "juce_core/juce_core.h"
#include "qwqdsp/convert.hpp"
#include <memory>

namespace dsp {
void Playing::GetParams(ParamListeners& l, juce::AudioProcessorValueTreeState::ParameterLayout& apvts) {
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"w0",1},
            "w0",
            juce::NormalisableRange<float>{0.0f, 20000.0f, 1.0f, 0.4f},
            0.0f
        );
        l.Add(p, [this](float v) {
            dsf_.SetW0(qwqdsp::convert::Freq2W(v, fs_));
            g_ = dsf_.NormalizeGain();
        });
        apvts.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"w",1},
            "w",
            juce::NormalisableRange<float>{0.0f, 20000.0f, 1.0f, 0.4f},
            0.0f
        );
        l.Add(p, [this](float v) {
            dsf_.SetWSpace(qwqdsp::convert::Freq2W(v, fs_));
            g_ = dsf_.NormalizeGain();
        });
        apvts.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"n",1},
            "n",
            juce::NormalisableRange<float>{1.0f, 512.0f, 1.0f, 0.4f},
            4.0f
        );
        l.Add(p, [this](float v) {
            dsf_.SetN(v);
            g_ = dsf_.NormalizeGain();
        });
        apvts.add(std::move(p));
    }
    // {
    //     auto p = std::make_unique<juce::AudioParameterFloat>(
    //         juce::ParameterID{"a",1},
    //         "a",
    //         juce::NormalisableRange<float>{0.0f, 2.0f},
    //         0.5f
    //     );
    //     l.Add(p, [this](float v) {
    //         dsf_.SetAmpFactor(v);
    //         g_ = dsf_.NormalizeGain();
    //     });
    //     apvts.add(std::move(p));
    // }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"g",1},
            "g",
            juce::NormalisableRange<float>{0.0f, 2.0f},
            0.5f
        );
        l.Add(p, [this](float v) {
            dsf_.SetAmpGain(v);
            g_ = dsf_.NormalizeGain();
        });
        apvts.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"p",1},
            "p",
            juce::NormalisableRange<float>{0.0f, 1.0f},
            0.0f
        );
        l.Add(p, [this](float v) {
            dsf_.SetAmpPhase(v * std::numbers::pi_v<float> * 2.0f);
            g_ = dsf_.NormalizeGain();
        });
        apvts.add(std::move(p));
    }
}
}