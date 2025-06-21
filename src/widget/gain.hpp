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

    Gain(dsp::Gain& gain) : gain_(gain) {
        addAndMakeVisible(gain_slide_);
    }

    void resized() override {
        auto b = getLocalBounds();
        auto e = b.removeFromLeft(20);
        gain_slide_.setBounds(b);
    }

    void paint(juce::Graphics& g) override {
        auto b = getLocalBounds();
        auto e = b.removeFromLeft(20).toFloat();
        e.reduce(5, 10);
        auto bck_e = e;
        
        g.setColour(juce::Colours::black);
        g.fillRect(e);

        float peak = gain_.peak_;
        float db = 20.0f * std::log10(peak + 1e-10f);
        float norm = (db - kMinDb) / (kMaxDb - kMinDb);
        float h = e.getHeight() * norm;
        if (peak > 1.0f) {
            g.setColour(juce::Colours::orange);
        }
        else {
            g.setColour(juce::Colours::green);
        }
        g.fillRect(e.removeFromBottom(h));

        g.setColour(juce::Colours::white);
        g.drawRect(bck_e);
    }
    
    ui::VerticalSlider gain_slide_;
private:
    dsp::Gain& gain_;
};

}