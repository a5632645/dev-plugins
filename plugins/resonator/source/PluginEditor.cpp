#include "PluginEditor.h"
#include "PluginProcessor.h"

// ---------------------------------------- resonator ----------------------------------------
ResonatorGUI::ResonatorGUI(ResonatorAudioProcessor& p, size_t idx)
    : p_(p)
    , idx_(idx) {
    auto& apvts = *p.value_tree_;

    pitch_.BindParam(apvts, juce::String{"pitch"} + juce::String{idx});
    pitch_.slider.onValueChange = [this] {
        backup_pitch_ = pitch_.slider.getValue();
    };
    addAndMakeVisible(pitch_);

    fine_.BindParam(apvts, juce::String{"fine"} + juce::String{idx});
    addAndMakeVisible(fine_);

    dispersion_.BindParam(apvts, juce::String{"dispersion"} + juce::String{idx});
    addAndMakeVisible(dispersion_);

    damp_.BindParam(apvts, juce::String{"damp"} + juce::String{idx});
    addAndMakeVisible(damp_);

    decay_.BindParam(apvts, juce::String{"decay"} + juce::String{idx});
    addAndMakeVisible(decay_);

    polarity_.BindParam(apvts, juce::String{"polarity"} + juce::String{idx});
    addAndMakeVisible(polarity_);

    mix_.BindParam(apvts, juce::String{"mix"} + juce::String{idx});
    addAndMakeVisible(mix_);
    
    backup_pitch_ = pitch_.slider.getValue();
}

void ResonatorGUI::resized() {
    auto b = getLocalBounds();
    pitch_.setBounds(b.removeFromLeft(b.getHeight()));
    fine_.setBounds(b.removeFromLeft(b.getHeight()));
    dispersion_.setBounds(b.removeFromLeft(b.getHeight()));
    damp_.setBounds(b.removeFromLeft(b.getHeight()));
    decay_.setBounds(b.removeFromLeft(b.getHeight()));
    auto polarity_width = b.getHeight() * 0.3f;
    polarity_.setBounds(b.removeFromLeft(polarity_width).withSizeKeepingCentre(polarity_width, polarity_width));
    mix_.setBounds(b.removeFromLeft(b.getHeight()));
}

void ResonatorGUI::paint(juce::Graphics& g) {
    auto b = getLocalBounds().reduced(0, 2);
    g.setColour(ui::green_bg);
    g.fillRect(b);
}

void ResonatorGUI::ConnectParam(bool connect) {
    if (connect) {
        pitch_.slider.setValue(backup_pitch_, juce::dontSendNotification);
    }
    else {
        backup_pitch_ = pitch_.slider.getValue();
    }
}

void ResonatorGUI::UpdateMidiNote() {
    pitch_.slider.setValue(p_.note_manager_.getVoice(idx_).midiNote, juce::dontSendNotification);
}

// ---------------------------------------- editor ----------------------------------------

ResonatorAudioProcessorEditor::ResonatorAudioProcessorEditor (ResonatorAudioProcessor& p)
    : AudioProcessorEditor (&p)
    , p_(p)
    , r0_(p, 0)
    , r1_(p, 1)
    , r2_(p, 2)
    , r3_(p, 3)
    , r4_(p, 4)
    , r5_(p, 5)
    , r6_(p, 6)
    , r7_(p, 7)
{
    auto& apvts = *p.value_tree_;

    addAndMakeVisible(r0_);
    addAndMakeVisible(r1_);
    addAndMakeVisible(r2_);
    addAndMakeVisible(r3_);
    addAndMakeVisible(r4_);
    addAndMakeVisible(r5_);
    addAndMakeVisible(r6_);
    addAndMakeVisible(r7_);

    reflection0_.BindParam(apvts, "reflection0");
    addAndMakeVisible(reflection0_);
    reflection1_.BindParam(apvts, "reflection1");
    addAndMakeVisible(reflection1_);
    reflection2_.BindParam(apvts, "reflection2");
    addAndMakeVisible(reflection2_);
    reflection3_.BindParam(apvts, "reflection3");
    addAndMakeVisible(reflection3_);
    reflection4_.BindParam(apvts, "reflection4");
    addAndMakeVisible(reflection4_);
    reflection5_.BindParam(apvts, "reflection5");
    addAndMakeVisible(reflection5_);
    reflection6_.BindParam(apvts, "reflection6");
    addAndMakeVisible(reflection6_);

    addAndMakeVisible(midi_title_);
    midi_drive_.BindParam(apvts, "midi_drive");
    addAndMakeVisible(midi_drive_);
    round_robin_.BindParam(apvts, "round_robin");
    addAndMakeVisible(round_robin_);

    addAndMakeVisible(global_title_);
    global_mix_.BindParam(apvts, "global_mix");
    addAndMakeVisible(global_mix_);
    global_decay_.BindParam(apvts, "global_decay");
    addAndMakeVisible(global_decay_);
    global_damp_.BindParam(apvts, "global_damp");
    addAndMakeVisible(global_damp_);
    dry_.BindParam(apvts, "dry");
    addAndMakeVisible(dry_);

    setSize(800, 600);
    startTimerHz(15);
}

