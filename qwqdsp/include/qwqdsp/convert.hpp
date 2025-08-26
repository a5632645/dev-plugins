#pragma once
#include <numbers>
#include <cmath>

namespace qwqdsp::convert {
static constexpr float Freq2W(float f, float fs) {
    return f * std::numbers::pi_v<float> * 2 / fs;
}

static float Freq2Pitch(float f, float a4 = 440.0f) {
    return 69.0f + 12.0f * std::log2(f / a4);
}

static float Pitch2Freq(float pitch, float a4 = 440.0f) {
    return a4 * std::pow(2.0f, (pitch - 69.0f) / 12.0f);
}

static float Freq2Mel(float f) {
    return 1127.0f * std::log(1.0f + f / 700.0f);
}

static float Mel2Freq(float mel) {
    return 700.0f * (std::exp(mel / 1127.0f) - 1.0f);
}

static float Samples2Decay(float samples, float gain) {
    if (samples < 1.0f) {
        return 0.0f;
    }
    return std::pow(gain, 1.0f / samples);
}

static float Samples2DecayDb(float samlpes, float db) {
    if (samlpes < 1.0f) {
        return 0.0f;
    }
    return std::pow(10.0f, db / (20.0f * samlpes));
}
}