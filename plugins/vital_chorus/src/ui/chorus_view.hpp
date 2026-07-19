#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class VitalChorusAudioProcessor;

class ChorusView : public juce::Component, public juce::Timer {
public:
    ChorusView(VitalChorusAudioProcessor& p)
        : p_(p) {
        startTimerHz(30);
    }
    void paint(juce::Graphics& g) override;
private:
    VitalChorusAudioProcessor& p_;
    void timerCallback() override {
        repaint();
    }
};
