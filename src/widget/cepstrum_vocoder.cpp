#include "cepstrum_vocoder.hpp"
#include "../PluginProcessor.h"
#include "../param_ids.hpp"

namespace widget {

CepstrumVocoderUI::CepstrumVocoderUI(AudioPluginAudioProcessor& processor)
    : processor_(processor) {
    auto& apvts = *processor.value_tree_;

    title_.setText("Cepstrum Vocoder", juce::dontSendNotification);
    addAndMakeVisible(title_);

    omega_.BindParameter(apvts, id::kCepstrumOmega);
    omega_.SetShortName("OMEGA");
    addAndMakeVisible(omega_);

    release_.BindParameter(apvts, id::kStftRelease);
    release_.SetShortName("REL");
    addAndMakeVisible(release_);
}

void CepstrumVocoderUI::resized() {
    auto b = getLocalBounds();
    title_.setBounds(b.removeFromTop(20));
    auto top = b.removeFromTop(100);
    omega_.setBounds(top.removeFromLeft(50));
    release_.setBounds(top.removeFromLeft(50));
}

void CepstrumVocoderUI::paint(juce::Graphics& g) {
    auto bb = getLocalBounds();
    bb.removeFromTop(omega_.getBottom());

    g.setColour(juce::Colours::black);
    g.fillRect(bb);

    auto b = bb.toFloat();
    auto gains = processor_.cepstrum_vocoder_.gains_;
    juce::Point<float> line_last{ b.getX(), b.getCentreY() };
    g.setColour(juce::Colours::green);
    float mul_val = std::pow(10.0f, 2.0f / b.getWidth());
    float mul_begin = 1.0f;
    float omega_base = 180.0f * 2.0f / static_cast<float>(processor_.getSampleRate());
    for (int x = 0; x < bb.getWidth(); ++x) {
        float omega = omega_base * mul_begin;
        mul_begin *= mul_val;
        
        int idx = static_cast<int>(omega * gains.size());
        idx = std::min<int>(idx, gains.size() - 1);
        float db_gain = gains[idx];
        float y_nor = (db_gain - (-96.0f)) / (10.0f - (-96.0f));
        float y = b.getBottom() - y_nor * b.getHeight();
        juce::Point line_end{ static_cast<float>(x + b.toFloat().getX()), y };
        g.drawLine(juce::Line<float>{line_last, line_end}, 2.0f);
        line_last = line_end;
    }

    g.setColour(juce::Colours::white);
    g.drawRect(bb);
}

void CepstrumVocoderUI::timerCallback() {
    repaint(getLocalBounds().removeFromTop(omega_.getBottom()));
}

}
