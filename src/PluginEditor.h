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
#include "widget/rls_lpc.hpp"
#include "widget/stft_vocoder.hpp"
#include "widget/channel_vocoder.hpp"
#include "widget/gain.hpp"
#include "widget/ensemble.hpp"

//==============================================================================
class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor
    , private juce::Timer
    , private juce::ComboBox::Listener
    , private tooltip::Tooltips::Listener
{
public:
    explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void OnLanguageChanged(tooltip::Tooltips& tooltips) override;
private:
    void timerCallback() override;
    void comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged) override;

    AudioPluginAudioProcessor& processorRef;
    juce::TooltipWindow tooltip_window_;
    ui::MyLookAndFeel myLookAndFeel_;

    juce::Label filter_;
    ui::VerticalSlider em_pitch_;
    ui::VerticalSlider em_gain_;
    ui::VerticalSlider em_s_;
    ui::VerticalSlider hp_pitch_;
    juce::Label shifter_{"", "Enable"};
    ui::ToggleButton shift_enable_;
    ui::VerticalSlider shift_pitch_;
    widget::Gain main_gain_;
    widget::Gain side_gain_;
    widget::Gain output_gain_;
    juce::ComboBox language_box_;

    ui::CombBox vocoder_type_;

    widget::STFTVocoder stft_vocoder_;
    widget::BurgLPC burg_lpc_;
    widget::RLSLPC rls_lpc_;
    widget::ChannelVocoder channel_vocoder_;
    juce::Component* current_vocoder_widget_{};

    widget::Ensemble ensemble_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
