#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/vertical_slider.hpp"
#include "ui/comb_box.hpp"
#include "tooltips.hpp"

class AudioPluginAudioProcessor;

namespace widget {
class Tracking : public juce::Component {
public:
    Tracking(AudioPluginAudioProcessor& p);

    void ChangeLang(tooltip::Tooltips& t);
    void resized() override;
private:
    juce::Label label_{"tracking", "tracking"};
    ui::VerticalSlider fmin_;
    ui::VerticalSlider fmax_;
    ui::VerticalSlider pitch_;
    ui::VerticalSlider pwm_;
    ui::CombBox waveform_;
    ui::VerticalSlider noise_;
};
}