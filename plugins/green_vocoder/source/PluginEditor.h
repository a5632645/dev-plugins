#pragma once
#include "PluginProcessor.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "pluginshared/preset_panel.hpp"
#include "widget/ensemble.hpp"
#include "widget/tracking.hpp"
#include "widget/pre_fx.hpp"
#include "widget/vocoder.hpp"

//==============================================================================
class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    AudioPluginAudioProcessor& processorRef;
    pluginshared::PresetPanel preset_panel_;
    juce::TooltipWindow tooltip_window_;

    green_vocoder::widget::PreFx pre_fx_;
    green_vocoder::widget::Vocoder vocoder_;
    green_vocoder::widget::Ensemble ensemble_;
    green_vocoder::widget::Tracking tracking_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
