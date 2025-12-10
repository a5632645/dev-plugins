#include "tracking.hpp"
#include "PluginProcessor.h"
#include "param_ids.hpp"

namespace green_vocoder::widget {
Tracking::Tracking(AudioPluginAudioProcessor& p) {
    auto& apvts = *p.value_tree_;

    fmin_.BindParam(apvts, id::kTrackingLow);
    addAndMakeVisible(fmin_);

    fmax_.BindParam(apvts, id::kTrackingHigh);
    addAndMakeVisible(fmax_);

    pitch_.BindParam(apvts, id::kTrackingPitch);
    addAndMakeVisible(pitch_);

    pwm_.BindParam(apvts, id::kTrackingPwm);
    addAndMakeVisible(pwm_);

    waveform_.BindParam(apvts, id::kTrackingWaveform);
    addAndMakeVisible(waveform_);

    noise_.BindParam(apvts, id::kTrackingNoise);
    addAndMakeVisible(noise_);

    glide_.BindParam(apvts, id::kTrackingGlide);
    addAndMakeVisible(glide_);

    addAndMakeVisible(title_);
}

void Tracking::resized() {
    auto b = getLocalBounds();
    title_.setBounds(b.removeFromTop(20));

    auto f_bound = b.removeFromLeft(50);
    fmin_.setBounds(f_bound.removeFromTop(f_bound.getHeight() / 2));
    fmax_.setBounds(f_bound);

    auto dials = b.removeFromLeft(100).removeFromTop(65);
    pitch_.setBounds(dials.removeFromLeft(50));
    pwm_.setBounds(dials.removeFromLeft(50));
    
    waveform_.setBounds(b.removeFromTop(25));
    noise_.setBounds(b.removeFromTop(25));
    glide_.setBounds(b.removeFromTop(25));
}
}
