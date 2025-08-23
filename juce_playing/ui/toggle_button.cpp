#include "toggle_button.hpp"

namespace ui {
ToggleButton::~ToggleButton() {
    attach_ = nullptr;
}

void ToggleButton::BindParameter(juce::AudioProcessorValueTreeState& apvts, const juce::String& id) {
    auto* p = apvts.getParameter(id);
    jassert(p != nullptr);
    attach_ = std::make_unique<juce::ButtonParameterAttachment>(*p, *this);
}
}