#pragma once
#include "../../shared/component.hpp"

class ResonatorAudioProcessor;

// ---------------------------------------- resonator ----------------------------------------

class ResonatorGUI : public juce::Component {
public:
    ResonatorGUI(ResonatorAudioProcessor& p, size_t idx);
    void paint(juce::Graphics& g) override;
    void resized() override;

    void ConnectParam(bool connect);
    void UpdateMidiNote();
private:
    ResonatorAudioProcessor& p_;
    const size_t idx_;
    ui::Dial pitch_{"pitch"};
    ui::Dial fine_{"fine"};
    ui::Dial dispersion_{"dispersion"};
    ui::Dial damp_{"damp"};
    ui::Dial gain_{"gain"};
    ui::Dial decay_{"decay"};
    ui::Switch polarity_{"-", "+"};
    ui::Dial mix_{"mix"};
    float backup_pitch_{};
};

// ---------------------------------------- editor ----------------------------------------

class ResonatorAudioProcessorEditor final 
    : public juce::AudioProcessorEditor
    , public juce::Timer {
public:
    explicit ResonatorAudioProcessorEditor (ResonatorAudioProcessor&);
    ~ResonatorAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    ResonatorAudioProcessor& p_;

    ResonatorGUI r0_;
    ResonatorGUI r1_;
    ResonatorGUI r2_;
    ResonatorGUI r3_;
    ResonatorGUI r4_;
    ResonatorGUI r5_;
    ResonatorGUI r6_;
    ResonatorGUI r7_;

    ui::Dial reflection0_{"couple"};
    ui::Dial reflection1_{"couple"};
    ui::Dial reflection2_{"couple"};
    ui::Dial reflection3_{"couple"};
    ui::Dial reflection4_{"couple"};
    ui::Dial reflection5_{"couple"};
    ui::Dial reflection6_{"couple"};

    juce::Label midi_title_{"", "midi"};
    ui::Switch midi_drive_{"midi"};
    bool was_midi_drive_{false};
    ui::Switch round_robin_{"round_robin"};
    juce::Label global_title_{"", "global setting"};
    ui::Dial global_decay_{"decay"};
    ui::Dial global_mix_{"mix"};
    ui::Dial global_damp_{"damp"};
    ui::Dial dry_{"dry"};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResonatorAudioProcessorEditor)
};
