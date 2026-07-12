#pragma once
#include "global.hpp"
#include "pluginshared/bpm_sync_ui.hpp"
#include "pluginshared/component.hpp"
#include "pluginshared/preset_panel.hpp"
#include "spectral_view.hpp"
#include "time_view.hpp"

//==============================================================================
class PluginUi final
    : public juce::Component
    , public juce::Timer {
public:
    explicit PluginUi(SteepFlangerAudioProcessor&);
    ~PluginUi() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;

    void UpdateGui() {
        timeview_.UpdateGui();
        spectralview_.UpdateGui();
    }

    void UpdateGuiFromTimeView() {
        spectralview_.UpdateGui();
    }

    void timerCallback() override;
private:
    void TrySetSize(int width, int height);

    void SetIirMode(bool is_iir);

    SteepFlangerAudioProcessor& p_;
    pluginshared::PresetPanel preset_panel_;

    const juce::Rectangle<int> lfo_bound_{0, 30, 150, 150};
    juce::Label lfo_title_{"lfo", "lfo"};
    ui::Dial delay_{"delay"};
    ui::Dial depth_{"depth"};
    ui::BpmSyncDial speed_{"speed"};
    ui::Dial phase_{"phase"};
    ui::Dial drywet_{"drywet"};

    const juce::Rectangle<int> fir_bound_{370, 30, 150, 85};
    ui::Switch iir_mode_{"iir", "fir"};
    ui::Dial cutoff_{"cutoff"};
    ui::Dial coeff_len_{"steep"};
    ui::Dial side_lobe_{"side_lobe"};
    ui::Switch minum_phase_{"min(Φ)"};
    ui::Switch highpass_{"highpass"};

    const juce::Rectangle<int> feedback_bound_{0, 180, 150, 85};
    juce::Label feedback_title_{"feedback", "feedback"};
    ui::Dial fb_value_{"gain"};
    ui::Dial fb_damp_{"damp"};
    ui::FlatButton panic_;

    const juce::Rectangle<int> barber_bound_{370, 180, 150, 85};
    juce::Label barber_title_{"barberpole", "barberpole"};
    ui::Switch barber_enable_{"enable"};
    ui::Dial barber_phase_{"phase"};
    ui::BpmSyncDial barber_speed_{"speed"};
    ui::Dial barber_stereo_{"stereo"};

    const juce::Rectangle<int> ops_bound_{370, 115, 150, 65};
    juce::Label title_{"", "Time view"};
    ui::FlatButton reload_{"reload"};
    ui::FlatButton copy_{"copy"};
    ui::FlatButton clear_{"clear"};
    ui::Switch display_custom_{"show ctm"};

    TimeView timeview_;
    SpectralView spectralview_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginUi)
};
