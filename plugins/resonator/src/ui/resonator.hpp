#pragma once
#include <pluginshared/component.hpp>

class ResonatorAudioProcessor;

class ResonatorBarGUI : public juce::Component {
public:
    ResonatorBarGUI(ResonatorAudioProcessor& p, size_t idx);
    void paint(juce::Graphics& g) override;
    void resized() override;

    void SetMidiDrive(bool connect);
    void UpdateMidiNote();
private:
    ResonatorAudioProcessor& p_;
    const size_t idx_;
    int old_midi_pitch_{-2};
    ui::Dial pitch_{"pitch"};
    juce::Label midi_pitch_;
    ui::Dial fine_{"fine"};
    ui::Dial dispersion_{"dispersion"};
    ui::Dial damp_{"damp"};
    ui::Dial gain_{"gain"};
    ui::Dial decay_{"decay"};
    ui::Switch polarity_{"-", "+"};
    ui::Dial mix_{"mix"};
};

class ResonatorGui : public juce::Component , private juce::Timer {
public:
    ResonatorGui(ResonatorAudioProcessor& p);

    void resized() override;
    void SetMidiDrive(bool midi_drive);
private:
    void timerCallback() override;

    bool midi_drive_{};
    ResonatorBarGUI r0_;
    ResonatorBarGUI r1_;
    ResonatorBarGUI r2_;
    ResonatorBarGUI r3_;
    ResonatorBarGUI r4_;
    ResonatorBarGUI r5_;
    ResonatorBarGUI r6_;
    ResonatorBarGUI r7_;
};
