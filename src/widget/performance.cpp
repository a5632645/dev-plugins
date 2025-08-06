#include "performance.hpp"
#include "PluginProcessor.h"
#include "juce_core/juce_core.h"
#include "juce_graphics/juce_graphics.h"
#include <algorithm>
#include <cstddef>

#ifdef __VOCODER_ENABLE_PERFORMANCE_DEBUG
namespace widget {

void Performance::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::black);

    juce::Path p;
    p.startNewSubPath(0.0f, 0.0f);

    auto b = getLocalBounds().toFloat();

    int max = *std::max_element(historys_.cbegin(), historys_.cend());
    size_t num = historys_.size();
    for (size_t i = 0; i < num; ++i) {
        size_t idx = (i + wpos_) % num;
        float norm = 1.0f - historys_[idx] / static_cast<float>(max + 1e-18f);
        float y = norm * b.getHeight();
        p.lineTo(i, y);
    }

    g.setColour(juce::Colours::green);
    g.strokePath(p, juce::PathStrokeType{1.0f});

    juce::String s;
    if (last_ > 1000) {
        s << last_ / 1000 << "ms" << last_ % 1000 << "us";
    }
    else {
        s << last_ << "us";
    }
    g.setColour(juce::Colours::white);
    g.drawSingleLineText(s, 0, b.getCentreY());

    g.drawRect(b);
}

void Performance::resized() {
    historys_.resize(getWidth());
    wpos_ = 0;
}

void Performance::Update() {
    last_ = processor_.process_ns_;
    size_t num = historys_.size();
    historys_[wpos_++] = last_;
    wpos_ %= num;
    repaint();
}

}
#endif