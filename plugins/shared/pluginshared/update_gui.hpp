#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "update_data.hpp"
#include "component.hpp"

namespace pluginshared {
class UpdateMessageDialog : public juce::DialogWindow, private juce::Timer {
public:
    UpdateMessageDialog(UpdateData& data)
        : juce::DialogWindow("checking update", juce::Colours::black, false)
        , update_data_(data)
    {
        setUsingNativeTitleBar(true);
        setContentNonOwned(&update_message_component_, true);
        setTopLeftPosition(juce::Desktop::getInstance().getMousePosition());

        update_message_component_.button.onClick = [&]() { userTriedToCloseWindow(); };

        startTimerHz(15);
    }
private:
    void timerCallback() override {
        if (update_data_.IsComplete()) {
            update_message_component_.button.setButtonText(update_data_.GetButtonLabel());
            update_message_component_.label.setText(update_data_.GetUpdateMessage());
            stopTimer();
        }
    }

    void closeButtonPressed() override {
        stopTimer();
        exitModalState();
    }

    class UpdateMessageComponent : public juce::Component {
    public:
        UpdateMessageComponent() {
            label.setReadOnly(true);
            label.setColour(juce::TextEditor::ColourIds::backgroundColourId, ui::green_bg);
            label.setColour(juce::TextEditor::ColourIds::textColourId, ui::black_bg);
            label.setText("checking update");
            addAndMakeVisible(label);
            button.setButtonText("cancel");
            addAndMakeVisible(button);
            setSize(500, 300);
        }

        void resized() override {
            auto b = getLocalBounds();
            button.setBounds(b.removeFromBottom(30)
                            .withSizeKeepingCentre(100, 30)
                            .reduced(2));
            label.setBounds(b);
        }

        void paint(juce::Graphics& g) override {
            g.fillAll(ui::green_bg);
        }

        juce::TextEditor label;
        ui::FlatButton button;
    };

    UpdateData& update_data_;
    UpdateMessageComponent update_message_component_;
};
} // namespace pluginshared
