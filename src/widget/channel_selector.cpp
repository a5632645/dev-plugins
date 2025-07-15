#include "channel_selector.hpp"
#include "juce_core/juce_core.h"
#include "juce_graphics/juce_graphics.h"

namespace widget {

ChannelSelector::ChannelSelector() {
    for (int i = 0; i < 6; ++i) {
        boxes_[i].on_click = [i, this] {
            slider_.setValue(i);
        };
        addAndMakeVisible(boxes_[i]);
    }

    slider_.onValueChange = [this] {
        int i = static_cast<int>(slider_.getValue());
        for (int j = 0; j < 6; ++j) {
            boxes_[j].SetSelected(j == i);
        }
        repaint();
    };
    
    addAndMakeVisible(label_);
}

void ChannelSelector::BindParameter(juce::AudioProcessorValueTreeState& apvts, const char* const id) {
    auto* p = apvts.getParameter(juce::String::fromUTF8(id));
    jassert(p != nullptr);
    attach_ = std::make_unique<juce::SliderParameterAttachment>(*p, slider_);
    // 初始化slider.value=0导致绑定后不更新,在这里强制更新
    slider_.onValueChange();
}

void ChannelSelector::resized() {
    auto b = getLocalBounds();
    auto w = b.getWidth() / 6;
    for (int i = 0; i < 6; ++i) {
        auto bb = b.removeFromLeft(w).reduced(2);
        auto length = std::min(bb.getWidth(), bb.getHeight());
        bb = bb.withSizeKeepingCentre(length, length);
        boxes_[i].setBounds(bb);
    }
    
    b = getLocalBounds();
    label_.setBounds(b.removeFromTop(boxes_.front().getY()));
}

void ChannelSelector::paint(juce::Graphics& g) {
    static const juce::StringArray kStr{ "L", "R", "LR", "L", "R", "LR" };
    g.setColour(juce::Colours::whitesmoke);
    for (int i = 0; i < 6; ++i) {
        juce::Rectangle box{ boxes_[i].getX(), boxes_[i].getBottom(), boxes_[i].getWidth(), getHeight() - boxes_[i].getBottom() };
        g.drawText(kStr[i], box, juce::Justification::centred);
    }
}

}