#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <ui/vertical_slider.hpp>
#include "tooltips.hpp"
#include "ui/comb_box.hpp"

class AudioPluginAudioProcessor;

namespace widget {

class Ensemble : public juce::Component, public tooltip::Tooltips::Listener {
public:
    Ensemble(AudioPluginAudioProcessor& p);
    void resized() override;
    void OnLanguageChanged(tooltip::Tooltips& tooltips) override;
private:
    juce::Label label_;
    ui::VerticalSlider num_voice_;
    ui::VerticalSlider detune_;
    ui::VerticalSlider spread_;
    ui::VerticalSlider mix_;
    ui::VerticalSlider rate_;
    ui::CombBox mode_;
};

}