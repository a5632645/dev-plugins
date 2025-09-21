#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace ui {

class ToggleButton : public juce::ToggleButton {
public:
    ~ToggleButton() override;
    void BindParameter(juce::AudioProcessorValueTreeState& apvts, const juce::String& id);
private:
    std::unique_ptr<juce::ButtonParameterAttachment> attach_;
};

}