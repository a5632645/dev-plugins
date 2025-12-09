#pragma once
#include <span>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace ui {

// #99a196 live9的浅绿色，亮度90
static juce::Colour const green_bg{153,161,150};
// #c1cac2 live9的下拉框何滑块背景
static juce::Colour const light_green_bg{193, 202, 194};
// #161820 live9的图表黑色背景
static juce::Colour const black_bg{22, 27, 32};
// #d64800 live9的图表，红色
static juce::Colour const line_fore{214, 72, 0};
// #cc5100 live9的旋钮颜色，红色
static juce::Colour const dial_fore{204, 81, 0};
// #c6cd00 live9的开关背景，黄绿色
static juce::Colour const active_bg{198, 205, 0};
// #778592 live9开关关闭背景，灰色
static juce::Colour const inactive_bg{119,133,146};
// #f5b126 live9的滑块前景，橙色
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

    void drawLinearSlider(
        juce::Graphics& g,
        int x, int y, int width, int height,
        float sliderPos, float minSliderPos, float maxSliderPos,
        juce::Slider::SliderStyle style, juce::Slider& s
    ) override {
        if (style == juce::Slider::SliderStyle::LinearBar) {
            g.fillAll(inactive_bg);
            juce::Rectangle<int> bound{x,y,width,height};
            g.setColour(orange_fore);
            g.fillRect(bound.removeFromLeft(static_cast<int>(sliderPos)));
            juce::Rectangle<int> bound2{x,y,width,height};
            g.setColour(juce::Colours::black);
            g.setColour(juce::Colours::black);
            g.drawRect(bound2);
            // g.drawText(s.getTextFromValue(s.getValue()), bound2, juce::Justification::centred);
        }
        else {
            juce::LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, s);
        }
    }

    // Slider textbox
    void drawLabel(juce::Graphics& g, juce::Label& label) override
    {
        g.setColour(black_bg);

        juce::String text = label.getText();
        int width = label.getWidth();
        int height = label.getHeight();
        // g.setFont(juce::Font{juce::FontOptions{static_cast<float>(height)}});
        g.setFont(label.getFont());
        g.drawFittedText(text, 0, 0, width, height, label.getJustificationType(), 1);
    }

    void drawComboBox(
        juce::Graphics& g,
        int width, int height,
        bool isButtonDown,
        int buttonX, int buttonY, int buttonW, int buttonH,
        juce::ComboBox& box) override {
        juce::ignoreUnused(width,height,isButtonDown);
        g.fillAll(box.isEnabled() ? ui::light_green_bg : ui::green_bg);
        g.setColour(juce::Colours::black);
        g.drawRect(g.getClipBounds());
        juce::Rectangle<int> b{buttonX, buttonY, buttonW, buttonH};
        g.drawText(juce::String::fromUTF8("▼"), b, juce::Justification::centred);
    }

    void positionComboBoxText(juce::ComboBox& box, juce::Label& label) override {
        label.setBounds(4, 1,
                        box.getWidth() - 30,
                        box.getHeight() - 2);

        label.setFont(getComboBoxFont (box));
    }

    void drawPopupMenuBackground(
        juce::Graphics& g,
        int width,
        int height
    ) override {
        juce::ignoreUnused(width,height);
        g.fillAll(ui::light_green_bg);
    }

    void drawMenuBarItem(
        juce::Graphics& g,
        int width, int height,
        int itemIndex, const juce::String& itemText,
        bool isMouseOverItem, bool isMenuOpen,
        bool /*isMouseOverBar*/, juce::MenuBarComponent& menuBar
    ) override {
        if (! menuBar.isEnabled())
        {
            g.setColour(ui::green_bg);
        }
        else if (isMenuOpen || isMouseOverItem)
        {
            g.fillAll(ui::light_green_bg);
            g.setColour(juce::Colours::black);
        }
        else
        {
            g.setColour (juce::Colours::black);
        }
        
        g.setFont (getMenuBarFont (menuBar, itemIndex, itemText));
        g.drawFittedText (itemText, 0, 0, width, height, juce::Justification::centredLeft, 1);
    }

    void drawPopupMenuItem(
        juce::Graphics& g, const juce::Rectangle<int>& area,
        const bool isSeparator, const bool isActive,
        const bool isHighlighted, const bool isTicked,
        const bool hasSubMenu, const juce::String& text,
        const juce::String& shortcutKeyText,
        const juce::Drawable* icon, const juce::Colour* const textColourToUse
    ) override {
        juce::ignoreUnused(isTicked,textColourToUse);
        if (isSeparator)
        {
            auto r  = area.reduced (5, 0);
            r.removeFromTop (juce::roundToInt (((float) r.getHeight() * 0.5f) - 0.5f));

            g.setColour (juce::Colours::black);
            g.fillRect (r.removeFromTop (1));
        }
        else
        {
            auto textColour = juce::Colours::black;

            auto r  = area.reduced (1);

            if (isHighlighted && isActive)
            {
                g.setColour (orange_fore);
                g.fillRect (r);

                g.setColour (findColour (juce::PopupMenu::highlightedTextColourId));
            }
            else
            {
                g.setColour (textColour.withMultipliedAlpha (isActive ? 1.0f : 0.5f));
            }

            r.reduce (juce::jmin (5, area.getWidth() / 20), 0);

            auto font = getPopupMenuFont();

            auto maxFontHeight = (float) r.getHeight() / 1.3f;

            if (font.getHeight() > maxFontHeight)
                font.setHeight (maxFontHeight);

            g.setFont (font);

            auto iconArea = r.removeFromLeft (juce::roundToInt (maxFontHeight)).toFloat();

            if (icon != nullptr)
            {
                icon->drawWithin (g, iconArea, juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize, 1.0f);
                r.removeFromLeft (juce::roundToInt (maxFontHeight * 0.5f));
            }

            if (hasSubMenu)
            {
                auto arrowH = 0.6f * getPopupMenuFont().getAscent();

                auto x = static_cast<float> (r.removeFromRight ((int) arrowH).getX());
                auto halfH = static_cast<float> (r.getCentreY());

                juce::Path path;
                path.startNewSubPath (x, halfH - arrowH * 0.5f);
                path.lineTo (x + arrowH * 0.6f, halfH);
                path.lineTo (x, halfH + arrowH * 0.5f);

                g.strokePath (path, juce::PathStrokeType (2.0f));
            }

            r.removeFromRight (3);
            g.setColour(juce::Colours::black);
            g.drawFittedText (text, r, juce::Justification::centredLeft, 1);

            if (shortcutKeyText.isNotEmpty())
            {
                auto f2 = font;
                f2.setHeight (f2.getHeight() * 0.75f);
                f2.setHorizontalScale (0.95f);
                g.setFont (f2);

                g.drawText (shortcutKeyText, r, juce::Justification::centredRight, true);
            }
        }
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

// ----------------------------------------
// flat dial
// ----------------------------------------
class Dial : public juce::Component {
public:
    Dial() : Dial("unname") {}

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

// ----------------------------------------
// flat slider
// ----------------------------------------
class FlatSlider : public juce::Component {
public:
    enum class TitleLayout {
        None,
        Left,
        Top
    };

    FlatSlider(juce::StringRef title = "unname", TitleLayout title_place = TitleLayout::Left)
        : slider_menu_(slider) {
        slider.setSliderStyle(juce::Slider::SliderStyle::LinearBar);
        slider.setLookAndFeel(&lookandfeel_);
        slider.addMouseListener(&slider_menu_, true);
        addAndMakeVisible(slider);

        label.setText(title, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centredLeft);
        label.setColour(juce::Label::ColourIds::textColourId, juce::Colours::black);
        addAndMakeVisible(label);

        SetTitleLayout(title_place);
    }

    ~FlatSlider() override {
        slider.setLookAndFeel(nullptr);
        attach_ = nullptr;
    }

    void resized() override {
        if (title_layout == TitleLayout::None) {
            slider.setBounds(getLocalBounds());
        }
        else if (title_layout == TitleLayout::Top) {
            auto b = getLocalBounds();
            label.setBounds(b.removeFromTop(static_cast<int>(label.getFont().getHeight())));
            slider.setBounds(b);
        }
        else {
            auto font = label.getFont();
            auto width = 1.2f * juce::TextLayout::getStringWidth(font, label.getText());
            auto b = getLocalBounds();
            label.setBounds(b.removeFromLeft(static_cast<int>(width)));
            slider.setBounds(b);
        }
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

    void SetTitleLayout(TitleLayout layout) {
        if (title_layout == layout) return;
        title_layout = layout;
        label.setVisible(layout != TitleLayout::None);
        resized();
    }

    juce::Slider slider;
    juce::Label label;
private:
    TitleLayout title_layout{TitleLayout::Top};
    std::unique_ptr<juce::SliderParameterAttachment> attach_;
    SliderMenu slider_menu_;
    CustomLookAndFeel lookandfeel_;
};

// ----------------------------------------
// flat selector
// ----------------------------------------
class CubeSelector : public juce::Component {
public:
    class Cube : public juce::Component {
    public:
        Cube(CubeSelector& parent) : parent_(parent) {}
        void paint(juce::Graphics& g) override {
            if (parent_.selecting_ == this) {
                g.fillAll(ui::active_bg);
            }
            else {
                g.fillAll(ui::inactive_bg);
            }

            g.setColour(ui::black_bg);
            g.drawText(title, getLocalBounds(), juce::Justification::centred);

            g.setColour(juce::Colours::black);
            g.drawRect(getLocalBounds());
        } 
        
        juce::String title;
    private:
        CubeSelector& parent_;
    };

    ~CubeSelector() override {
        attach_ = nullptr;
        cubes_.clear();
    }

    std::span<std::unique_ptr<Cube>> BindParam(juce::AudioProcessorValueTreeState& apvts, juce::StringRef id, bool add_choices) {
        auto* param = apvts.getParameter(id);
        return BindParam(static_cast<juce::AudioParameterChoice*>(param), add_choices);
    }

    std::span<std::unique_ptr<Cube>> BindParam(juce::AudioParameterChoice* param, bool add_choices) {
        jassert(param != nullptr);
        attach_ = std::make_unique<juce::ParameterAttachment>(
            *param, [this](float v) {
                OnValueChanged(v);
            }
        );

        if (!add_choices) {
            return {};
        }

        size_t idx_begin = cubes_.size();
        auto const& choices = param->choices;
        for (auto const& str : choices) {
            auto& cube_item = cubes_.emplace_back(std::make_unique<Cube>(*this));
            cube_item->title = str;
            cube_item->addMouseListener(this, true);
            addAndMakeVisible(*cube_item);
        }

        attach_->sendInitialUpdate();

        return std::span{cubes_}.subspan(idx_begin);
    }

    void AddCube(juce::StringRef title) {
        auto& cube_item = cubes_.emplace_back(std::make_unique<Cube>(*this));
        cube_item->title = title;
        cube_item->addMouseListener(this, true);
        addAndMakeVisible(*cube_item);
    }

    Cube& GetCube(size_t idx) noexcept {
        return *cubes_[idx];
    }

    std::vector<std::unique_ptr<Cube>>& GetAllCubes() noexcept {
        return cubes_;
    }
private:
    void OnValueChanged(float v) {
        size_t idx = static_cast<size_t>(v);
        auto old_one = selecting_;
        selecting_ = cubes_[idx].get();
        if (old_one) old_one->repaint();
        selecting_->repaint();
    }

    void mouseDown(juce::MouseEvent const& e) override {
        if (e.originalComponent == this) return;

        for (size_t i = 0; i < cubes_.size(); ++i) {
            if (e.originalComponent == cubes_[i].get()) {
                attach_->setValueAsCompleteGesture(static_cast<float>(i));
                break;
            }
        }
    }

    std::unique_ptr<juce::ParameterAttachment> attach_;
    Cube* selecting_{};
    std::vector<std::unique_ptr<Cube>> cubes_;
};

// ----------------------------------------
// flat toggle box
// ----------------------------------------
class Switch : public juce::ToggleButton {
public:
    Switch() {
        on_text_ = "on";
        off_text_ = "off";
    }

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
        BindParam(param);
    }

    void BindParam(juce::RangedAudioParameter* param) {
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

// ----------------------------------------
// flat button
// ----------------------------------------
class FlatButton : public juce::TextButton {
public:
    FlatButton() {}

    FlatButton(juce::StringRef text) {
        setButtonText(text);
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(isDown() || isMouseOver() ? active_bg : inactive_bg);
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
        g.setColour(juce::Colours::black);
        g.drawText(getButtonText(), b, juce::Justification::centred);
    }
};

// ----------------------------------------
// flat combobox
// ----------------------------------------
class FlatCombobox : public juce::ComboBox {
public:
    FlatCombobox() {
        setLookAndFeel(&look_);
        setJustificationType(juce::Justification::centredLeft);
    }

    ~FlatCombobox() override {
        attach_ = nullptr;
        setLookAndFeel(nullptr);
    }

    void BindParam(juce::AudioProcessorValueTreeState& apvts, juce::StringRef id) {
        auto* param = apvts.getParameter(id);
        BindParam(static_cast<juce::AudioParameterChoice*>(param));
    }

    void BindParam(juce::AudioParameterChoice* param) {
        jassert(param != nullptr);

        clear();
        addItemList(param->choices, 1);
        // combobox_.setSelectedItemIndex(static_cast<juce::AudioParameterChoice*>(param)->getIndex());
        attach_ = std::make_unique<juce::ComboBoxParameterAttachment>(*param, *this);
    }
private:
    std::unique_ptr<juce::ComboBoxParameterAttachment> attach_;
    CustomLookAndFeel look_;
};

[[maybe_unused]]
static void SetLableBlack(juce::Label& lable) {
    lable.setColour(juce::Label::ColourIds::textColourId, black_bg);
}

}
