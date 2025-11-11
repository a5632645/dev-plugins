#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <pluginshared/component.hpp>

namespace analogsynth {
class Synth;

class Osc4Gui : public juce::Component {
public:
    Osc4Gui(Synth& synth);
    void resized() override;
    void paint(juce::Graphics& g) override;
private:
    juce::Label title_{"", "Oscillator4"};
    ui::Dial detune_{"detune"};
    ui::Dial volume_{"volume"};
    ui::Dial ratio_{"ratio"};
    ui::Dial slope_{"slope"};
    ui::Dial width_{"width"};
    juce::Label label_shape_{"","shape"};
    ui::FlatCombobox shape_;
    ui::FlatSlider n_{"n"};
    ui::Switch use_max_n_{"max_n"};
};
}