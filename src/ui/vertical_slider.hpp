#pragma once
#include "juce_core/juce_core.h"
#include "tooltips.hpp"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace ui {

class VerticalSlider : public juce::Component, public tooltip::Tooltips::Listener {
public:
    VerticalSlider();
    ~VerticalSlider() override;
    void BindParameter(juce::AudioProcessorValueTreeState& apvts, const char* id);
    // void SetShortName(juce::String name);
    void resized() override;
    juce::Slider slider_;

    void OnLanguageChanged(tooltip::Tooltips& tooltips) override;
private:
    juce::String short_name_;
    juce::Label label_;
    const char* id_;
    std::unique_ptr<juce::SliderParameterAttachment> attach_;
};

}