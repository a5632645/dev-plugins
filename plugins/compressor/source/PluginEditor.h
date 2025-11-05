#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <pluginshared/component.hpp>

class EmptyAudioProcessor;

class LimiterReduceMeter : public juce::Component, private juce::Timer {
public:
    LimiterReduceMeter(EmptyAudioProcessor& p);
    void paint(juce::Graphics& g) override;
private:
    void timerCallback() override {
        repaint();
    }

    EmptyAudioProcessor& p_;
};

// ---------------------------------------- editor ----------------------------------------
class SteepFlangerAudioProcessorEditor final 
    : public juce::AudioProcessorEditor {
public:
    explicit SteepFlangerAudioProcessorEditor (EmptyAudioProcessor&);
    ~SteepFlangerAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    EmptyAudioProcessor& p_;

    ui::Dial release_{"release"};
    ui::Dial makeup_{"makeup"};
    ui::Dial hold_{"hold"};
    ui::Dial lookahead_{"lookahead"};
    ui::Dial limit_{"limit"};
    LimiterReduceMeter meter_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SteepFlangerAudioProcessorEditor)
};
