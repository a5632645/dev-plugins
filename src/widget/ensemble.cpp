#include "widget/ensemble.hpp"
#include "PluginProcessor.h"
#include "juce_events/juce_events.h"
#include "param_ids.hpp"
#include "tooltips.hpp"

namespace widget {

Ensemble::Ensemble(AudioPluginAudioProcessor& p) {
    auto& apvts = *p.value_tree_;

    addAndMakeVisible(label_);

    num_voice_.BindParameter(apvts, id::kEnsembleNumVoices);
    addAndMakeVisible(num_voice_);

    mix_.BindParameter(apvts, id::kEnsembleMix);
    addAndMakeVisible(mix_);

    spread_.BindParameter(apvts, id::kEnsembleSpread);
    addAndMakeVisible(spread_);

    detune_.BindParameter(apvts, id::kEnsembleDetune);
    addAndMakeVisible(detune_);

    rate_.BindParameter(apvts, id::kEnsembleRate);
    addAndMakeVisible(rate_);

    mode_.BindParam(apvts, id::kEnsembleMode);
    addAndMakeVisible(mode_);
}

void Ensemble::OnLanguageChanged(tooltip::Tooltips& tooltips) {
    label_.setText(tooltips.Label(id::kEnsembleTitle), juce::dontSendNotification);
    num_voice_.OnLanguageChanged(tooltips);
    detune_.OnLanguageChanged(tooltips);
    spread_.OnLanguageChanged(tooltips);
    mix_.OnLanguageChanged(tooltips);
    rate_.OnLanguageChanged(tooltips);
    mode_.OnLanguageChanged(tooltips);
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