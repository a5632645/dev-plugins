#pragma once
#include "PluginProcessor.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"

#include "tooltips.hpp"
#include "ui/toggle_button.hpp"
#include "ui/vertical_slider.hpp"
#include "ui/comb_box.hpp"
#include "ui/look_and_feel.hpp"

#include "widget/burg_lpc.hpp"
#include "widget/channel_selector.hpp"
#include "widget/rls_lpc.hpp"
#include "widget/stft_vocoder.hpp"
#include "widget/channel_vocoder.hpp"
#include "widget/gain.hpp"
#include "widget/ensemble.hpp"
#include "widget/tracking.hpp"

//==============================================================================
class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor
    , private juce::Timer
    , private juce::ComboBox::Listener
{
public:
    explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void OnLanguageChanged(tooltip::Tooltips& tooltips);

    tooltip::Tooltips tooltips_;
private:
    void timerCallback() override;
    void comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged) override;

    ui::MyLookAndFeel myLookAndFeel_;
    AudioPluginAudioProcessor& processorRef;
    juce::TooltipWindow tooltip_window_;

    juce::Label filter_;
    ui::VerticalSlider em_pitch_;
    ui::VerticalSlider em_gain_;
    ui::VerticalSlider em_s_;
    juce::Label shifter_{"", "Enable"};
    ui::ToggleButton shift_enable_;
    ui::VerticalSlider shift_pitch_;
    widget::Gain main_gain_;
    widget::Gain side_gain_;
    widget::Gain output_gain_;
    widget::ChannelSelector main_channel_selector_;
    widget::ChannelSelector side_channel_selector_;
    juce::ComboBox language_box_;

    ui::CombBox vocoder_type_;

    widget::STFTVocoder stft_vocoder_;
    widget::BurgLPC burg_lpc_;
    widget::RLSLPC rls_lpc_;
    widget::ChannelVocoder channel_vocoder_;
    juce::Component* current_vocoder_widget_{};

    widget::Ensemble ensemble_;
    widget::Tracking tracking_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
