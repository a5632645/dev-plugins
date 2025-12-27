#pragma once
#include <pluginshared/component.hpp>

class ResonatorAudioProcessor;

class CoupleGui : public juce::Component {
public:
    CoupleGui(ResonatorAudioProcessor& p);

    void resized() override;
    void paint(juce::Graphics& g) override;
private:
    ui::Dial reflection0_{"couple0"};
    ui::Dial reflection1_{"couple1"};
    ui::Dial reflection2_{"couple2"};
    ui::Dial reflection3_{"couple3"};
    ui::Dial reflection4_{"couple4"};
    ui::Dial reflection5_{"couple5"};
    ui::Dial reflection6_{"couple6"};
    ui::Dial reflection7_{"couple7"};
};
