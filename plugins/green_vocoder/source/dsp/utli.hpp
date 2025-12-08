#pragma once
#include <cmath>

namespace dsp::utli {

/*
 * \brief get a decay value where ms later decrease 60dB
 */
static float GetDecayValue(float sample_rate, float ms) {
    float nsamples = ms * sample_rate / 1000.0f;
    if (nsamples < 1.0f) {
        return 0.0f;
    }
    return std::pow(10.0f, -3.0f / nsamples);
}

}
