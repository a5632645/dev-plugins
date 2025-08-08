#pragma once
#include "PluginProcessor.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "ui/look_and_feel.hpp"
#include "ui/vertical_slider.hpp"
#include "widget/performance.hpp"

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

private:
    void timerCallback() override;
    void comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged) override;

    ui::MyLookAndFeel look_;
    AudioPluginAudioProcessor& processorRef;
    juce::TooltipWindow tooltip_window_;

    ui::VerticalSlider reflection_;
    ui::VerticalSlider begin_;
    ui::VerticalSlider end_;
    ui::VerticalSlider freq_;
    ui::VerticalSlider num_;
    ui::VerticalSlider mix_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
