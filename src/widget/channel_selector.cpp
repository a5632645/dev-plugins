#include "channel_selector.hpp"
#include "juce_core/juce_core.h"
#include "juce_graphics/juce_graphics.h"

namespace widget {

void ChannelSelector::BindParameter(juce::AudioProcessorValueTreeState& apvts, const char* const id) {
    auto* p = apvts.getParameter(juce::String::fromUTF8(id));
    jassert(p != nullptr);
    attach_ = std::make_unique<juce::SliderParameterAttachment>(*p, slider_);

    num_boxes_ = std::round(slider_.getRange().getEnd()) + 1;
    removeAllChildren();
    for (int i = 0; i < num_boxes_; ++i) {
        boxes_[i].on_click = [i, this] {
            slider_.setValue(i);
        };
        addAndMakeVisible(boxes_[i]);
    }

    slider_.onValueChange = [this] {
        int i = static_cast<int>(slider_.getValue());
        for (int j = 0; j < num_boxes_; ++j) {
            boxes_[j].SetSelected(j == i);
        }
        repaint();
    };
    
    addAndMakeVisible(label_);

    slider_.onValueChange();
}

void ChannelSelector::resized() {
    auto b = getLocalBounds();
    auto w = b.getWidth() / num_boxes_;
    for (int i = 0; i < num_boxes_; ++i) {
        auto bb = b.removeFromLeft(w).reduced(2);
        auto length = std::min(bb.getWidth(), bb.getHeight());
        bb = bb.withSizeKeepingCentre(length, length);
        boxes_[i].setBounds(bb);
    }
    
    b = getLocalBounds();
    label_.setBounds(b.removeFromTop(boxes_.front().getY()));
}

void ChannelSelector::paint(juce::Graphics& g) {
    static const juce::StringArray kStr{ "L", "R", "LR", "L", "R", "LR", "P" };
    g.setColour(juce::Colours::whitesmoke);
    for (int i = 0; i < num_boxes_; ++i) {
        juce::Rectangle box{ boxes_[i].getX(), boxes_[i].getBottom(), boxes_[i].getWidth(), getHeight() - boxes_[i].getBottom() };
        g.drawText(kStr[i], box, juce::Justification::centred);
    }
}

}