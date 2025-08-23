#pragma once
#include "juce_audio_processors/juce_audio_processors.h"
#include "juce_core/juce_core.h"
#include "juce_events/juce_events.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <memory>

namespace ui {

class CombBox : public juce::Component {
public:
    CombBox() {
        addAndMakeVisible(label_);
        addAndMakeVisible(combobox_);
    }

    void BindParam(juce::AudioProcessorValueTreeState& apvts, const char* id) {
        auto* param = apvts.getParameter(juce::String::fromUTF8(id));
        attach_ = std::make_unique<juce::ComboBoxParameterAttachment>(*param, combobox_);
        
        id_ = id;
        auto* choices = static_cast<juce::AudioParameterChoice*>(param);
        combobox_.clear(juce::dontSendNotification);
        combobox_.addItemList(choices->choices, 1);
        combobox_.setSelectedItemIndex(static_cast<juce::AudioParameterChoice*>(param)->getIndex());
    }

    void resized() override {
        auto b = getLocalBounds();
        label_.setBounds(b.removeFromLeft(50));
        combobox_.setBounds(b);
    }

    juce::ComboBox combobox_;
private:
    juce::Label label_;
    const char* id_;
    std::unique_ptr<juce::ComboBoxParameterAttachment> attach_;
};

}