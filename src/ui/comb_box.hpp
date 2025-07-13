#pragma once
#include "juce_audio_processors/juce_audio_processors.h"
#include "juce_core/juce_core.h"
#include "juce_events/juce_events.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "tooltips.hpp"
#include <memory>

namespace ui {

class CombBox : public juce::Component, public tooltip::Tooltips::Listener {
public:
    CombBox() {
        addAndMakeVisible(label_);
        addAndMakeVisible(combobox_);
    }

    void BindParam(juce::AudioProcessorValueTreeState& apvts, const char* id) {
        auto* param = apvts.getParameter(juce::String::fromUTF8(id));
        attach_ = std::make_unique<juce::ComboBoxParameterAttachment>(*param, combobox_);
        // combobox_.addItemList(static_cast<juce::AudioParameterChoice*>(param)->choices, 1);
        
        id_ = id;
        tooltip::tooltips.AddListener(this);
        OnLanguageChanged(tooltip::tooltips);
        combobox_.setSelectedItemIndex(static_cast<juce::AudioParameterChoice*>(param)->getIndex(), juce::NotificationType::dontSendNotification);
    }

    // void SetShortName(juce::StringRef name) {
    //     label_.setText(name, juce::dontSendNotification);
    // }

    void resized() override {
        auto b = getLocalBounds();
        label_.setBounds(b.removeFromLeft(50));
        combobox_.setBounds(b);
    }

    void OnLanguageChanged(tooltip::Tooltips& tooltips) override {
        label_.setText(tooltips.Label(id_), juce::NotificationType::dontSendNotification);
        combobox_.setTooltip(tooltips.Tooltip(id_));

        auto item_ids = tooltips.CombboxIds(id_);
        int select_idx = combobox_.getSelectedItemIndex();
        combobox_.clear(juce::dontSendNotification);
        for (int juce_id = 1; auto& item_id : item_ids) {
            combobox_.addItem(tooltips.Label(item_id), juce_id++);
        }
        combobox_.setSelectedItemIndex(select_idx, juce::NotificationType::dontSendNotification);
    }

    juce::ComboBox combobox_;
private:
    juce::Label label_;
    const char* id_;
    std::unique_ptr<juce::ComboBoxParameterAttachment> attach_;
};

}