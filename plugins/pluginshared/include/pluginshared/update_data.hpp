#pragma once
#include <juce_core/juce_core.h>

namespace pluginshared {
class UpdateData {
public:
    UpdateData() = default;

    juce::String GetPluginReleaseUrl() {
        return "https://github.com/ManasWolrd/dev-plugins/releases/latest";
    }
};
} // namespace pluginshared
