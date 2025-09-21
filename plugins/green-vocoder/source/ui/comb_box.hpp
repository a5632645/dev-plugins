#pragma once
#include "juce_audio_processors/juce_audio_processors.h"
#include "juce_core/juce_core.h"
#include "juce_events/juce_events.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "tooltips.hpp"
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
        auto bb = juce::TextLayout::getStringBounds(getLookAndFeel().getLabelFont(label_), label_.getText());
        label_.setBounds(b.removeFromLeft(bb.getWidth() * 1.5f));
        combobox_.setBounds(b);
    }

    void OnLanguageChanged(tooltip::Tooltips& tooltips) {
        label_.setText(tooltips.Label(id_), juce::NotificationType::dontSendNotification);
        combobox_.setTooltip(tooltips.Tooltip(id_));

        if (!no_choice_str_) {
            auto item_ids = tooltips.CombboxIds(id_);
            int select_idx = combobox_.getSelectedItemIndex();
            combobox_.clear(juce::dontSendNotification);
            for (int juce_id = 1; auto& item_id : item_ids) {
                combobox_.addItem(tooltips.Label(item_id), juce_id++);
            }
            combobox_.setSelectedItemIndex(select_idx, juce::NotificationType::dontSendNotification);
        }

        if (!getLocalBounds().isEmpty()) {
            resized();
        }
    }

    void SetNoChoiceStrs(bool b) {
        no_choice_str_ = b;
    }

    juce::ComboBox combobox_;
private:
    juce::Label label_;
    const char* id_{};
    bool no_choice_str_{false};
    std::unique_ptr<juce::ComboBoxParameterAttachment> attach_;
};

}