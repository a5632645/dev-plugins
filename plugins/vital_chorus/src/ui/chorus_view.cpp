#include "chorus_view.hpp"
#include <pluginshared/component.hpp>
#include "../PluginProcessor.h"
#include "../global.hpp"

void ChorusView::paint(juce::Graphics& g) {
    g.fillAll(ui::black_bg);
    auto const delay_ms = p_.dsp_->GetDelayMs();
    auto num_voices = static_cast<size_t>(p_.params_.num_voices.Get());
    float const ytop = 0.25f * static_cast<float>(getHeight());
    float const ybottom = 0.75f * static_cast<float>(getHeight());
    float const width = static_cast<float>(getWidth());

    g.setColour(ui::line_fore);
    for (size_t i = 0; i < num_voices; i += 2) {
        float const x = delay_ms[i] / global::kMaxDelayMs * width;
        g.drawVerticalLine(static_cast<int>(x), ytop, ybottom);
    }

    g.setColour(ui::active_bg);
    for (size_t i = 1; i < num_voices; i += 2) {
        float const x = delay_ms[i] / global::kMaxDelayMs * width;
        g.drawVerticalLine(static_cast<int>(x), ytop, ybottom);
    }
}
