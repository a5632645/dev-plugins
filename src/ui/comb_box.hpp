#pragma once
#include "juce_audio_processors/juce_audio_processors.h"
#include "juce_core/juce_core.h"
#include "juce_events/juce_events.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <memory>

namespace ui {

class CombBox : public juce::Component {
public:
    CombBox() {
        addAndMakeVisible(label_);
        addAndMakeVisible(combobox_);
    }

    void BindParam(juce::AudioProcessorValueTreeState& apvts, juce::StringRef id) {
        auto* param = apvts.getParameter(id);
        attach_ = std::make_unique<juce::ComboBoxParameterAttachment>(*param, combobox_);
        combobox_.addItemList(static_cast<juce::AudioParameterChoice*>(param)->choices, 1);
        combobox_.setSelectedItemIndex(static_cast<juce::AudioParameterChoice*>(param)->getIndex(), juce::NotificationType::dontSendNotification);
    }

    void SetShortName(juce::StringRef name) {
        label_.setText(name, juce::dontSendNotification);
    }

    void resized() override {
        auto b = getLocalBounds();
        label_.setBounds(b.removeFromLeft(50));
        combobox_.setBounds(b);
    }

    juce::Label label_;
    juce::ComboBox combobox_;
private:
    std::unique_ptr<juce::ComboBoxParameterAttachment> attach_;
};

}