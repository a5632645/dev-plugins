#include "chorus_view.hpp"

#include <pluginshared/component.hpp>

#include "../PluginProcessor.h"

void ChorusView::paint(juce::Graphics& g) {
    g.fillAll(ui::black_bg);
    auto copy = p_.dsp_.delay_ms_;
    auto num_voices = static_cast<size_t>(p_.param_num_voices_->get());
    float const ytop = 0.25f * static_cast<float>(getHeight());
    float const ybottom = 0.75f * static_cast<float>(getHeight());
    for (size_t i = 0; i < num_voices / simd::LaneSize<SimdType>; ++i) {
        SimdType norm = copy[i] / (VitalChorus::kMaxDelayMs);
        SimdType x = norm * (static_cast<float>(getWidth()));
        // left
        g.setColour(ui::line_fore);
        for (size_t j = 0; j < simd::LaneSize<SimdType>; j += 2) {
            g.drawVerticalLine(static_cast<int>(x[j]), ytop, ybottom);
        }
        // right
        g.setColour(ui::active_bg);
        for (size_t j = 1; j < simd::LaneSize<SimdType>; j += 2) {
            g.drawVerticalLine(static_cast<int>(x[j]), ytop, ybottom);
        }
    }
}
