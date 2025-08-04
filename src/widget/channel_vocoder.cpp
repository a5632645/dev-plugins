#include "channel_vocoder.hpp"
#include "PluginProcessor.h"
#include "juce_events/juce_events.h"
#include "juce_graphics/juce_graphics.h"
#include "param_ids.hpp"
#include "tooltips.hpp"

namespace widget {

ChannelVocoder::ChannelVocoder(AudioPluginAudioProcessor& p)
    : vocoder_(p.channel_vocoder_) {
    auto& apvts = *p.value_tree_;

    addAndMakeVisible(label_);

    attack_.BindParameter(apvts, id::kChannelVocoderAttack);
    addAndMakeVisible(attack_);

    release_.BindParameter(apvts, id::kChannelVocoderRelease);
    addAndMakeVisible(release_);

    nbands_.BindParameter(apvts, id::kChannelVocoderNBands);
    addAndMakeVisible(nbands_);

    freq_begin_.BindParameter(apvts, id::kChannelVocoderFreqBegin);
    addAndMakeVisible(freq_begin_);

    freq_end_.BindParameter(apvts, id::kChannelVocoderFreqEnd);
    addAndMakeVisible(freq_end_);

    scale_.BindParameter(apvts, id::kChannelVocoderScale);
    addAndMakeVisible(scale_);

    carry_scale_.BindParameter(apvts, id::kChannelVocoderCarryScale);
    addAndMakeVisible(carry_scale_);

    tooltip::tooltips.AddListenerAndInvoke(this);
}

void ChannelVocoder::OnLanguageChanged(tooltip::Tooltips& strs) {
    label_.setText(strs.Label(id::combbox::kVocoderNameIds[3]), juce::dontSendNotification);
}

void ChannelVocoder::resized() {
    auto b = getLocalBounds();
    label_.setBounds(b.removeFromTop(20));
    auto top = b.removeFromTop(100);
    attack_.setBounds(top.removeFromLeft(50));
    release_.setBounds(top.removeFromLeft(50));
    nbands_.setBounds(top.removeFromLeft(50));
    freq_begin_.setBounds(top.removeFromLeft(50));
    freq_end_.setBounds(top.removeFromLeft(50));
    scale_.setBounds(top.removeFromLeft(50));
    carry_scale_.setBounds(top.removeFromLeft(50));
}

void ChannelVocoder::paint(juce::Graphics& g) {
    auto b = getLocalBounds();
    b.removeFromTop(scale_.getBottom());
    auto bb = b.toFloat();

    g.setColour(juce::Colours::black);
    g.fillRect(bb);
    g.setColour(juce::Colours::white);
    g.drawRect(bb);

    constexpr float up = -30.0f;
    constexpr float down = -60.0f;

    int nbands = vocoder_.GetNumBins();
    float width = bb.getWidth() / nbands;
    float x = bb.getX();
    for (int i = 0; i < nbands; ++i) {
        juce::Rectangle<float> rect{ x, bb.getY(), width * 0.5f, bb.getHeight() };
        float gain = vocoder_.GetBinPeak(i);

        float db_gain = 20.0f * std::log10(gain + 1e-10f);
        if (db_gain < down) db_gain = down;
        float y_nor = (db_gain - (down)) / (up - (down));

        auto bin = rect.removeFromBottom(y_nor * rect.getWidth());
        g.setColour(juce::Colours::green);
        g.fillRect(bin);

        x += width;
    }
}


}