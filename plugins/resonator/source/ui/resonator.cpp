#include "resonator.hpp"
#include "../PluginProcessor.h"

static juce::String FormatMidiNote(int note) {
    if (note > 0) {
        return juce::MidiMessage::getMidiNoteName(note, true, true, false);
    }
    else {
        return "inactive";
    }
}

ResonatorBarGUI::ResonatorBarGUI(ResonatorAudioProcessor& p, size_t idx)
    : p_(p)
    , idx_(idx) {
    auto& apvts = *p.value_tree_;

    pitch_.BindParam(apvts, juce::String{"pitch"} + juce::String{idx});
    addAndMakeVisible(pitch_);
    ui::SetLableBlack(midi_pitch_);
    midi_pitch_.setJustificationType(juce::Justification::centred);
    addChildComponent(midi_pitch_);

    fine_.BindParam(apvts, juce::String{"fine"} + juce::String{idx});
    addAndMakeVisible(fine_);

    dispersion_.BindParam(apvts, juce::String{"dispersion"} + juce::String{idx});
    dispersion_.OnMenuShowup() = [single = this](juce::PopupMenu& m) {
        m.addSeparator();
        m.addItem("set all", [ptr = single](){
            float const v = ptr->p_.dispersion_pole_radius_[0]->convertTo0to1(static_cast<float>(ptr->dispersion_.slider.getValue()));
            for (auto& param : ptr->p_.dispersion_pole_radius_) {
                param->setValueNotifyingHost(v);
            }
        });
    };
    addAndMakeVisible(dispersion_);

    damp_.BindParam(apvts, juce::String{"damp"} + juce::String{idx});
    damp_.OnMenuShowup() = [single = this](juce::PopupMenu& m) {
        m.addSeparator();
        m.addItem("set all", [ptr = single](){
            float const v = ptr->p_.damp_pitch_[0]->convertTo0to1(static_cast<float>(ptr->damp_.slider.getValue()));
            for (auto& param : ptr->p_.damp_pitch_) {
                param->setValueNotifyingHost(v);
            }
        });
    };
    addAndMakeVisible(damp_);

    gain_.BindParam(apvts, juce::String{"gain"} + juce::String{idx});
    gain_.OnMenuShowup() = [single = this](juce::PopupMenu& m) {
        m.addSeparator();
        m.addItem("set all", [ptr = single](){
            float const v = ptr->p_.damp_gain_db_[0]->convertTo0to1(static_cast<float>(ptr->gain_.slider.getValue()));
            for (auto& param : ptr->p_.damp_gain_db_) {
                param->setValueNotifyingHost(v);
            }
        });
    };
    addAndMakeVisible(gain_);

    decay_.BindParam(apvts, juce::String{"decay"} + juce::String{idx});
    decay_.OnMenuShowup() = [single = this](juce::PopupMenu& m) {
        m.addSeparator();
        m.addItem("set all", [ptr = single](){
            float const v = ptr->p_.decays_[0]->convertTo0to1(static_cast<float>(ptr->decay_.slider.getValue()));
            for (auto& param : ptr->p_.decays_) {
                param->setValueNotifyingHost(v);
            }
        });
    };
    addAndMakeVisible(decay_);

    polarity_.BindParam(apvts, juce::String{"polarity"} + juce::String{idx});
    addAndMakeVisible(polarity_);

    mix_.BindParam(apvts, juce::String{"mix"} + juce::String{idx});
    mix_.OnMenuShowup() = [single = this](juce::PopupMenu& m) {
        m.addSeparator();
        m.addItem("set all", [ptr = single](){
            float const v = ptr->p_.mix_volume_[0]->convertTo0to1(static_cast<float>(ptr->mix_.slider.getValue()));
            for (auto& param : ptr->p_.mix_volume_) {
                param->setValueNotifyingHost(v);
            }
        });
    };
    addAndMakeVisible(mix_);
}

