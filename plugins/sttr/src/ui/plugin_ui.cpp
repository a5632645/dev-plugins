#include "plugin_ui.hpp"

#include "../PluginProcessor.h"

PluginUi::PluginUi(SttrAudioProcessor& p)
    : preset_(*p.preset_manager_) {
    preset_.SetDspInstName(p.dsp_->InstName().data());

    // dials
    stretch_dial_.BindParam(*p.value_tree_, "Formant");
    grain_dial_.BindParam(*p.value_tree_, "HopMs");
    dry_delay_dial_.BindParam(*p.value_tree_, "DryDelay");
    mix_dial_.BindParam(*p.value_tree_, "Mix");

    addAndMakeVisible(stretch_dial_);
    addAndMakeVisible(grain_dial_);
    addAndMakeVisible(dry_delay_dial_);
    addAndMakeVisible(mix_dial_);

    // Kaiser window parameters
    mul_dial_.BindParam(*p.value_tree_, "nGrains");
    beta_dial_.BindParam(*p.value_tree_, "Harmonic");

    addAndMakeVisible(mul_dial_);
    addAndMakeVisible(beta_dial_);

    // grain playback direction (rev = time-reversed, fwd = forward)
    reverse_switch_.BindParam(*p.value_tree_, "reverse");
    addAndMakeVisible(reverse_switch_);

    addAndMakeVisible(preset_);
    setSize(kWidth, kHeight);
}

void PluginUi::resized() {
    auto b = getLocalBounds();
    preset_.setBounds(b.removeFromTop(28));

    // row 1: 4 dials centred
    static constexpr int dial_w = 70;
    static constexpr int dial_h = 76;
    int const dials_total = dial_w * 4;
    int const x_start = (kWidth - dials_total) / 2;

    stretch_dial_.setBounds(x_start, b.getY(), dial_w, dial_h);
    grain_dial_.setBounds(x_start + dial_w, b.getY(), dial_w, dial_h);
    dry_delay_dial_.setBounds(x_start + dial_w * 2, b.getY(), dial_w, dial_h);
    mix_dial_.setBounds(x_start + dial_w * 3, b.getY(), dial_w, dial_h);

    // row 2: playback direction switch on the left, Kaiser window dials centred
    int const row2_y = b.getY() + dial_h + 10;
    int const row2_total = dial_w * 2;
    int const row2_x = (kWidth - row2_total) / 2;

    static constexpr int sw_w = 70;
    static constexpr int sw_h = 30;
    reverse_switch_.setBounds(row2_x - sw_w, row2_y + (dial_h - sw_h) / 2, sw_w, sw_h);

    mul_dial_.setBounds(row2_x, row2_y, dial_w, dial_h);
    beta_dial_.setBounds(row2_x + dial_w, row2_y, dial_w, dial_h);
}
