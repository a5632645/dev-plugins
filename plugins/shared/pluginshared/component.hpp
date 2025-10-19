#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace ui {

// #99a196 ableton live9的浅绿色，亮度90
static juce::Colour const green_bg{153,161,150};
// #161820 ableton live9的图表黑色背景
static juce::Colour const black_bg{22, 27, 32};
// #d64800 ableton live9的图表，红色
static juce::Colour const line_fore{214, 72, 0};
// #cc5100 ableton live9的旋钮颜色，红色
static juce::Colour const dial_fore{204, 81, 0};
// #c6cd00 ableton live9的开关背景，黄绿色
static juce::Colour const active_bg{198, 205, 0};
// #778592 ableton live9开关关闭背景，灰色
static juce::Colour const inactive_bg{119,133,146};
// #f5b126 ableton live9的滑块前景，橙色
static juce::Colour const orange_fore{0xf5,0xb1,0x26};

// ---------------------------------------- dial ----------------------------------------

class CustomLookAndFeel : public juce::LookAndFeel_V4 {
public:
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
        const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider&) override
    {
        // Radius of knob
        auto radius = juce::jmin(static_cast<float>(width) / 2.0f, static_cast<float>(height) / 2.0f) - 3.0f;
        // Centre point (centreX, centreY) of knob
        auto centreX = static_cast<float>(x) + static_cast<float>(width) * 0.5f;
        auto centreY = static_cast<float>(y) + static_cast<float>(height) * 0.5f + 3.0f;

        auto thickness = std::max(3.0f, 0.1f * radius);

        // current angle of the slider
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // Draw path of the slider backgound (in darker background colour)
        juce::Path backgroundArc;
        backgroundArc.addCentredArc(centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(black_bg);
        g.strokePath(backgroundArc, juce::PathStrokeType(thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        // Draw path of slider foreground (in white)
        juce::Path foregroundArc;
        foregroundArc.addCentredArc(centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, angle, true);
        g.setColour(dial_fore);
        g.strokePath(foregroundArc, juce::PathStrokeType(thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Pointer
        juce::Path p;
        auto pointerLength = radius * 1.f;
        auto pointerThickness = thickness;
        p.addRectangle(-pointerThickness * 0.5f, -radius, pointerThickness, pointerLength);
        p.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
        g.setColour(black_bg);
        g.fillPath(p);
    }

    // Slider textbox
    void drawLabel(juce::Graphics& g, juce::Label& label) override
    {
        g.setColour(black_bg);

        juce::String text = label.getText();
        int width = label.getWidth();
        int height = label.getHeight();
        g.setFont(juce::Font{juce::FontOptions{static_cast<float>(height) - 2.0f}});
        g.drawFittedText(text, 0, 0, width, height, juce::Justification::centred, 1);
    }
};

class SliderMenu : public juce::Slider::MouseListener {
public:
    SliderMenu(juce::Slider& slider)
        : slider_(slider)
    {}

    void mouseDown(const juce::MouseEvent& e) override {
        std::ignore = e;
        juce::ModifierKeys keys = juce::ModifierKeys::getCurrentModifiers();
        if (keys.isPopupMenu()) {
            menu_.clear();
            menu_.addItem("enter",
            [ptr_component = this]{
                auto* editor = new juce::TextEditor;
                editor->setText(ptr_component->slider_.getTextFromValue(ptr_component->slider_.getValue()));

                juce::DialogWindow::LaunchOptions op;
                op.dialogTitle = "enter";
                op.escapeKeyTriggersCloseButton = true;
                op.useNativeTitleBar = true;
                op.resizable = false;
                op.content = {editor, true};

                auto* dialog = op.create();
                editor->onReturnKey = [&slider = ptr_component->slider_, dialog, editor] {
                    slider.setValue(slider.getValueFromText(editor->getText()),
                                juce::NotificationType::sendNotificationSync);
                    dialog->userTriedToCloseWindow();
                };
                dialog->enterModalState(true, nullptr, true);
                juce::MessageManager::callAsync([editor]{
                    editor->grabKeyboardFocus();
                    editor->selectAll();
                });
            });

            menu_.addItem("reset",
            [this] {
                slider_.setValue(slider_.getDoubleClickReturnValue());
            });

            if (on_menu_showup) {
                on_menu_showup(menu_);
            }

            juce::PopupMenu::Options op;
            menu_.showMenuAsync(op.withMousePosition());
        }
    }

    std::function<void(juce::PopupMenu&)> on_menu_showup;
private:
    juce::Slider& slider_;
    juce::PopupMenu menu_;
};

class Dial : public juce::Component {
public:
    Dial(juce::StringRef title)
        : slider_menu_(slider) {
        slider.setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag);
        slider.setLookAndFeel(&lookandfeel_);
        slider.addMouseListener(&slider_menu_, true);
        addAndMakeVisible(slider);

        label.setText(title, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centredBottom);
        label.setColour(juce::Label::ColourIds::textColourId, juce::Colours::black);
        addAndMakeVisible(label);
    }

    ~Dial() override {
        slider.setLookAndFeel(nullptr);
        attach_ = nullptr;
    }

    void resized() override {
        auto b = getLocalBounds();
        auto title_h = std::max(16.0f, 0.15f * static_cast<float>(getHeight()));
        auto top = b.removeFromTop(static_cast<int>(title_h));
        label.setFont(juce::Font{juce::FontOptions{title_h}});
        label.setBounds(top);
        slider.setBounds(b);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, true,
            static_cast<int>(static_cast<float>(getWidth()) * 0.9f), static_cast<int>(title_h));
    }

    void BindParam(juce::AudioProcessorValueTreeState& apvts, juce::StringRef id) {
        BindParam(apvts.getParameter(id));
    }

    void BindParam(juce::RangedAudioParameter* param) {
        jassert(param != nullptr);
        attach_ = std::make_unique<juce::SliderParameterAttachment>(
            *param, slider
        );
    }

    std::function<void(juce::PopupMenu&)>& OnMenuShowup() {
        return slider_menu_.on_menu_showup;
    }

    juce::Slider slider;
    juce::Label label; // dial title
private:
std::unique_ptr<juce::SliderParameterAttachment> attach_;
    SliderMenu slider_menu_;
    CustomLookAndFeel lookandfeel_;
};

// ---------------------------------------- cube selector ----------------------------------------

class CubeSelector : public juce::Component {
public:
    CubeSelector(juce::StringRef title_str) {
        title.setText(title_str, juce::dontSendNotification);
        addAndMakeVisible(title);
    }

    ~CubeSelector() override {
        attach_ = nullptr;
    }

    void BindParam(juce::AudioProcessorValueTreeState& apvts, juce::StringRef id) {
        auto* param = static_cast<juce::AudioParameterChoice*>(apvts.getParameter(id));
        jassert(param != nullptr);

        attach_ = std::make_unique<juce::ComboBoxParameterAttachment>(
            *param, combobox_
        );
        combobox_.addItemList(param->choices, 1);
    }

    void mouseDown(const juce::MouseEvent& e) override {
        auto b = getLocalBounds().toFloat();
        b.removeFromTop(title.getLocalBounds().toFloat().getHeight());

        auto const num_choices = combobox_.getNumItems();
        float const width = getLocalBounds().toFloat().getWidth() / static_cast<float>(num_choices);
        int choice = static_cast<int>(static_cast<float>(e.getMouseDownX()) / width);
        choice = std::clamp(choice, 0, num_choices - 1);
        if (choice != combobox_.getSelectedItemIndex()) {
            combobox_.setSelectedItemIndex(choice);
            repaint();
        }
    }

    void paint(juce::Graphics& g) override {
        auto b = getLocalBounds().toFloat();
        b.removeFromTop(title.getLocalBounds().toFloat().getHeight());
        auto const num_choices = combobox_.getNumItems();
        int const select_idx = combobox_.getSelectedItemIndex();
        float const width = getLocalBounds().toFloat().getWidth() / static_cast<float>(num_choices);
        auto cube = b.removeFromLeft(width);
        for (int i = 0; i < num_choices; ++i) {
            b = cube;
            b.reduce(2, 4);

            auto color = select_idx == i ? juce::Colour{198,205,0} : juce::Colour{119,133,146};
            g.setColour(color);
            g.fillRect(b);
            g.setColour(juce::Colours::black);
            g.drawRect(b);
            g.drawText(combobox_.getItemText(i), b, juce::Justification::centred);

            cube.translate(width, 0);
        }
    }

    void resized() override {
        auto b = getLocalBounds();
        title.setBounds(b.removeFromTop(static_cast<int>(title.getFont().getHeight())));
    }

    juce::Label title;
private:
    juce::ComboBox combobox_;
    std::unique_ptr<juce::ComboBoxParameterAttachment> attach_;
};

// ---------------------------------------- switch ----------------------------------------

class Switch : public juce::ToggleButton {
public:
    Switch(juce::StringRef text) {
        on_text_ = text;
        off_text_ = text;
    }

    Switch(juce::StringRef on_text, juce::StringRef off_text) {
        on_text_ = on_text;
        off_text_ = off_text;
    }

    ~Switch() override {
        attach_ = nullptr;
    }

    void BindParam(juce::AudioProcessorValueTreeState& apvts, juce::StringRef id) {
        auto* param = apvts.getParameter(id);
        jassert(param != nullptr);

        attach_ = std::make_unique<juce::ButtonParameterAttachment>(
            *param, *this
        );
    }

    void paint(juce::Graphics& g) override {
        auto color = getToggleState() ? juce::Colour{198,205,0} : juce::Colour{119,133,146};
        auto b = getLocalBounds();
        g.setColour(color);
        g.fillRect(b);
        g.setColour(juce::Colours::black);
        g.drawRect(b);
        g.drawText(getToggleState() ? on_text_: off_text_, b, juce::Justification::centred);
    }
private:
    juce::String on_text_;
    juce::String off_text_;
    std::unique_ptr<juce::ButtonParameterAttachment> attach_;
};

// ---------------------------------------- flat button ----------------------------------------
class FlatButton : public juce::TextButton {
public:
    void paint(juce::Graphics& g) override {
        g.fillAll(line_fore);
        auto b = getLocalBounds();
        auto const fb = b.toFloat();
        g.setColour(black_bg);
        if (isDown()) {
            g.drawHorizontalLine(0, 0, fb.getRight());
            g.drawHorizontalLine(1, 0, fb.getRight());
            g.drawVerticalLine(0, 0, fb.getBottom());
            g.drawVerticalLine(1, 0, fb.getBottom());
        }
        else {
            g.drawRect(b);
        }
        g.setColour(juce::Colours::white);
        g.drawText(getButtonText(), b, juce::Justification::centred);
    }
};

[[maybe_unused]]
static void SetLableBlack(juce::Label& lable) {
    lable.setColour(juce::Label::ColourIds::textColourId, black_bg);
}

}
