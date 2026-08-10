#include "tracking.hpp"
#include "PluginProcessor.h"

namespace green_vocoder::ui {
Tracking::Tracking(AudioPluginAudioProcessor& p) {
    fmin_.BindParam(p.params_.track_low.ptr_);
    addAndMakeVisible(fmin_);
    fmin_title_.setJustificationType(juce::Justification::centredLeft);
    ::ui::SetLableBlack(fmin_title_);
    addAndMakeVisible(fmin_title_);

    fmax_.BindParam(p.params_.track_high.ptr_);
    addAndMakeVisible(fmax_);
    fmax_title_.setJustificationType(juce::Justification::centredLeft);
    ::ui::SetLableBlack(fmax_title_);
    addAndMakeVisible(fmax_title_);

    pitch_.BindParam(p.params_.track_pitch.ptr_);
    addAndMakeVisible(pitch_);

    pwm_.BindParam(p.params_.track_pwm.ptr_);
    addAndMakeVisible(pwm_);

    waveform_.BindParam(p.params_.track_waveform.ptr_);
    addAndMakeVisible(waveform_);

    noise_.BindParam(p.params_.track_noise.ptr_);
    addAndMakeVisible(noise_);
    noise_title_.setJustificationType(juce::Justification::centredLeft);
    ::ui::SetLableBlack(noise_title_);
    addAndMakeVisible(noise_title_);

    glide_.BindParam(p.params_.track_glide.ptr_);
    addAndMakeVisible(glide_);
    glide_title_.setJustificationType(juce::Justification::centredLeft);
    ::ui::SetLableBlack(glide_title_);
    addAndMakeVisible(glide_title_);

    addAndMakeVisible(title_);
}

void Tracking::resized() {
    auto b = getLocalBounds();
    title_.setBounds(b.removeFromTop(20));

    auto f_bound = b.removeFromLeft(50);
    {
        auto fmin_bound = f_bound.removeFromTop(f_bound.getHeight() / 2);
        fmin_title_.setBounds(fmin_bound.removeFromTop(static_cast<int>(fmin_title_.getFont().getHeight())));
        fmin_.setBounds(fmin_bound);
    }
    fmax_title_.setBounds(f_bound.removeFromTop(static_cast<int>(fmax_title_.getFont().getHeight())));
    fmax_.setBounds(f_bound);

    auto dials = b.removeFromLeft(100).removeFromTop(65);
    pitch_.setBounds(dials.removeFromLeft(50));
    pwm_.setBounds(dials.removeFromLeft(50));

    waveform_.setBounds(b.removeFromTop(25));
    {
        auto noise_bound = b.removeFromTop(25);
        auto noise_width = static_cast<int>(1.2f * juce::TextLayout::getStringWidth(noise_title_.getFont(), noise_title_.getText()));
        noise_title_.setBounds(noise_bound.removeFromLeft(noise_width));
        noise_.setBounds(noise_bound);
    }
    {
        auto glide_bound = b.removeFromTop(25);
        auto glide_width = static_cast<int>(1.2f * juce::TextLayout::getStringWidth(glide_title_.getFont(), glide_title_.getText()));
        glide_title_.setBounds(glide_bound.removeFromLeft(glide_width));
        glide_.setBounds(glide_bound);
    }
}
} // namespace green_vocoder::ui
