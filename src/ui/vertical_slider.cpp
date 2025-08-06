#include "vertical_slider.hpp"
#include "PluginEditor.h"
#include "juce_core/juce_core.h"
#include "juce_events/juce_events.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "tooltips.hpp"
#include "param_ids.hpp"

namespace ui {
void MyPopmenuSlider::mouseDown(const juce::MouseEvent& e) {
    juce::ModifierKeys keys = juce::ModifierKeys::getCurrentModifiers();
    if (keys.isPopupMenu()) {
        AudioPluginAudioProcessorEditor* ed = findParentComponentOfClass<AudioPluginAudioProcessorEditor>();
        if (ed == nullptr) {
            return;
        }
        auto& tpis = ed->tooltips_;

        menu_.clear();
        menu_.addItem(tpis.Label(id::kSliderMenuEnterValue),
         [this, &tooltips = ed->tooltips_]{
            auto* editor = new juce::TextEditor;
            editor->setText(this->getTextFromValue(this->getValue()));

            juce::DialogWindow::LaunchOptions op;
            op.dialogTitle = tooltips.Label(id::kSliderMenuEnterValue);
            op.escapeKeyTriggersCloseButton = true;
            op.useNativeTitleBar = true;
            op.resizable = false;
            op.content = {editor, true};

            auto* dialog = op.create();
            editor->onReturnKey = [this, dialog, editor] {
                this->setValue(this->getValueFromText(editor->getText()),
                               juce::NotificationType::sendNotificationSync);
                this->onDragEnd();
                dialog->closeButtonPressed();
            };
            dialog->enterModalState(true, nullptr, true);
            juce::MessageManager::callAsync([editor]{
                editor->grabKeyboardFocus();
                editor->selectAll();
            });
        });

        juce::PopupMenu::Options op;
        menu_.showMenuAsync(op.withMousePosition());
    }
    else {
        juce::Slider::mouseDown(e);
    }
}
}

namespace ui {
VerticalSlider::VerticalSlider() {
    slider_.setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
    slider_.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox, false, 0, 0);
    auto display_value = [this] {
        label_.setText(slider_.getTextFromValue(slider_.getValue()), juce::NotificationType::dontSendNotification);
    };
    slider_.onDragStart = display_value;
    slider_.onValueChange = display_value;
    slider_.onDragEnd = [this] {
        label_.setText(short_name_, juce::NotificationType::dontSendNotification);
    };
    label_.setJustificationType(juce::Justification::centredBottom);
    addAndMakeVisible(slider_);
    addAndMakeVisible(label_);
}

VerticalSlider::~VerticalSlider() {
    attach_ = nullptr;
}

void VerticalSlider::BindParameter(juce::AudioProcessorValueTreeState& apvts, const char* id) {
    auto* p = apvts.getParameter(juce::String::fromUTF8(id));
    jassert(p != nullptr);
    attach_ = std::make_unique<juce::SliderParameterAttachment>(*p, slider_);
    
    id_ = id;
}

void VerticalSlider::resized() {
    auto b = getLocalBounds();
    auto e = b.removeFromBottom(20);
    label_.setBounds(e);
    slider_.setBounds(b);
}

void VerticalSlider::OnLanguageChanged(tooltip::Tooltips& tooltips) {
    short_name_ = tooltips.Label(id_);
    label_.setText(short_name_, juce::NotificationType::dontSendNotification);
    slider_.setTooltip(tooltips.Tooltip(id_));
}
}