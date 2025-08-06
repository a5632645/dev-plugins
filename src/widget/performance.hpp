#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

class AudioPluginAudioProcessor;

namespace widget {

class Performance : public juce::Component {
public:
    Performance(AudioPluginAudioProcessor& p)
        : processor_(p)
    {}

    void paint(juce::Graphics& g) override;

    void resized() override;

    void Update();

private:
    AudioPluginAudioProcessor& processor_;
    std::vector<int> historys_;
    int wpos_{};
    int last_{};
};

}