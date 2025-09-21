#include "PluginEditor.h"
#include "PluginProcessor.h"

SteepFlangerAudioProcessorEditor::SteepFlangerAudioProcessorEditor (SteepFlangerAudioProcessor& p)
    : AudioProcessorEditor (&p)
{
    auto& apvts = *p.value_tree_;

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

    setSize (575, 550);
}

SteepFlangerAudioProcessorEditor::~SteepFlangerAudioProcessorEditor() {
}

//==============================================================================
void SteepFlangerAudioProcessorEditor::paint (juce::Graphics& g) {
    g.fillAll(findColour(juce::ResizableWindow::backgroundColourId));
}

void SteepFlangerAudioProcessorEditor::resized() {
    auto b = getLocalBounds();
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
        b.removeFromLeft(8);
        {
            auto fir_block = topblock.removeFromLeft(80 * 4);
            {
                auto fir_title = fir_block.removeFromTop(25);
                minum_phase_.setBounds(fir_title.removeFromRight(200));
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
    }
}
