#include "plugin_ui.hpp"
#include "../PluginProcessor.h"
#include "../PluginEditor.h"

PluginUi::PluginUi(SteepFlangerAudioProcessor& p)
    : p_(p)
    , preset_panel_(*p.preset_manager_)
    , timeview_(p)
    , spectralview_(timeview_, p) {
    auto& apvts = *p.value_tree_;

    preset_panel_.SetDspInstName(p.dsp_processor_.name);
    addAndMakeVisible(preset_panel_);

    addAndMakeVisible(lfo_title_);
    delay_.BindParam(apvts, "delay");
    addAndMakeVisible(delay_);
    depth_.BindParam(apvts, "depth");
    addAndMakeVisible(depth_);
    speed_.BindParam(p.delay_lfo_state_);
    addAndMakeVisible(speed_);
    phase_.BindParam(apvts, "phase");
    addAndMakeVisible(phase_);
    drywet_.BindParam(p.param_drywet_);
    addAndMakeVisible(drywet_);

    cutoff_.BindParam(apvts, "cutoff");
    cutoff_.slider.onValueChange = [this] {
        spectralview_.DrawIirResponce();
    };
    addAndMakeVisible(cutoff_);
    coeff_len_.BindParam(apvts, "coeff_len");
    addAndMakeVisible(coeff_len_);
    side_lobe_.BindParam(apvts, "side_lobe");
    addAndMakeVisible(side_lobe_);
    minum_phase_.BindParam(apvts, "minum_phase");
    addAndMakeVisible(minum_phase_);
    iir_mode_.onClick = [this] { SetIirMode(iir_mode_.getToggleState()); };
    iir_mode_.BindParam(p.param_iir_mode_);
    addAndMakeVisible(iir_mode_);
    highpass_.BindParam(apvts, "highpass");
    highpass_.onClick = [this] {
        spectralview_.DrawIirResponce();
    };
    addAndMakeVisible(highpass_);

    fb_value_.BindParam(apvts, "fb_value");
    addAndMakeVisible(fb_value_);
    panic_.setButtonText("panic");
    panic_.onClick = [&p] {
        p.reset();
    };
    addAndMakeVisible(panic_);
    fb_damp_.BindParam(apvts, "fb_damp");
    addAndMakeVisible(fb_damp_);
    addAndMakeVisible(feedback_title_);

    addAndMakeVisible(barber_title_);
    barber_phase_.BindParam(apvts, "barber_phase");
    addAndMakeVisible(barber_phase_);
    barber_speed_.BindParam(p.barber_lfo_state_);
    addAndMakeVisible(barber_speed_);
    barber_enable_.BindParam(apvts, "barber_enable");
    addAndMakeVisible(barber_enable_);
    barber_stereo_.BindParam(p.param_barber_stereo_);
    addAndMakeVisible(barber_stereo_);

    addAndMakeVisible(timeview_);
    addAndMakeVisible(spectralview_);

    clear_.onClick = [this] {
        timeview_.ClearCustomCoeffs();
    };
    addAndMakeVisible(clear_);
    copy_.onClick = [this] {
        timeview_.CopyCoeffesToCustom();
    };
    addAndMakeVisible(copy_);
    reload_.onClick = [this] {
        timeview_.SendCoeffs();
    };
    addAndMakeVisible(reload_);
    display_custom_.onClick = [this] {
        bool display = display_custom_.getToggleState();
        timeview_.SetDisplayWaveform(display);
        spectralview_.repaint();
    };
    addAndMakeVisible(display_custom_);

    setSize(520, 265);
    iir_mode_.onClick();
    display_custom_.setToggleState(p.display_custom_, juce::sendNotificationSync);
    UpdateGui();
    startTimerHz(30);
}

PluginUi::~PluginUi() {
    stopTimer();
}

//==============================================================================
void PluginUi::paint(juce::Graphics& g) {
    g.fillAll(ui::green_bg);
    g.setColour(ui::light_green_bg);
    g.drawRect(lfo_bound_);
    g.drawRect(fir_bound_);
    g.drawRect(feedback_bound_);
    g.drawRect(barber_bound_);
}

