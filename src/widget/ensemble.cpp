#include "widget/ensemble.hpp"
#include "PluginProcessor.h"
#include "juce_events/juce_events.h"
#include "param_ids.hpp"
#include "tooltips.hpp"

namespace widget {

Ensemble::Ensemble(AudioPluginAudioProcessor& p) {
    auto& apvts = *p.value_tree_;

    label_.setText("ensemble", juce::dontSendNotification);
    addAndMakeVisible(label_);

    num_voice_.BindParameter(apvts, id::kEnsembleNumVoices);
    num_voice_.slider_.setTooltip(tooltip::kEnsembleNumVoices);
    num_voice_.SetShortName("NVOICE");
    addAndMakeVisible(num_voice_);

    mix_.BindParameter(apvts, id::kEnsembleMix);
    mix_.slider_.setTooltip(tooltip::kEnsembleMix);
    mix_.SetShortName("MIX");
    addAndMakeVisible(mix_);

    spread_.BindParameter(apvts, id::kEnsembleSpread);
    spread_.slider_.setTooltip(tooltip::kEnsembleSpread);
    spread_.SetShortName("SPREAD");
    addAndMakeVisible(spread_);

    detune_.BindParameter(apvts, id::kEnsembleDetune);
    detune_.slider_.setTooltip(tooltip::kEnsembleDetune);
    detune_.SetShortName("DETUNE");
    addAndMakeVisible(detune_);

    rate_.BindParameter(apvts, id::kEnsembleRate);
    rate_.slider_.setTooltip(tooltip::kEnsembleRate);
    rate_.SetShortName("RATE");
    addAndMakeVisible(rate_);

    mode_.BindParam(apvts, id::kEnsembleMode);
    mode_.SetShortName("MODE");
    addAndMakeVisible(mode_);
}

void Ensemble::resized() {
    auto b = getLocalBounds();
    {
        auto top = b.removeFromTop(20);
        label_.setBounds(top);
    }
    {
        num_voice_.setBounds(b.removeFromLeft(50));
        detune_.setBounds(b.removeFromLeft(50));
        rate_.setBounds(b.removeFromLeft(50));
        spread_.setBounds(b.removeFromLeft(50));
        mix_.setBounds(b.removeFromLeft(50));
        {
            auto box = b.removeFromLeft(150);
            mode_.setBounds(box.removeFromTop(30));
        }
    }
}

}