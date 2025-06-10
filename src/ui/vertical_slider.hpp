#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace ui {

class VerticalSlider : public juce::Component {
public:
    VerticalSlider();
    ~VerticalSlider() override;
    void BindParameter(juce::AudioProcessorValueTreeState& apvts, const juce::String& id);
    void SetShortName(juce::String name);
    void resized() override;
    juce::Slider slider_;
private:
    juce::String short_name_;
    juce::Label label_;
    std::unique_ptr<juce::SliderParameterAttachment> attach_;
};

}