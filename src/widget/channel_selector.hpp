#pragma once
#include "juce_audio_processors/juce_audio_processors.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <array>
#include <memory>

namespace widget {
class ChannelSelector : public juce::Component {
public:
    class Box : public juce::Component {
    public:
        Box() = default;
        std::function<void()> on_click;

        void mouseDown(const juce::MouseEvent&) override { on_click(); }
        void SetSelected(bool selected) { selected_ = selected; }
        void paint(juce::Graphics& g) override {
            auto b = getLocalBounds();
            g.setColour(juce::Colours::white);
            g.drawRect(b);
            if (selected_) {
                g.drawText("X", b, juce::Justification::centred);
            }
        }
    private:
        bool selected_ = false;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Box)
    };

    ChannelSelector();
    void resized() override;
    void BindParameter(juce::AudioProcessorValueTreeState& apvts, const char* const id);
    void SetLabelName(const juce::String& name) { label_.setText(name, juce::dontSendNotification); }
    void paint(juce::Graphics& g) override;
private:
    std::array<Box, 6> boxes_;
    juce::Slider slider_;
    juce::Label label_;
    std::unique_ptr<juce::SliderParameterAttachment> attach_;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelSelector)
};
}