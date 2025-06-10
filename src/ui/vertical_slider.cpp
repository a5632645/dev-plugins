#include "vertical_slider.hpp"

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

void VerticalSlider::SetShortName(juce::String name) {
    short_name_ = std::move(name);
    label_.setText(short_name_, juce::NotificationType::dontSendNotification);
}

VerticalSlider::~VerticalSlider() {
    attach_ = nullptr;
}

void VerticalSlider::BindParameter(juce::AudioProcessorValueTreeState& apvts, const juce::String& id) {
    auto* p = apvts.getParameter(id);
    jassert(p != nullptr);
    attach_ = std::make_unique<juce::SliderParameterAttachment>(*p, slider_);
}

void VerticalSlider::resized() {
    auto b = getLocalBounds();
    auto e = b.removeFromBottom(20);
    label_.setBounds(e);
    slider_.setBounds(b);
}
}