#include "look_and_feel.hpp"
#include "BinaryData.h"
#include "juce_graphics/juce_graphics.h"

namespace ui {

MyLookAndFeel::MyLookAndFeel() {
    typeface_ = juce::Typeface::createSystemTypefaceFor(BinaryData::quan_ttf, BinaryData::quan_ttfSize);
    setDefaultSansSerifTypeface(typeface_);
}

juce::Font MyLookAndFeel::getTextButtonFont(juce::TextButton& button, int buttonHeight) {
    auto op = juce::FontOptions{typeface_}.withHeight(std::max(12.0f, buttonHeight * 0.4f));
    // auto op = juce::FontOptions{typeface_}.withHeight(12);
    return juce::Font{op};
}

juce::Font MyLookAndFeel::getAlertWindowTitleFont() {
    auto op = juce::FontOptions{typeface_}.withHeight(12);
    return juce::Font{op};
}

juce::Font MyLookAndFeel::getAlertWindowMessageFont() {
    auto op = juce::FontOptions{typeface_}.withHeight(12);
    return juce::Font{op};
}

juce::Font MyLookAndFeel::getAlertWindowFont() {
    auto op = juce::FontOptions{typeface_}.withHeight(12);
    return juce::Font{op};
}

juce::Font MyLookAndFeel::getComboBoxFont(juce::ComboBox& box) {
    auto op = juce::FontOptions{typeface_}.withHeight(std::max(12.0f, box.getHeight() * 0.4f));
    // auto op = juce::FontOptions{typeface_}.withHeight(12);
    return juce::Font{op};
}

juce::Font MyLookAndFeel::getLabelFont(juce::Label& label) {
    auto op = juce::FontOptions{typeface_}.withHeight(std::max(12.0f, label.getHeight() * 0.4f));
    // auto op = juce::FontOptions{typeface_}.withHeight(12);
    return juce::Font{op};
}

void MyLookAndFeel::drawTooltip(juce::Graphics& g, const juce::String& text, int width, int height) {
    // 设置背景颜色
    g.fillAll (findColour (juce::TooltipWindow::backgroundColourId));

    // 设置字体和颜色
    auto op = juce::FontOptions{typeface_}.withHeight(12);
    g.setFont (juce::Font{op});
    g.setColour (findColour (juce::TooltipWindow::textColourId));

    // 绘制文本
    g.drawFittedText (text, 0, 0, width, height, juce::Justification::centredLeft, 10);

    // (可选) 绘制边框
    g.setColour (findColour(juce::TooltipWindow::outlineColourId));
    g.drawRect (0, 0, width, height, 1);
}

}