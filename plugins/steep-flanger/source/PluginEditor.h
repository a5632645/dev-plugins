#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "qwqdsp/spectral/real_fft.hpp"

#include "shared.hpp"

// ---------------------------------------- dial ----------------------------------------

class CustomLookAndFeel : public juce::LookAndFeel_V4 {
public:
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
        const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider&) override
    {
        // Radius of knob
        auto radius = juce::jmin(width / 2, height / 2) - 5.0f;
        // Centre point (centreX, centreY) of knob
        auto centreX = x + width * 0.5f;
        auto centreY = y + radius + 12.f;

        // current angle of the slider
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // Draw path of the slider backgound (in darker background colour)
        juce::Path backgroundArc;
        backgroundArc.addCentredArc(centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        juce::Colour myColour = findColour(0x1005700);
        myColour = myColour.darker(.8f);
        g.setColour(myColour);
        g.strokePath(backgroundArc, juce::PathStrokeType(3.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        // Draw path of slider foreground (in white)
        juce::Path foregroundArc;
        foregroundArc.addCentredArc(centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, angle, true);
        g.setColour(juce::Colours::white);
        g.strokePath(foregroundArc, juce::PathStrokeType(3.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Pointer
        juce::Path p;
        auto pointerLength = radius * 1.f;
        auto pointerThickness = 3.0f;
        p.addRectangle(-pointerThickness * 0.5f, -radius, pointerThickness, pointerLength);
        p.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
        g.setColour(juce::Colours::white);
        g.fillPath(p);
    }

    // Slider textbox
    void drawLabel(juce::Graphics& g, juce::Label& label) override
    {
        g.setColour(juce::Colours::white);

        juce::String text = label.getText();
        int width = label.getWidth();
        int height = label.getHeight();
        g.setFont(juce::Font(height - 2.f, juce::Font::plain));
        g.drawFittedText(text, 0, 0, width, height, juce::Justification::centred, 1);
    }
};

class SliderMenu : public juce::Slider::MouseListener {
public:
    SliderMenu(juce::Slider& slider)
        : slider_(slider)
    {}

    void mouseDown(const juce::MouseEvent& e) override {
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
                    dialog->closeButtonPressed();
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

            juce::PopupMenu::Options op;
            menu_.showMenuAsync(op.withMousePosition());
        }
    }
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
        addAndMakeVisible(label);
    }

    ~Dial() override {
        slider.setLookAndFeel(nullptr);
        attach_ = nullptr;
    }

    void resized() override {
        auto b = getLocalBounds();
        b.removeFromTop(label.getFont().getHeight());
        slider.setBounds(b);
        label.attachToComponent(&slider, false);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, getWidth() * 0.9f, std::max(getHeight() * 0.15f, 14.0f));
    }

    void BindParam(juce::AudioProcessorValueTreeState& apvts, juce::StringRef id) {
        auto* param = apvts.getParameter(id);
        jassert(param != nullptr);

        attach_ = std::make_unique<juce::SliderParameterAttachment>(
            *param, slider
        );
    }

    juce::Slider slider;
    juce::Label label;
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
        b.removeFromTop(title.getHeight());

        auto const num_choices = combobox_.getNumItems();
        float const width = getWidth() / static_cast<float>(num_choices);
        int choice = e.getMouseDownX() / width;
        choice = std::clamp(choice, 0, num_choices - 1);
        if (choice != combobox_.getSelectedItemIndex()) {
            combobox_.setSelectedItemIndex(choice);
            repaint();
        }
    }

    void paint(juce::Graphics& g) override {
        auto b = getLocalBounds().toFloat();
        b.removeFromTop(title.getHeight());
        auto const num_choices = combobox_.getNumItems();
        int const select_idx = combobox_.getSelectedItemIndex();
        float const width = getWidth() / static_cast<float>(num_choices);
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
        title.setBounds(b.removeFromTop(title.getFont().getHeight()));
    }

    juce::Label title;
private:
    juce::ComboBox combobox_;
    std::unique_ptr<juce::ComboBoxParameterAttachment> attach_;
};

