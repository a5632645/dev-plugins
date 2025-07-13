#include "stft_vocoder.hpp"
#include "../PluginProcessor.h"
#include "../param_ids.hpp"
#include "../tooltips.hpp"
#include "juce_events/juce_events.h"

namespace widget {

STFTVocoder::STFTVocoder(AudioPluginAudioProcessor& processor)
: processor_(processor) {
    auto& apvts = *processor.value_tree_;

    addAndMakeVisible(title_);

    bandwidth_.BindParameter(apvts, id::kStftWindowWidth);
    addAndMakeVisible(bandwidth_);

    attack_.BindParameter(apvts, id::kStftAttack);
    addAndMakeVisible(attack_);

    release_.BindParameter(apvts, id::kStftRelease);
    addAndMakeVisible(release_);

    blend_.BindParameter(apvts, id::kStftBlend);
    addAndMakeVisible(blend_);
    
    tooltip::tooltips.AddListenerAndInvoke(this);
}

void STFTVocoder::OnLanguageChanged(tooltip::Tooltips& tooltips) {
    title_.setText(tooltips.Label(id::combbox::kVocoderNameIds[2]), juce::dontSendNotification);
}

void STFTVocoder::resized() {
    auto b = getLocalBounds();
    title_.setBounds(b.removeFromTop(20));
    auto top = b.removeFromTop(100);
    bandwidth_.setBounds(top.removeFromLeft(50));
    attack_.setBounds(top.removeFromLeft(50));
    release_.setBounds(top.removeFromLeft(50));
    blend_.setBounds(top.removeFromLeft(50));
}

void STFTVocoder::paint(juce::Graphics& g) {
    auto bb = getLocalBounds();
    bb.removeFromTop(bandwidth_.getBottom());

    g.setColour(juce::Colours::black);
    g.fillRect(bb);

    auto b = bb.toFloat();
    auto gains = processor_.stft_vocoder_.gains_;
    juce::Point<float> line_last{ b.getX(), b.getCentreY() };
    g.setColour(juce::Colours::green);
    float mul_val = std::pow(10.0f, 2.0f / b.getWidth());
    float mul_begin = 1.0f;
    float omega_base = 180.0f * 2.0f / static_cast<float>(processor_.getSampleRate());
    for (int x = 0; x < bb.getWidth(); ++x) {
        float omega = omega_base * mul_begin;
        mul_begin *= mul_val;
        
        int idx = static_cast<int>(omega * gains.size());
        idx = std::min<int>(idx, static_cast<int>(gains.size()) - 1);
        float gain = gains[idx];
        float db_gain = 20.0f * std::log10(gain + 1e-10f);
        float y_nor = (db_gain - (-100.0f)) / (20.0f - (-100.0f));
        float y = b.getBottom() - y_nor * b.getHeight();
        juce::Point line_end{ static_cast<float>(x + b.toFloat().getX()), y };
        g.drawLine(juce::Line<float>{line_last, line_end}, 2.0f);
        line_last = line_end;
    }

    g.setColour(juce::Colours::white);
    g.drawRect(bb);
}

void STFTVocoder::timerCallback() {
    repaint(getLocalBounds().removeFromTop(bandwidth_.getBottom()));
}

}
