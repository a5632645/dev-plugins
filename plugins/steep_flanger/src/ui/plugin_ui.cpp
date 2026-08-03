#include "plugin_ui.hpp"
#include "../PluginEditor.h"
#include "../PluginProcessor.h"

PluginUi::PluginUi(SteepFlangerAudioProcessor& p)
    : p_(p)
    , preset_panel_(*p.preset_manager_)
    , timeview_(p)
    , spectralview_(timeview_, p) {
    preset_panel_.SetDspInstName(p.dsp_->InstName().data());
    addAndMakeVisible(preset_panel_);

    addAndMakeVisible(lfo_title_);
    delay_.BindParam(p.params_.delay.ptr_);
    addAndMakeVisible(delay_);
    depth_.BindParam(p.params_.depth.ptr_);
    addAndMakeVisible(depth_);
    speed_.BindParam(p.params_.delay_lfo);
    addAndMakeVisible(speed_);
    phase_.BindParam(p.params_.lfo_phase.ptr_);
    addAndMakeVisible(phase_);
    drywet_.BindParam(p.params_.drywet.ptr_);
    addAndMakeVisible(drywet_);

    cutoff_.BindParam(p.params_.fir_cutoff.ptr_);
    cutoff_.slider.onValueChange = [this] { spectralview_.DrawIirResponce(); };
    addAndMakeVisible(cutoff_);
    coeff_len_.BindParam(p.params_.fir_coeff_len.ptr_);
    addAndMakeVisible(coeff_len_);
    side_lobe_.BindParam(p.params_.fir_side_lobe.ptr_);
    addAndMakeVisible(side_lobe_);
    minum_phase_.BindParam(p.params_.fir_min_phase.ptr_);
    addAndMakeVisible(minum_phase_);
    iir_mode_.onClick = [this] { SetIirMode(iir_mode_.getToggleState()); };
    iir_mode_.BindParam(p.params_.iir_mode.ptr_);
    addAndMakeVisible(iir_mode_);
    highpass_.BindParam(p.params_.fir_highpass.ptr_);
    highpass_.onClick = [this] { spectralview_.DrawIirResponce(); };
    addAndMakeVisible(highpass_);

    fb_value_.BindParam(p.params_.feedback.ptr_);
    addAndMakeVisible(fb_value_);
    panic_.setButtonText("panic");
    panic_.onClick = [&p] { p.reset(); };
    addAndMakeVisible(panic_);
    fb_damp_.BindParam(p.params_.damp_pitch.ptr_);
    addAndMakeVisible(fb_damp_);
    addAndMakeVisible(feedback_title_);

    addAndMakeVisible(barber_title_);
    barber_phase_.BindParam(p.params_.barber_phase.ptr_);
    addAndMakeVisible(barber_phase_);
    barber_speed_.BindParam(p.params_.barber_lfo);
    addAndMakeVisible(barber_speed_);
    barber_enable_.BindParam(p.params_.barber_enable.ptr_);
    addAndMakeVisible(barber_enable_);
    barber_stereo_.BindParam(p.params_.barber_stereo.ptr_);
    addAndMakeVisible(barber_stereo_);

    addAndMakeVisible(timeview_);
    addAndMakeVisible(spectralview_);

    clear_.onClick = [this] { timeview_.ClearCustomCoeffs(); };
    addAndMakeVisible(clear_);
    copy_.onClick = [this] { timeview_.CopyCoeffesToCustom(); };
    addAndMakeVisible(copy_);
    reload_.onClick = [this] { timeview_.SendCoeffs(); };
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
    if (p_.dsp_->ExchangeNewCoeff()) {
        UpdateGui();
    }
}

void PluginUi::SetIirMode(bool is_iir) {
    minum_phase_.setVisible(!is_iir);
    fb_damp_.setEnabled(!is_iir);
    fb_value_.setEnabled(!is_iir);

    if (is_iir) {
        coeff_len_.BindParam(p_.params_.iir_filter_num.ptr_);
        side_lobe_.BindParam(p_.params_.iir_ripple.ptr_);
        coeff_len_.slider.onValueChange = [this] { spectralview_.DrawIirResponce(); };
        side_lobe_.slider.onValueChange = [this] { spectralview_.DrawIirResponce(); };
    }
    else {
        coeff_len_.BindParam(p_.params_.fir_coeff_len.ptr_);
        side_lobe_.BindParam(p_.params_.fir_side_lobe.ptr_);
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
