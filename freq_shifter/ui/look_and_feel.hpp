#pragma once
#include "juce_graphics/juce_graphics.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace ui {

class MyLookAndFeel : public juce::LookAndFeel_V4 {
public:
    MyLookAndFeel();
    ~MyLookAndFeel() override = default;

    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override;
    juce::Font getAlertWindowTitleFont() override;
    juce::Font getAlertWindowMessageFont() override;
    juce::Font getAlertWindowFont() override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;
    juce::Font getLabelFont (juce::Label& label) override;
    juce::Font getPopupMenuFont () override;
    void drawTooltip (juce::Graphics& g, const juce::String& text, int width, int height) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;
private:
    juce::Typeface::Ptr typeface_;
};

}