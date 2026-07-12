#include "plugin_ui.hpp"

#include "../PluginProcessor.h"
#include "../PluginEditor.h"

PluginUi::PluginUi(EmptyAudioProcessor& p)
    : tooltip_(this, 500)
    , preset_(*p.preset_manager_) {
    addAndMakeVisible(preset_);
    preset_.SetDspInstName(p.dsp_processor_.name);

    auto& apvt = *p.value_tree_;
    warp_.BindParam(apvt, "warp");
    warp_.slider.setTooltip("Split spectrum to {this value} bands");
    addAndMakeVisible(warp_);

    f_low_.BindParam(apvt, "scale");
    f_low_.slider.setTooltip("Set the filter's bandwidth of each band.\nlower sounds metallic, higher sounds clicky(downsampled)");
    addAndMakeVisible(f_low_);

    f_high_.BindParam(apvt, "f_high");
    f_high_.slider.setTooltip("Set the warp spectrum ceil frequency.\nHigher frequency components will be silenced");
    addAndMakeVisible(f_high_);

    poles_.BindParam(apvt, "poles");
    poles_.slider.setTooltip("Set the filter's poles");
    addAndMakeVisible(poles_);

    drywet_.BindParam(apvt, "drywet");
    addAndMakeVisible(drywet_);

    pitch_.BindParam(apvt, "pitch");
    pitch_.slider.setTooltip("Set the warped signal's pitch.\nThis is used to simulate PiWarp's pitch parameter");
    addAndMakeVisible(pitch_);

    pitch_affect_.BindParam(apvt, "pitch_affect");
    pitch_affect_.setTooltip("How the formant is changed");
    addAndMakeVisible(pitch_affect_);
    ui::SetLableBlack(pitch_affect_label);
    addAndMakeVisible(pitch_affect_label);

    fill_gap_.BindParam(apvt, "fill_gap");
    fill_gap_.setTooltip("scale the filter cutoff to fill gaps when changing pitch parameter.\n*Off: a comb filter effect\n*On: bands overlay cause harsh sound");
    addAndMakeVisible(fill_gap_);

    freq_mode_.BindParam(apvt, "freq_mode");
    freq_mode_.setTooltip("Set the frequency distribution.\n*voice mode is best for making robotic sounds.\n*music mode is best for making negative harmony sounds");
    addAndMakeVisible(freq_mode_);
    ui::SetLableBlack(freq_label_);
    addAndMakeVisible(freq_label_);

    setSize(480, 160);
    tooltip_.setLookAndFeel(ui::GetLookAndFeel());
}

PluginUi::~PluginUi() {
    tooltip_.setLookAndFeel(nullptr);
}

void PluginUi::resized() {
    auto b = getLocalBounds();
    preset_.setBounds(b.removeFromTop(30));

    auto line = b.removeFromBottom(100);
    warp_.setBounds(line.removeFromLeft(80));
    f_high_.setBounds(line.removeFromLeft(80));
    f_low_.setBounds(line.removeFromLeft(80));
    poles_.setBounds(line.removeFromLeft(80));
    pitch_.setBounds(line.removeFromLeft(80));
    drywet_.setBounds(line.removeFromLeft(80));

    fill_gap_.setBounds(b.removeFromRight(80).reduced(2));
    auto top_left_b = b.removeFromLeft(pitch_.getBounds().getX());

    pitch_affect_.setBounds(b.withWidth(pitch_.getWidth()).reduced(2));
    pitch_affect_label.setBounds(pitch_affect_.getBounds().translated(-100, 0).withWidth(100));

    freq_label_.setBounds(top_left_b.removeFromLeft(80));
    freq_mode_.setBounds(top_left_b.removeFromLeft(120).reduced(2));
}

void PluginUi::paint(juce::Graphics& g) {
    juce::ignoreUnused(g);
}

void PluginUi::TrySetSize(int width, int height) {
    if (auto p = findParentComponentOfClass<EmptyAudioProcessorEditor>(); p != nullptr) {
        p->SetNewSize(width, height);
    }
}
