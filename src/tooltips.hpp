#pragma once
#include <unordered_map>
#include <span>
#include <juce_core/juce_core.h>

namespace tooltip {

class Tooltips {
public:
    struct Listener {
        virtual ~Listener() = default;
        virtual void OnLanguageChanged(Tooltips& tooltips) = 0;
    };

    Tooltips();
    void MakeEnglishTooltips();
    void MakeChineseTooltips();
    void AddListener(Listener* listener) { listeners_.push_back(listener); }
    void AddListenerAndInvoke(Listener* listener) { listeners_.push_back(listener); listener->OnLanguageChanged(*this); }
    void RemoveListener(Listener* listener) { listeners_.erase(std::remove(listeners_.begin(), listeners_.end(), listener), listeners_.end()); }
    void OnLanguageChanged() { for (auto listener : listeners_) listener->OnLanguageChanged(*this); }

    // const juce::String& operator[](const char* id) const { return tooltips_.at(id); }
    const juce::String& Label(const char* id) const { return labels_.at(id); }
    const juce::String& Tooltip(const char* id) const { return tooltips_.at(id); }
    std::span<const char* const> CombboxIds(const char* id) const { return combbox_ids_.at(id); }
private:
    std::unordered_map<const char*, juce::String> tooltips_;
    std::unordered_map<const char*, juce::String> labels_;
    std::unordered_map<const char*, std::span<const char* const>> combbox_ids_;
    std::vector<Listener*> listeners_;
};

extern Tooltips tooltips;

}