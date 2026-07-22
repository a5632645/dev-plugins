#include "tracking.hpp"
#include "PluginProcessor.h"

namespace green_vocoder::widget {
Tracking::Tracking(AudioPluginAudioProcessor& p) {
    fmin_.BindParam(p.params_.track_low.ptr_);
    addAndMakeVisible(fmin_);

    fmax_.BindParam(p.params_.track_high.ptr_);
    addAndMakeVisible(fmax_);

    pitch_.BindParam(p.params_.track_pitch.ptr_);
    addAndMakeVisible(pitch_);

    pwm_.BindParam(p.params_.track_pwm.ptr_);
    addAndMakeVisible(pwm_);

    waveform_.BindParam(p.params_.track_waveform.ptr_);
    addAndMakeVisible(waveform_);

    noise_.BindParam(p.params_.track_noise.ptr_);
    addAndMakeVisible(noise_);

    glide_.BindParam(p.params_.track_glide.ptr_);
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
} // namespace green_vocoder::widget
