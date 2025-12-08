#pragma once
#include <juce_core/juce_core.h>
#include "constant.hpp"

namespace analogsynth {
class IModulator {
public:
    IModulator(juce::StringRef _name) : name_(_name) {}
    virtual ~IModulator() = default;

    // [0~1]
    using ModulatorOutputBuffer = std::array<float, kBlockSize>;
    ModulatorOutputBuffer modulator_output[kMaxPoly]{};
    juce::String name_;
};
}