#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "qwqdsp/performance.hpp"

namespace widget {

class Performance : public juce::Component {
public:
    Performance(qwqdsp::Performance& p)
        : processor_(p)
    {}

    void paint(juce::Graphics& g) override;

    void resized() override;

    void Update();

private:
    qwqdsp::Performance& processor_;
    std::vector<int> historys_;
    int wpos_{};
    int last_{};
    int avg_{};
};

}
