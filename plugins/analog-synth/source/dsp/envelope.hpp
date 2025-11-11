#pragma once
#include "imodulator.hpp"

#include <qwqdsp/polymath.hpp>
#include <qwqdsp/osciilor/smooth_noise.hpp>
#include <qwqdsp/adsr_envelope.hpp>

namespace analogsynth {
class ModEnvelope : public IModulator {
public:
    ModEnvelope(juce::StringRef name)
        : IModulator(name) {
    }

    void Process(bool exp, size_t num_samples) noexcept {
        if (exp) {
            envelope_.ProcessExp({modulator_output, num_samples});
        }
        else {
            envelope_.Process({modulator_output, num_samples});
        }
    }

    qwqdsp::AdsrEnvelope envelope_;
};
}