ResonatorAudioProcessorEditor::~ResonatorAudioProcessorEditor() {
}

//==============================================================================
void ResonatorAudioProcessorEditor::paint (juce::Graphics& g) {
    g.fillAll(ui::black_bg);

    g.setColour(ui::green_bg);
    auto b = getLocalBounds();
    b.removeFromLeft(80 * 6);
    b.removeFromLeft(4);
    auto matrix_b = b.removeFromTop(80 * 4);
    g.fillRect(matrix_b);

    b.removeFromTop(4);
    auto misc_b = b;
    g.fillRect(misc_b);
}

void ResonatorAudioProcessorEditor::resized() {
    auto b = getLocalBounds();
    {
        auto resonators = b.removeFromLeft(80 * 6);
        auto height = resonators.getHeight() / 8;
        r0_.setBounds(resonators.removeFromTop(height));
        r1_.setBounds(resonators.removeFromTop(height));
        r2_.setBounds(resonators.removeFromTop(height));
        r3_.setBounds(resonators.removeFromTop(height));
        r4_.setBounds(resonators.removeFromTop(height));
        r5_.setBounds(resonators.removeFromTop(height));
        r6_.setBounds(resonators.removeFromTop(height));
        r7_.setBounds(resonators.removeFromTop(height));
    }
    {
        auto martix_b = b.removeFromTop(80 * 4);
        {
            auto first4 = martix_b.removeFromLeft(80);
            reflection0_.setBounds(first4.removeFromTop(80));
            reflection1_.setBounds(first4.removeFromTop(80));
            reflection2_.setBounds(first4.removeFromTop(80));
            reflection3_.setBounds(first4.removeFromTop(80));
        }
        {
            auto second2 = martix_b.removeFromLeft(80);
            reflection4_.setBounds(second2.removeFromTop(second2.getHeight() / 2).withSizeKeepingCentre(80, 80));
            reflection5_.setBounds(second2.withSizeKeepingCentre(80, 80));
        }
        {
            auto last1 = martix_b.removeFromLeft(80);
            reflection6_.setBounds(last1.withSizeKeepingCentre(80, 80));
        }
    }
    {
        b.removeFromTop(4);
        b.removeFromLeft(4);
        auto misc_b = b;
        {
            midi_title_.setBounds(misc_b.removeFromTop(20));
            auto top = misc_b.removeFromTop(30);
            midi_drive_.setBounds(top.removeFromLeft(100).reduced(2, 2));
            round_robin_.setBounds(top.removeFromLeft(100).reduced(2, 2));
        }
        {
            global_title_.setBounds(misc_b.removeFromTop(20));
            auto top = misc_b.removeFromTop(80);
            global_decay_.setBounds(top.removeFromLeft(60));
            global_mix_.setBounds(top.removeFromLeft(60));
            global_damp_.setBounds(top.removeFromLeft(60));
            dry_.setBounds(top.removeFromLeft(60));
        }
    }
}

void ResonatorAudioProcessorEditor::timerCallback() {
    bool midi_drive = midi_drive_.getToggleState();
    if (midi_drive != was_midi_drive_) {
        was_midi_drive_ = midi_drive;
        r0_.ConnectParam(!midi_drive);
        r1_.ConnectParam(!midi_drive);
        r2_.ConnectParam(!midi_drive);
        r3_.ConnectParam(!midi_drive);
        r4_.ConnectParam(!midi_drive);
        r5_.ConnectParam(!midi_drive);
        r6_.ConnectParam(!midi_drive);
        r7_.ConnectParam(!midi_drive);
    }

    if (!midi_drive) {
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