#pragma once
#include <unordered_map>
#include <span>
#include <juce_core/juce_core.h>

namespace tooltip {

class Tooltips {
public:
    Tooltips();
    void MakeEnglishTooltips();
    void MakeChineseTooltips();

    const juce::String& Label(const char* id) const { return labels_.at(id); }
    const juce::String& Tooltip(const char* id) const { return tooltips_.at(id); }
    std::span<const char* const> CombboxIds(const char* id) const { return combbox_ids_.at(id); }
private:
    std::unordered_map<const char*, juce::String> tooltips_;
    std::unordered_map<const char*, juce::String> labels_;
    std::unordered_map<const char*, std::span<const char* const>> combbox_ids_;
};

}