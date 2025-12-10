#include "ensemble.hpp"
#include "PluginProcessor.h"
#include "param_ids.hpp"

namespace green_vocoder::widget {

Ensemble::Ensemble(AudioPluginAudioProcessor& p) {
    auto& apvts = *p.value_tree_;

    addAndMakeVisible(title_);

    num_voice_.BindParam(apvts, id::kEnsembleNumVoices);
    addAndMakeVisible(num_voice_);

    mix_.BindParam(apvts, id::kEnsembleMix);
    addAndMakeVisible(mix_);

    spread_.BindParam(apvts, id::kEnsembleSpread);
    addAndMakeVisible(spread_);

    detune_.BindParam(apvts, id::kEnsembleDetune);
    addAndMakeVisible(detune_);

    rate_.BindParam(apvts, id::kEnsembleRate);
    addAndMakeVisible(rate_);

    mode_.BindParam(apvts, id::kEnsembleMode);
    addAndMakeVisible(mode_);
}

void Ensemble::resized() {
    auto b = getLocalBounds();
    title_.setBounds(b.removeFromTop(20));

    auto block = b.removeFromLeft(70);
    mode_.setBounds(block.removeFromBottom(25));
    num_voice_.setBounds(block);

    auto dials = b.removeFromTop(65);
    detune_.setBounds(dials.removeFromLeft(50));
    rate_.setBounds(dials.removeFromLeft(50));
    spread_.setBounds(dials.removeFromLeft(50));
    mix_.setBounds(dials.removeFromLeft(50));
}

}
