#pragma once
#include "dsp/gain.hpp"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "ui/vertical_slider.hpp"

namespace widget {

class Gain : public juce::Component {
public:
    static constexpr float kMinDb = -60.0f;
    static constexpr float kMaxDb = 20.0f;

    Gain(dsp::IGain& gain) : gain_(gain) {
        addAndMakeVisible(gain_slide_);
    }

    void resized() override {
        auto b = getLocalBounds();
        gain_slide_.setBounds(b);
    }

    void paint(juce::Graphics& g) override {
        int nchannel = gain_.GetNumChannels();
        float total = 0.0f;
        for (int channel = 0; channel < nchannel; ++channel) {
            total += gain_.GetPeak(channel);
        }
        total /= nchannel;

        auto b = getLocalBounds();
        g.setColour(total > 1.0f ? juce::Colours::orange : juce::Colours::green);
        g.drawRect(b);
    }
    
    ui::VerticalSlider gain_slide_;
private:
    dsp::IGain& gain_;
};

}