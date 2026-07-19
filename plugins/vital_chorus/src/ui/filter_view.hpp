#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class VitalChorusAudioProcessor;

class FilterView : public juce::Component {
public:
    FilterView(VitalChorusAudioProcessor& p)
        : p_(p) {}
    void paint(juce::Graphics& g) override;
private:
    VitalChorusAudioProcessor& p_;
};