// ---------------------------------------- on/off ----------------------------------------

class Switch : public juce::ToggleButton {
public:
    Switch(juce::StringRef title) {
        setName(title);
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
        g.drawText(getName(), b, juce::Justification::centred);
    }
private:
    std::unique_ptr<juce::ButtonParameterAttachment> attach_;
};

class SteepFlangerAudioProcessor;

// ---------------------------------------- time prev ----------------------------------------
class TimeView : public juce::Component {
public:
    TimeView(SteepFlangerAudioProcessor& p)
        : p_(p)
    {
        addAndMakeVisible(title_);
        reload_.onClick = [this] {
            SendCoeffs();
        };
        reload_.setButtonText("reload");
        addAndMakeVisible(reload_);
    }

    void paint(juce::Graphics& g) override;

    void mouseDown(const juce::MouseEvent& e) override {
        mouseDrag(e);
    }

    void mouseDrag(const juce::MouseEvent& e) override;

    void mouseUp(const juce::MouseEvent& e) override;

    void SendCoeffs();

    void resized() override {
        auto b = getLocalBounds();
        auto top = b.removeFromTop(title_.getFont().getHeight() * 1.5f);
        reload_.setBounds(top.removeFromRight(80));
        title_.setBounds(top);
    }

    void UpdateGui();

private:
    SteepFlangerAudioProcessor& p_;
    juce::Label title_{"", "Time view"};
    juce::TextButton reload_;
    std::array<float, kMaxCoeffLen + 1> coeff_buffer_{};
    std::array<float, kMaxCoeffLen + 1> custom_coeff_{};

    friend class SpectralView;
};

class SpectralView : public juce::Component {
public:
    SpectralView(TimeView& time)
        : time_(time)
    {
        addAndMakeVisible(title_);
        fft_.Init(kGainFFTSize);
    }

    void paint(juce::Graphics& g) override;

    void resized() override {
        auto b = getLocalBounds();
        title_.setBounds(b.removeFromTop(title_.getFont().getHeight()));
    }

    void UpdateGui();

private:
    static constexpr size_t kGainFFTSize = 1024;
    static constexpr size_t kGainNumBins = qwqdsp::spectral::RealFFT::NumBins(kGainFFTSize);

    TimeView& time_;
    juce::Label title_{"", "Responce"};
    std::array<float, kGainNumBins> gains_{};
    float max_db_{};
    float min_db_{};
    qwqdsp::spectral::RealFFT fft_;
};

// ---------------------------------------- editor ----------------------------------------

//==============================================================================
class SteepFlangerAudioProcessorEditor final : public juce::AudioProcessorEditor {
public:
    explicit SteepFlangerAudioProcessorEditor (SteepFlangerAudioProcessor&);
    ~SteepFlangerAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    void UpdateGui() {
        timeview_.UpdateGui();
        spectralview_.UpdateGui();
    }

    void UpdateGuiFromTimeView() {
        spectralview_.UpdateGui();
    }
private:
    SteepFlangerAudioProcessor& p_;

    juce::Label plugin_title_;

    juce::Label lfo_title_{"lfo", "lfo"};
    Dial delay_{"delay"};
    Dial depth_{"depth"};
    Dial speed_{"speed"};
    Dial phase_{"phase"};

    juce::Label fir_title_{"fir", "fir"};
    Dial cutoff_{"cutoff"};
    Dial coeff_len_{"steep"};
    Dial side_lobe_{"side_lobe"};
    Switch minum_phase_{"minum_phase"};
    Switch highpass_{"highpass"};
    Switch custom_{"custom"};

    juce::Label feedback_title_{"feedback", "feedback"};
    Switch fb_enable_{"feedback"};
    Dial fb_value_{"feedback"};
    Dial fb_damp_{"damp"};
    juce::TextButton panic_;

    juce::Label barber_title_{"barberpole", "barberpole"};
    Switch barber_enable_{"barberpole"};
    Dial barber_phase_{"phase"};
    Dial barber_speed_{"speed"};

    TimeView timeview_;
    SpectralView spectralview_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SteepFlangerAudioProcessorEditor)
};
