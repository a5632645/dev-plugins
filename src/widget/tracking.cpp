#include "tracking.hpp"

#include "PluginProcessor.h"
#include "param_ids.hpp"

namespace widget {
Tracking::Tracking(AudioPluginAudioProcessor& p) {
    auto& apvts = *p.value_tree_;

    fmin_.BindParameter(apvts, id::kTrackingLow);
    addAndMakeVisible(fmin_);

    fmax_.BindParameter(apvts, id::kTrackingHigh);
    addAndMakeVisible(fmax_);

    pitch_.BindParameter(apvts, id::kTrackingPitch);
    addAndMakeVisible(pitch_);

    pwm_.BindParameter(apvts, id::kTrackingPwm);
    addAndMakeVisible(pwm_);

    waveform_.SetNoChoiceStrs(true);
    waveform_.BindParam(apvts, id::kTrackingWaveform);
    addAndMakeVisible(waveform_);

    noise_.SetHorizontal(true);
    noise_.BindParameter(apvts, id::kTrackingNoise);
    addAndMakeVisible(noise_);

    addAndMakeVisible(label_);
}

void Tracking::ChangeLang(tooltip::Tooltips& t) {
    fmin_.OnLanguageChanged(t);
    fmax_.OnLanguageChanged(t);
    pitch_.OnLanguageChanged(t);
    pwm_.OnLanguageChanged(t);
    waveform_.OnLanguageChanged(t);
    noise_.OnLanguageChanged(t);
}

void Tracking::resized() {
    auto b = getLocalBounds();
    label_.setBounds(b.removeFromTop(20));
    fmin_.setBounds(b.removeFromLeft(50));
    fmax_.setBounds(b.removeFromLeft(50));
    pitch_.setBounds(b.removeFromLeft(50));
    pwm_.setBounds(b.removeFromLeft(50));
    waveform_.setBounds(b.removeFromTop(25));
    noise_.setBounds(b.removeFromTop(30));
}
}