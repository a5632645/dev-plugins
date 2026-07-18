#include "plugin_ui.hpp"

#include "../PluginProcessor.h"

//==============================================================================
PluginUi::PluginUi(EmptyAudioProcessor& p)
    : preset_(*p.preset_manager_)
{
    // ---- configure sliders ----
    auto setupSlider = [&](juce::Slider& s, juce::Label& label, const juce::String& text) {
        s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        addAndMakeVisible(s);

        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.attachToComponent(&s, false);
        addAndMakeVisible(label);
    };

    setupSlider(rateSlider_, rateLabel_, "Rate");
    setupSlider(depthSlider_, depthLabel_, "Depth");
    setupSlider(feedbackSlider_, feedbackLabel_, "Feedback");
    setupSlider(mixSlider_, mixLabel_, "Mix");

    // ---- stages combo ----
    stagesLabel_.setText("Stages", juce::dontSendNotification);
    stagesLabel_.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(stagesLabel_);

    stagesCombo_.addItemList(
        juce::StringArray{"2", "4", "6", "8", "10", "12"}, 1);
    addAndMakeVisible(stagesCombo_);

    // ---- APVTS attachments ----
    rateAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        *p.value_tree_, param_ids::rate, rateSlider_);
    depthAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        *p.value_tree_, param_ids::depth, depthSlider_);
    feedbackAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        *p.value_tree_, param_ids::feedback, feedbackSlider_);
    mixAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        *p.value_tree_, param_ids::mix, mixSlider_);
    stagesAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        *p.value_tree_, param_ids::stages, stagesCombo_);

    setSize(kWidth, kHeight);
}

//==============================================================================
void PluginUi::resized() {
    auto b = getLocalBounds();
    preset_.setBounds(b.removeFromTop(28));
    b.removeFromTop(8);

    // Layout: 5 columns for the 5 param controls
    const int knobW = juce::jmin(80, b.getWidth() / 6);
    const int knobY = b.getY();

    auto placeKnob = [&](juce::Slider& slider, juce::Label& label, int x) {
        auto area = juce::Rectangle{x, knobY, knobW, b.getHeight()};
        label.setBounds(area.removeFromTop(20));
        slider.setBounds(area.reduced(4));
    };

    int x = b.getX() + (b.getWidth() - knobW * 5) / 2;
    placeKnob(rateSlider_, rateLabel_, x);     x += knobW;
    placeKnob(depthSlider_, depthLabel_, x);     x += knobW;
    placeKnob(feedbackSlider_, feedbackLabel_, x); x += knobW;
    placeKnob(mixSlider_, mixLabel_, x);          x += knobW;

    // Stages combo
    const int comboW = 80;
    auto comboArea = juce::Rectangle{x, knobY, comboW, b.getHeight()};
    stagesLabel_.setBounds(comboArea.removeFromTop(20));
    stagesCombo_.setBounds(comboArea.reduced(4));
}
