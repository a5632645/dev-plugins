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

    // Kaiser window parameters
    mulDial_.BindParam(*p.value_tree_, "nGrains");
    betaDial_.BindParam(*p.value_tree_, "Harmonic");

    addAndMakeVisible(mulDial_);
    addAndMakeVisible(betaDial_);

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

    // Row 2: Kaiser window dials centered
    int const row2_y = b.getY() + dial_h + 10;
    int const row2_total = dial_w * 2;
    int const row2_x = (kWidth - row2_total) / 2;

    mulDial_.setBounds(row2_x, row2_y, dial_w, dial_h);
    betaDial_.setBounds(row2_x + dial_w, row2_y, dial_w, dial_h);
}
