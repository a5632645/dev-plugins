#include "look_and_feel.hpp"
#include "BinaryData.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"

namespace ui {

MyLookAndFeel::MyLookAndFeel() {
    typeface_ = juce::Typeface::createSystemTypefaceFor(BinaryData::quan_ttf, BinaryData::quan_ttfSize);
    setDefaultSansSerifTypeface(typeface_);
}

juce::Font MyLookAndFeel::getTextButtonFont(juce::TextButton& button, int buttonHeight) {
    std::ignore = button;
    auto op = juce::FontOptions{typeface_}.withHeight(std::max(12.0f, buttonHeight * 0.4f));
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
    return juce::Font{op};
}

juce::Font MyLookAndFeel::getLabelFont(juce::Label& label) {
    auto op = juce::FontOptions{typeface_}.withHeight(std::max(12.0f, label.getHeight() * 0.4f));
    return juce::Font{op};
}

juce::Font MyLookAndFeel::getPopupMenuFont() {
    auto op = juce::FontOptions{typeface_}.withHeight(12);
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

void MyLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                        float sliderPos, float minSliderPos, float maxSliderPos,
                        juce::Slider::SliderStyle style, juce::Slider& slider) 
{
    std::ignore = minSliderPos;
    std::ignore = maxSliderPos;
    
    auto b = juce::Rectangle{ x, y, width, height }.toFloat();
    if (style == juce::Slider::LinearVertical) {
        // 中间竖着的线
        auto track_width= b.getWidth() * 0.1f;
        track_width = std::max(track_width, 2.0f);
        auto track = juce::Rectangle{0.0f, 0.0f, track_width, b.getHeight()}.withCentre(b.getCentre());
        g.setColour(slider.findColour(juce::Slider::backgroundColourId));
        g.fillRect(track);
        g.setColour(slider.findColour(juce::Slider::ColourIds::trackColourId));
        g.drawRect(track);

        // 两边的线
        constexpr int nwires = 11;
        constexpr float longer_shirk = (1.0f - 0.45f);
        constexpr float shorter_shirk = (1.0f - 0.3f);
        float height_span = b.getHeight() / (nwires - 1.0f);
        float yy = b.getY();
        g.setColour(slider.findColour(juce::Slider::ColourIds::trackColourId));
        for (int i = 0; i < nwires; ++i) {
            int iyy = static_cast<int>(yy);
            if (i == 0 || i == (nwires - 1) || i == nwires / 2) {
                g.drawHorizontalLine(iyy, b.getX() + b.getWidth() * 0.5f * longer_shirk, track.getX());
                g.drawHorizontalLine(iyy, track.getRight(), b.getRight() - b.getWidth() * 0.5f * longer_shirk);
            }
            else {
                g.drawHorizontalLine(iyy, b.getX() + b.getWidth() * 0.5f * shorter_shirk, track.getX());
                g.drawHorizontalLine(iyy, track.getRight(), b.getRight() - b.getWidth() * 0.5f * shorter_shirk);
            }
            yy += height_span;
        }

        // 推子
        auto thumb_width = b.getWidth() * 0.4f;
        thumb_width = std::max(thumb_width, 2.0f);
        auto thumb_height = b.getHeight() * 0.15f;
        auto thumb = juce::Rectangle{b.getCentreX() - thumb_width * 0.5f, sliderPos - thumb_height * 0.5f, thumb_width, thumb_height};
        g.setColour(slider.findColour(juce::Slider::ColourIds::thumbColourId)
                                     .withAlpha(slider.isEnabled() ? 1.0f : 0.5f));
        g.fillRect(thumb);
        g.setColour(slider.findColour(juce::Slider::ColourIds::trackColourId));
        g.drawRect(thumb);
    }
    else if (style == juce::Slider::LinearHorizontal) {
        // 中间横着的线
        auto track_height= b.getHeight() * 0.1f;
        track_height = std::max(track_height, 2.0f);
        auto track = juce::Rectangle{0.0f, 0.0f, b.getWidth(), track_height}.withCentre(b.getCentre());
        g.setColour(slider.findColour(juce::Slider::backgroundColourId));
        g.fillRect(track);
        g.setColour(slider.findColour(juce::Slider::ColourIds::trackColourId));
        g.drawRect(track);

        // 两边的线
        constexpr int nwires = 11;
        constexpr float longer_shirk = (1.0f - 0.45f);
        constexpr float shorter_shirk = (1.0f - 0.3f);
        float width_span = b.getWidth() / (nwires - 1.0f);
        float xx = b.getX();
        g.setColour(slider.findColour(juce::Slider::ColourIds::trackColourId));
        for (int i = 0; i < nwires; ++i) {
            int ixx = static_cast<int>(xx);
            if (i == 0 || i == (nwires - 1) || i == nwires / 2) {
                g.drawVerticalLine(ixx, b.getY() + b.getHeight() * 0.5f * longer_shirk, track.getY());
                g.drawVerticalLine(ixx, track.getBottom(), b.getBottom() - b.getHeight() * 0.5f * longer_shirk);
            }
            else {
                g.drawVerticalLine(ixx, b.getY() + b.getHeight() * 0.5f * shorter_shirk, track.getY());
                g.drawVerticalLine(ixx, track.getBottom(), b.getBottom() - b.getHeight() * 0.5f * shorter_shirk);
            }
            xx += width_span;
        }

        // 推子
        auto thumb_height = b.getHeight() * 0.6f;
        thumb_height = std::max(thumb_height, 2.0f);
        auto thumb_width = b.getWidth() * 0.15f;
        auto thumb = juce::Rectangle{0.0f, 0.0f, thumb_width, thumb_height}.withCentre({sliderPos, b.getCentreY()});
        g.setColour(slider.findColour(juce::Slider::ColourIds::thumbColourId)
                                     .withAlpha(slider.isEnabled() ? 1.0f : 0.5f));
        g.fillRect(thumb);
        g.setColour(slider.findColour(juce::Slider::ColourIds::trackColourId));
        g.drawRect(thumb);
    }
}

}