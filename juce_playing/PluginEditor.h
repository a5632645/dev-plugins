#pragma once
#include "PluginProcessor.h"

#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"

#include "ui/look_and_feel.hpp"

//==============================================================================
class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor
    , private juce::Timer
{
public:
    explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    ui::MyLookAndFeel look_;
    AudioPluginAudioProcessor& processorRef;
    juce::TooltipWindow tooltip_window_;

    std::vector<std::unique_ptr<juce::Component>> controls_;
    juce::TextButton button_add_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
