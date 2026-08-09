#include "plugin_ui.hpp"

#include "../PluginProcessor.h"

PluginUi::PluginUi(SttrAudioProcessor& p)
    : tooltip_(this, 500)
    , preset_(*p.preset_manager_) {
    preset_.SetDspInstName(p.dsp_->InstName().data());

    // dials
    stretch_dial_.BindParam(*p.value_tree_, "Formant");
    stretch_dial_.slider.setTooltip(
        "Formant shift in semitones.\nIn short HopMs sounds like formant shift.\nIn long HopMs sounds like pitch shift.");
    addAndMakeVisible(stretch_dial_);

    grain_dial_.BindParam(*p.value_tree_, "HopMs");
    grain_dial_.slider.setTooltip("Grain hop length in milliseconds.\nSmaller hop = audioable harmonics.\nBigger hop = time reverse.");
    addAndMakeVisible(grain_dial_);

    dry_delay_dial_.BindParam(*p.value_tree_, "DryDelay");
    dry_delay_dial_.slider.setTooltip("Dry signal read offset, as a fraction of the grain length");
    addAndMakeVisible(dry_delay_dial_);

    mix_dial_.BindParam(*p.value_tree_, "Mix");
    mix_dial_.slider.setTooltip("Dry/wet balance between the delayed dry signal and the granulated wet signal");
    addAndMakeVisible(mix_dial_);

    // Kaiser window parameters
    mul_dial_.BindParam(*p.value_tree_, "nGrains");
    mul_dial_.slider.setTooltip(
        "Grain length multiplier (window length = nGrains * hop).\nAlso sets the number of active grains");
    addAndMakeVisible(mul_dial_);

    beta_dial_.BindParam(*p.value_tree_, "Harmonic");
    beta_dial_.slider.setTooltip("Kaiser window beta.\nHigher beta gives harmonics that focus on original postition.\nLower beta gives discontinuous artifact.");
    addAndMakeVisible(beta_dial_);

    // grain playback direction (rev = time-reversed, fwd = forward)
    reverse_switch_.BindParam(*p.value_tree_, "reverse");
    reverse_switch_.setTooltip(
        "Grain playback direction.\nReverse: time-reversed grains\nForward: sequential grain playback");
    addAndMakeVisible(reverse_switch_);

    addAndMakeVisible(preset_);
    setSize(kWidth, kHeight);

    tooltip_.setLookAndFeel(ui::GetLookAndFeel());
}

PluginUi::~PluginUi() {
    tooltip_.setLookAndFeel(nullptr);
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
