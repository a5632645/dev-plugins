#include "PluginEditor.h"
#include "PluginProcessor.h"

SteepFlangerAudioProcessorEditor::SteepFlangerAudioProcessorEditor (SteepFlangerAudioProcessor& p)
    : AudioProcessorEditor (&p)
{
    auto& apvts = *p.value_tree_;

    plugin_title_.setText("SteepFlanger Prototype by Mana", juce::dontSendNotification);
    plugin_title_.setFont(juce::Font{28});
    addAndMakeVisible(plugin_title_);

    delay_.BindParam(apvts, "delay");
    addAndMakeVisible(delay_);
    depth_.BindParam(apvts, "depth");
    addAndMakeVisible(depth_);
    speed_.BindParam(apvts, "speed");
    addAndMakeVisible(speed_);
    phase_.BindParam(apvts, "phase");
    addAndMakeVisible(phase_);
    addAndMakeVisible(lfo_title_);

    cutoff_.BindParam(apvts, "cutoff");
    addAndMakeVisible(cutoff_);
    coeff_len_.BindParam(apvts, "coeff_len");
    addAndMakeVisible(coeff_len_);
    side_lobe_.BindParam(apvts, "side_lobe");
    addAndMakeVisible(side_lobe_);
    minum_phase_.BindParam(apvts, "minum_phase");
    addAndMakeVisible(minum_phase_);
    addAndMakeVisible(fir_title_);
    highpass_.BindParam(apvts, "highpass");
    addAndMakeVisible(highpass_);

    fb_enable_.BindParam(apvts, "fb_enable");
    addAndMakeVisible(fb_enable_);
    fb_value_.BindParam(apvts, "fb_value");
    addAndMakeVisible(fb_value_);
    panic_.setButtonText("panic");
    panic_.onClick = [this, &p] {
        fb_enable_.setToggleState(false, juce::NotificationType::sendNotificationSync);
        p.Panic();
    };
    addAndMakeVisible(panic_);
    fb_damp_.BindParam(apvts, "fb_damp");
    addAndMakeVisible(fb_damp_);
    addAndMakeVisible(feedback_title_);

    addAndMakeVisible(barber_title_);
    barber_phase_.BindParam(apvts, "barber_phase");
    addAndMakeVisible(barber_phase_);
    barber_speed_.BindParam(apvts, "barber_speed");
    addAndMakeVisible(barber_speed_);
    barber_enable_.BindParam(apvts, "barber_enable");
    addAndMakeVisible(barber_enable_);

    setSize (600, 292);
}

SteepFlangerAudioProcessorEditor::~SteepFlangerAudioProcessorEditor() {
}

//==============================================================================
void SteepFlangerAudioProcessorEditor::paint (juce::Graphics& g) {
    g.fillAll(juce::Colour{22,27,32});

    auto b = getLocalBounds();
    {
        auto title_block = b.removeFromTop(plugin_title_.getFont().getHeight());
        g.setColour(juce::Colour{193,193,166});
        g.fillRect(title_block);
    }
    juce::Colour bg{139,148,135};
    g.setColour(bg);
    {
        auto topblock = b.removeFromTop(125);
        {
            auto lfo_block = topblock.removeFromLeft(80 * 4);
            g.fillRect(lfo_block);
        }
        topblock.removeFromLeft(8);
        {
            auto fir_block = topblock.removeFromLeft(80 * 4);
            g.fillRect(fir_block);
        }
    }
    b.removeFromTop(8);
    {
        auto bottom_block = b.removeFromTop(125);
        {
            auto feedback_block = bottom_block.removeFromLeft(80 * 3);
            g.fillRect(feedback_block);
        }
        bottom_block.removeFromLeft(8);
        {
            g.fillRect(bottom_block);
        }
    }
}

void SteepFlangerAudioProcessorEditor::resized() {
    auto b = getLocalBounds();
    {
        plugin_title_.setBounds(b.removeFromTop(plugin_title_.getFont().getHeight()));
    }
    {
        auto topblock = b.removeFromTop(125);
        {
            auto lfo_block = topblock.removeFromLeft(80 * 4);
            lfo_title_.setBounds(lfo_block.removeFromTop(25));
            delay_.setBounds(lfo_block.removeFromLeft(80));;
            depth_.setBounds(lfo_block.removeFromLeft(80));
            speed_.setBounds(lfo_block.removeFromLeft(80));
            phase_.setBounds(lfo_block.removeFromLeft(80));
        }
        topblock.removeFromLeft(8);
        {
            auto fir_block = topblock.removeFromLeft(80 * 4);
            {
                auto fir_title = fir_block.removeFromTop(25);
                minum_phase_.setBounds(fir_title.removeFromRight(100).reduced(4, 0));
                highpass_.setBounds(fir_title.removeFromRight(100).reduced(4, 0));
                fir_title_.setBounds(fir_title);
            }
            cutoff_.setBounds(fir_block.removeFromLeft(80));;
            coeff_len_.setBounds(fir_block.removeFromLeft(80));;
            side_lobe_.setBounds(fir_block.removeFromLeft(80));;
        }
    }
    b.removeFromTop(8);
    {
        auto bottom_block = b.removeFromTop(125);
        {
            auto feedback_block = bottom_block.removeFromLeft(80 * 3);
            feedback_title_.setBounds(feedback_block.removeFromTop(25));
            {
                auto button_block = feedback_block.removeFromLeft(80);
                button_block.removeFromTop(8);
                fb_enable_.setBounds(button_block.removeFromTop(30));
                button_block.removeFromBottom(8);
                panic_.setBounds(button_block.removeFromBottom(30));
            }
            fb_value_.setBounds(feedback_block.removeFromLeft(80));
            fb_damp_.setBounds(feedback_block.removeFromLeft(80));
        }
        bottom_block.removeFromLeft(8);
        {
            barber_title_.setBounds(bottom_block.removeFromTop(25));
            barber_phase_.setBounds(bottom_block.removeFromLeft(80));
            barber_speed_.setBounds(bottom_block.removeFromLeft(80));
            auto e = bottom_block.removeFromLeft(100);
            barber_enable_.setBounds(e.withSizeKeepingCentre(e.getWidth(), 25));
        }
    }
}
