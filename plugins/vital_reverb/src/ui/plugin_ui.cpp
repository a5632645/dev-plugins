#include "plugin_ui.hpp"

#include "../PluginEditor.h"
#include "../PluginProcessor.h"

PluginUi::PluginUi(EmptyAudioProcessor& p)
    : preset_(*p.preset_manager_) {
    preset_.SetDspInstName(p.dsp_->InstName().data());
    addAndMakeVisible(preset_);

    chorus_amount_.BindParam(p.params_.chorus_amount.ptr_);
    addAndMakeVisible(chorus_amount_);
    chorus_freq_.BindParam(p.params_.chorus_freq.ptr_);
    addAndMakeVisible(chorus_freq_);
    mix_.BindParam(p.params_.wet.ptr_);
    addAndMakeVisible(mix_);
    pre_lowpass_.BindParam(p.params_.pre_lowpass.ptr_);
    addAndMakeVisible(pre_lowpass_);
    pre_highpass_.BindParam(p.params_.pre_highpass.ptr_);
    addAndMakeVisible(pre_highpass_);
    low_damp_.BindParam(p.params_.low_damp.ptr_);
    addAndMakeVisible(low_damp_);
    high_damp_.BindParam(p.params_.high_damp.ptr_);
    addAndMakeVisible(high_damp_);
    low_gain_.BindParam(p.params_.low_gain.ptr_);
    addAndMakeVisible(low_gain_);
    high_gain_.BindParam(p.params_.high_gain.ptr_);
    addAndMakeVisible(high_gain_);
    size_.BindParam(p.params_.size.ptr_);
    addAndMakeVisible(size_);
    decay_.BindParam(p.params_.decay.ptr_);
    addAndMakeVisible(decay_);
    predelay_.BindParam(p.params_.predelay.ptr_);
    addAndMakeVisible(predelay_);
    freeze_.BindParam(p.params_.freeze.ptr_);
    addAndMakeVisible(freeze_);

    panic_.onClick = [&p] { p.panic_flag_ = true; };
    addAndMakeVisible(panic_);

    setSize(500, 230);
}

void PluginUi::resized() {
    auto b = getLocalBounds();
    preset_.setBounds(b.removeFromTop(std::max(30, b.proportionOfHeight(0.1f))));

    auto top = b.removeFromTop(30);
    freeze_.setBounds(top.removeFromLeft(80).reduced(2));
    panic_.setBounds(top.removeFromLeft(70).reduced(2));

    int w = b.getWidth() / 6;
    top = b.removeFromTop(b.getHeight() / 2);
    pre_lowpass_.setBounds(top.removeFromLeft(w));
    low_damp_.setBounds(top.removeFromLeft(w));
    high_damp_.setBounds(top.removeFromLeft(w));
    chorus_amount_.setBounds(top.removeFromLeft(w));
    predelay_.setBounds(top.removeFromLeft(w));
    mix_.setBounds(top.removeFromLeft(w));
    auto bottom = b;
    pre_highpass_.setBounds(bottom.removeFromLeft(w));
    low_gain_.setBounds(bottom.removeFromLeft(w));
    high_gain_.setBounds(bottom.removeFromLeft(w));
    chorus_freq_.setBounds(bottom.removeFromLeft(w));
    size_.setBounds(bottom.removeFromLeft(w));
    decay_.setBounds(bottom.removeFromLeft(w));
}

void PluginUi::paint(juce::Graphics& g) {
    (void)g;
}

void PluginUi::TrySetSize(int width, int height) {
    if (auto p = findParentComponentOfClass<EmptyAudioProcessorEditor>(); p != nullptr) {
        p->SetNewSize(width, height);
    }
}
