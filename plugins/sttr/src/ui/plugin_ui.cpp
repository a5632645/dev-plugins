#include "plugin_ui.hpp"

#include "../PluginProcessor.h"

PluginUi::PluginUi(SttrAudioProcessor& p)
    : preset_(*p.preset_manager_) {
    // Bind dials to parameters
    stretchDial_.BindParam(*p.value_tree_, "Stretch");
    grainDial_.BindParam(*p.value_tree_, "HopMs");
    dryDelayDial_.BindParam(*p.value_tree_, "DryDelay");
    mixDial_.BindParam(*p.value_tree_, "Mix");

    addAndMakeVisible(stretchDial_);
    addAndMakeVisible(grainDial_);
    addAndMakeVisible(dryDelayDial_);
    addAndMakeVisible(mixDial_);

    // Window type combo
    windowTypeCombo_.BindParam(*p.value_tree_, "WindowType");
    addAndMakeVisible(windowTypeCombo_);

    windowLabel_.setText("Window", juce::dontSendNotification);
    windowLabel_.setJustificationType(juce::Justification::centred);
    windowLabel_.setColour(juce::Label::ColourIds::textColourId, ui::black_bg);
    addAndMakeVisible(windowLabel_);

    addAndMakeVisible(preset_);
    setSize(kWidth, kHeight);
}

void PluginUi::resized() {
    auto b = getLocalBounds();
    preset_.setBounds(b.removeFromTop(28));

    // Row 1: 4 dials centered
    static constexpr int dial_w = 70;
    static constexpr int dial_h = 76;
    int const dials_total = dial_w * 4;
    int const x_start = (kWidth - dials_total) / 2;

    stretchDial_.setBounds(x_start, b.getY(), dial_w, dial_h);
    grainDial_.setBounds(x_start + dial_w, b.getY(), dial_w, dial_h);
    dryDelayDial_.setBounds(x_start + dial_w * 2, b.getY(), dial_w, dial_h);
    mixDial_.setBounds(x_start + dial_w * 3, b.getY(), dial_w, dial_h);

    // Row 2: "Window" label + combo left-aligned
    static constexpr int label_w = 70;
    static constexpr int combo_w = 140;
    int const row2_y = b.getY() + dial_h + 10;

    windowLabel_.setBounds(0, row2_y + 2, label_w, 30);
    windowTypeCombo_.setBounds(label_w, row2_y, combo_w, 30);
}
