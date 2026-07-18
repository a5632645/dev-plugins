#pragma once

#include <pluginshared/component.hpp>
#include <pluginshared/preset_panel.hpp>

class EmptyAudioProcessor;

class PluginUi : public juce::Component {
public:
    static constexpr int kWidth = 520;
    static constexpr int kHeight = 360;

    explicit PluginUi(EmptyAudioProcessor& p);

    void resized() override;

private:
    pluginshared::PresetPanel preset_;

    juce::Slider rateSlider_;
    juce::Slider depthSlider_;
    juce::Slider feedbackSlider_;
    juce::Slider mixSlider_;
    juce::ComboBox stagesCombo_;

    juce::Label rateLabel_;
    juce::Label depthLabel_;
    juce::Label feedbackLabel_;
    juce::Label mixLabel_;
    juce::Label stagesLabel_;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> rateAttach_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> depthAttach_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> feedbackAttach_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttach_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> stagesAttach_;
};
