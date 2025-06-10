#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "param_ids.hpp"
#include <complex>
#include <numbers>

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    auto& apvts = *p.value_tree_;

    addAndMakeVisible(filter_);
    hp_pitch_.BindParameter(apvts, id::kHighpassPitch);
    hp_pitch_.SetShortName("HPASS");
    addAndMakeVisible(hp_pitch_);
    em_pitch_.BindParameter(apvts, id::kEmphasisPitch);
    em_pitch_.SetShortName("HSHELF");
    addAndMakeVisible(em_pitch_);
    em_gain_.BindParameter(apvts, id::kEmphasisGain);
    em_gain_.SetShortName("GAIN");
    addAndMakeVisible(em_gain_);
    em_s_.BindParameter(apvts, id::kEmphasisS);
    em_s_.SetShortName("S");
    addAndMakeVisible(em_s_);

    addAndMakeVisible(shifter_);
    shift_pitch_.BindParameter(apvts, id::kShiftPitch);
    shift_pitch_.SetShortName("PITCH");
    addAndMakeVisible(shift_pitch_);

    addAndMakeVisible(lpc_);
    lpc_learn_.BindParameter(apvts, id::kLearnRate);
    lpc_learn_.SetShortName("LEARN");
    addAndMakeVisible(lpc_learn_);
    lpc_foorget_.BindParameter(apvts, id::kForgetRate);
    lpc_foorget_.SetShortName("FORGET");
    addAndMakeVisible(lpc_foorget_);
    lpc_smooth_.BindParameter(apvts, id::kLPCSmooth);
    lpc_smooth_.SetShortName("SMOOTH");
    addAndMakeVisible(lpc_smooth_);
    lpc_order_.BindParameter(apvts, id::kLPCOrder);
    lpc_order_.SetShortName("ORDER");
    addAndMakeVisible(lpc_order_);
    lpc_bend_.BindParameter(apvts, id::kLPCBend);
    lpc_bend_.SetShortName("BEND");
    addAndMakeVisible(lpc_bend_);

    setSize (500, 500);
    startTimerHz(30);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor() {
    stopTimer();
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g) {
    g.fillAll(juce::Colours::grey);
    auto b = getLocalBounds();
    b.removeFromTop(lpc_order_.getBottom());

    g.setColour(juce::Colours::black);
    g.fillRect(b);
    g.setColour(juce::Colours::white);
    g.drawRect(b);
    
    g.setColour(juce::Colours::green);
    int w = getWidth();
    float a = lpc_bend_.slider_.getValue();
    auto conv_y = [b](float y) {
        auto center = b.toFloat().getCentreY();
        auto high = b.toFloat().getHeight() / 2.0f;
        return center - y * high;
    };
    float beginx = 0.0f;
    float beginy = conv_y(0.0f);
    for (int i = 0; i < w; ++i) {
        float omega = static_cast<float>(i) * std::numbers::pi_v<float> / static_cast<float>(w - 1);
        auto z = std::polar(1.0f, -omega);
        auto up = a + z;
        auto down = 1.0f + a * z;
        auto res = up / down;
        auto phase = std::arg(res) / std::numbers::pi_v<float>;
        auto y = conv_y(phase);
        g.drawLine(beginx, beginy, i, y);
        beginx = i;
        beginy = y;
    }
}

void AudioPluginAudioProcessorEditor::resized() {
    auto b = getLocalBounds();
    {
        filter_.setBounds(b.removeFromTop(20));
        auto top = b.removeFromTop(100);
        hp_pitch_.setBounds(top.removeFromLeft(50));
        em_pitch_.setBounds(top.removeFromLeft(50));
        em_gain_.setBounds(top.removeFromLeft(50));
        em_s_.setBounds(top.removeFromLeft(50));
    }
    {
        shifter_.setBounds(b.removeFromTop(20));
        auto top = b.removeFromTop(100);
        shift_pitch_.setBounds(top.removeFromLeft(50));
    }
    {
        lpc_.setBounds(b.removeFromTop(20));
        auto top = b.removeFromTop(100);
        lpc_learn_.setBounds(top.removeFromLeft(50));
        lpc_foorget_.setBounds(top.removeFromLeft(50));
        lpc_smooth_.setBounds(top.removeFromLeft(50));
        lpc_order_.setBounds(top.removeFromLeft(50));
        lpc_bend_.setBounds(top.removeFromLeft(50));
    }
}

void AudioPluginAudioProcessorEditor::timerCallback() {
    repaint();
}