void ResonatorBarGUI::resized() {
    auto b = getLocalBounds();
    pitch_.setBounds(b.removeFromLeft(b.getHeight()));
    midi_pitch_.setBounds(pitch_.getBounds());
    fine_.setBounds(b.removeFromLeft(b.getHeight()));
    dispersion_.setBounds(b.removeFromLeft(b.getHeight()));
    damp_.setBounds(b.removeFromLeft(b.getHeight()));
    gain_.setBounds(b.removeFromLeft(b.getHeight()));
    decay_.setBounds(b.removeFromLeft(b.getHeight()));
    auto polarity_width = static_cast<int>(static_cast<float>(b.getHeight()) * 0.4f);
    polarity_.setBounds(b.removeFromLeft(polarity_width).withSizeKeepingCentre(polarity_width, polarity_width));
    mix_.setBounds(b.removeFromLeft(b.getHeight()));
}

void ResonatorBarGUI::paint(juce::Graphics& g) {
    g.fillAll(ui::green_bg);
}

void ResonatorBarGUI::SetMidiDrive(bool midi_drive) {
    pitch_.setVisible(!midi_drive);
    midi_pitch_.setVisible(midi_drive);
}

void ResonatorBarGUI::UpdateMidiNote() {
    int const now_midi = p_.note_manager_.getVoice(idx_).midiNote;
    if (now_midi != old_midi_pitch_) {
        old_midi_pitch_ = now_midi;
        midi_pitch_.setText(FormatMidiNote(now_midi), juce::dontSendNotification);
    }
}



ResonatorGui::ResonatorGui(ResonatorAudioProcessor& p)
    : r0_(p, 0)
    , r1_(p, 1)
    , r2_(p, 2)
    , r3_(p, 3)
    , r4_(p, 4)
    , r5_(p, 5)
    , r6_(p, 6)
    , r7_(p, 7) 
{
    addAndMakeVisible(r0_);
    addAndMakeVisible(r1_);
    addAndMakeVisible(r2_);
    addAndMakeVisible(r3_);
    addAndMakeVisible(r4_);
    addAndMakeVisible(r5_);
    addAndMakeVisible(r6_);
    addAndMakeVisible(r7_);

    startTimerHz(30);
}

void ResonatorGui::resized() {
    auto resonators = getLocalBounds();
    auto height = resonators.getHeight() / 8;
    r0_.setBounds(resonators.removeFromTop(height).reduced(2));
    r1_.setBounds(resonators.removeFromTop(height).reduced(2));
    r2_.setBounds(resonators.removeFromTop(height).reduced(2));
    r3_.setBounds(resonators.removeFromTop(height).reduced(2));
    r4_.setBounds(resonators.removeFromTop(height).reduced(2));
    r5_.setBounds(resonators.removeFromTop(height).reduced(2));
    r6_.setBounds(resonators.removeFromTop(height).reduced(2));
    r7_.setBounds(resonators.removeFromTop(height).reduced(2));
}

void ResonatorGui::SetMidiDrive(bool midi_drive) {
    midi_drive_ = midi_drive;
    r0_.SetMidiDrive(midi_drive);
    r1_.SetMidiDrive(midi_drive);
    r2_.SetMidiDrive(midi_drive);
    r3_.SetMidiDrive(midi_drive);
    r4_.SetMidiDrive(midi_drive);
    r5_.SetMidiDrive(midi_drive);
    r6_.SetMidiDrive(midi_drive);
    r7_.SetMidiDrive(midi_drive);
}

void ResonatorGui::timerCallback() {
    if (!midi_drive_) {
        return;
    }

    r0_.UpdateMidiNote();
    r1_.UpdateMidiNote();
    r2_.UpdateMidiNote();
    r3_.UpdateMidiNote();
    r4_.UpdateMidiNote();
    r5_.UpdateMidiNote();
    r6_.UpdateMidiNote();
    r7_.UpdateMidiNote();
}
