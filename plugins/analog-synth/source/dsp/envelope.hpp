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

    void Process(size_t channel, bool exp, size_t num_samples) noexcept {
        if (exp) {
            envelope_[channel].ProcessExp({modulator_output[channel].data(), num_samples});
        }
        else {
            envelope_[channel].Process({modulator_output[channel].data(), num_samples});
        }
    }

    qwqdsp::AdsrEnvelope envelope_[kMaxPoly];
};
}