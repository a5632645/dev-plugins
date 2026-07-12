#include "adsr.hpp"
#include "../dsp/synth.hpp"

namespace analogsynth {
AdsrGui::AdsrGui(Synth& synth, size_t idx) {
    switch (idx) {
        case 1:
            attack_.BindParam(synth.param_env_volume_attack.ptr_);
            decay_.BindParam(synth.param_env_volume_decay.ptr_);
            sustain_.BindParam(synth.param_env_volume_sustain.ptr_);
            release_.BindParam(synth.param_env_volume_release.ptr_);
            exp_.BindParam(synth.param_env_volume_exp.ptr_);
            title_.setText("Vol Env", juce::dontSendNotification);
            break;
        case 2:
            attack_.BindParam(synth.param_env_mod_attack.ptr_);
            decay_.BindParam(synth.param_env_mod_decay.ptr_);
            sustain_.BindParam(synth.param_env_mod_sustain.ptr_);
            release_.BindParam(synth.param_env_mod_release.ptr_);
            exp_.BindParam(synth.param_env_mod_exp.ptr_);
            title_.setText("Mod Env", juce::dontSendNotification);
            break;
        default:
            jassertfalse;
            break;
    }
    ui::SetLableBlack(title_);
    title_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(title_);
    addAndMakeVisible(attack_);
    addAndMakeVisible(decay_);
    addAndMakeVisible(sustain_);
    addAndMakeVisible(release_);
    addAndMakeVisible(exp_);
}

void AdsrGui::paint(juce::Graphics& g) {
    auto content_bound = getLocalBounds().reduced(1);
    g.fillAll(ui::light_green_bg);
    g.setColour(ui::green_bg);
    g.fillRect(content_bound);
}

void AdsrGui::resized() {
    auto content_bound = getLocalBounds().reduced(1);
    auto const ref = content_bound;
    auto b = ref;

    auto top_bound = b.removeFromTop(25);
    exp_.setBounds(top_bound.removeFromRight(50).reduced(2));
    title_.setBounds(top_bound);

    attack_.setBounds(b.removeFromLeft(ref.proportionOfWidth(0.25f)));
    decay_.setBounds(b.removeFromLeft(ref.proportionOfWidth(0.25f)));
    sustain_.setBounds(b.removeFromLeft(ref.proportionOfWidth(0.25f)));
    release_.setBounds(b.removeFromLeft(ref.proportionOfWidth(0.25f)));
}
}