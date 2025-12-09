#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <pluginshared/component.hpp>

class AudioPluginAudioProcessor;

namespace widget {

class BurgLPC : public juce::Component {
public:
    BurgLPC(AudioPluginAudioProcessor& processor);
    void resized() override;
    void paint(juce::Graphics& g) override;
private:
    AudioPluginAudioProcessor& processor_;
    ui::Dial forget_{"forget"};
    ui::Dial smear_{"smear"};
    ui::FlatSlider dicimate_{"dicimate", ui::FlatSlider::TitleLayout::Top};
    ui::FlatSlider order_{"order", ui::FlatSlider::TitleLayout::Top};
    ui::Dial attack_{"attack"};
    ui::Dial release_{"release"};
};

}
