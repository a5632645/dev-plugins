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
        auto e = b.removeFromRight(50);
        gain_slide_.setBounds(e);
    }

    void paint(juce::Graphics& g) override {
        auto b = getLocalBounds();
        b.removeFromRight(50);
        auto e = b.toFloat();
        e.reduce(3, 3);
        
        g.setColour(juce::Colours::black);
        g.fillRect(e);

        int nchannel = gain_.GetNumChannels();
        auto box = e.withWidth(e.getWidth() / nchannel);
        for (int channel = 0; channel < nchannel; ++channel) {
            float peak = gain_.GetPeak(channel);
            float db = 20.0f * std::log10(peak + 1e-10f);
            float norm = (db - kMinDb) / (kMaxDb - kMinDb);
            float h = e.getHeight() * norm;
            if (peak > 1.0f) {
                g.setColour(juce::Colours::orange);
            }
            else {
                g.setColour(juce::Colours::green);
            }
            auto bbox = box;
            g.fillRect(bbox.removeFromBottom(h));
    
            g.setColour(juce::Colours::white);
            g.drawRect(box);
            box.translate(box.getWidth(), 0.0f);
        }
    }
    
    ui::VerticalSlider gain_slide_;
private:
    dsp::IGain& gain_;
};

}