void PluginUi::resized() {
    {
        auto b = getLocalBounds();
        preset_panel_.setBounds(b.removeFromTop(30));
        spectralview_.setBounds(150, 30, 220, 125);
        timeview_.setBounds(150, 155, 220, 110);
    }
    {
        auto b = lfo_bound_;
        auto line = b.removeFromTop(20);
        lfo_title_.setBounds(line);

        line = b.removeFromTop(65);
        delay_.setBounds(line.removeFromLeft(50));
        depth_.setBounds(line.removeFromLeft(50));
        speed_.setBounds(line.removeFromLeft(50));

        line = b;
        phase_.setBounds(line.removeFromLeft(50));
        drywet_.setBounds(line.removeFromLeft(50));
    }
    {
        auto b = fir_bound_;
        auto line = b.removeFromTop(20);
        iir_mode_.setBounds(line.removeFromLeft(30));
        minum_phase_.setBounds(line.removeFromRight(50).reduced(1, 0));
        highpass_.setBounds(line.removeFromRight(60).reduced(1, 0));

        line = b.removeFromTop(65);
        cutoff_.setBounds(line.removeFromLeft(50));
        coeff_len_.setBounds(line.removeFromLeft(50));
        side_lobe_.setBounds(line.removeFromLeft(50));
    }
    {
        auto b = feedback_bound_;
        auto line = b.removeFromTop(20);
        panic_.setBounds(line.removeFromRight(50).reduced(1, 0));
        feedback_title_.setBounds(line);

        line = b.removeFromTop(65);
        fb_value_.setBounds(line.removeFromLeft(50));
        fb_damp_.setBounds(line.removeFromLeft(50));
    }
    {
        auto b = barber_bound_;
        auto line = b.removeFromTop(20);
        barber_enable_.setBounds(line.removeFromRight(50).reduced(1, 0));
        barber_title_.setBounds(line);

        line = b.removeFromTop(65);
        barber_speed_.setBounds(line.removeFromLeft(50));
        barber_phase_.setBounds(line.removeFromLeft(50));
        barber_stereo_.setBounds(line.removeFromLeft(50));
    }
    {
        auto b = ops_bound_;
        auto line = b.removeFromTop(20 + 2);
        copy_.setBounds(line.removeFromLeft(50).reduced(1, 1));
        clear_.setBounds(line.removeFromLeft(50).reduced(1, 1));

        line = b.removeFromTop(20 + 2);
        reload_.setBounds(line.removeFromLeft(50).reduced(1, 1));
        display_custom_.setBounds(line.removeFromLeft(80).reduced(1, 1));
    }
}

void PluginUi::timerCallback() {
    if (p_.dsp_state_.have_new_coeff_.exchange(false)) {
        UpdateGui();
    }
}

void PluginUi::SetIirMode(bool is_iir) {
    minum_phase_.setVisible(!is_iir);
    fb_damp_.setEnabled(!is_iir);
    fb_value_.setEnabled(!is_iir);

    if (is_iir) {
        coeff_len_.BindParam(p_.param_iir_filter_num_);
        side_lobe_.BindParam(p_.param_iir_ripple_);
        coeff_len_.slider.onValueChange = [this] {
            spectralview_.DrawIirResponce();
        };
        side_lobe_.slider.onValueChange = [this] {
            spectralview_.DrawIirResponce();
        };
    }
    else {
        coeff_len_.BindParam(p_.param_fir_coeff_len_);
        side_lobe_.BindParam(p_.param_fir_side_lobe_);
        coeff_len_.slider.onValueChange = [] {};
        side_lobe_.slider.onValueChange = [] {};
    }

    coeff_len_.label.setText(is_iir ? "N.filter" : "steep", juce::dontSendNotification);
    side_lobe_.label.setText(is_iir ? "ripple" : "sidelobe", juce::dontSendNotification);

    spectralview_.SetIir(is_iir);
    spectralview_.DrawIirResponce();
}

void PluginUi::TrySetSize(int width, int height) {
    auto* p = findParentComponentOfClass<EmptyAudioProcessorEditor>();
    jassert(p != nullptr);
    p->SetNewSize(width, height);
}
