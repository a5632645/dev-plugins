#include "performance.hpp"
#include <algorithm>
#include <cstddef>

namespace widget {

static juce::String FormatTime(int us) {
    juce::String s;
    if (us > 1000) {
        s << us / 1000 << "ms" << us % 1000 << "us";
    }
    else {
        s << us << "us";
    }
    return s;
}

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
    s << "curr: " << FormatTime(last_) << " avg: " << FormatTime(avg_ / historys_.size());
    g.setColour(juce::Colours::white);
    g.drawSingleLineText(s, 0, b.getCentreY());

    g.drawRect(b);
}

void Performance::resized() {
    historys_.resize(getWidth());
    wpos_ = 0;
}

void Performance::Update() {
    last_ = processor_.GetProcessUs();
    size_t num = historys_.size();
    avg_ -= historys_[wpos_];
    historys_[wpos_++] = last_;
    avg_ += last_;
    wpos_ %= num;
    repaint();
}

}
