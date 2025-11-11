#pragma once
#include <juce_core/juce_core.h>

namespace analogsynth {
class IModulator {
public:
    IModulator(juce::StringRef _name) : name_(_name) {}
    virtual ~IModulator() = default;

    float modulator_output[256]{};
    juce::String name_;
};
}