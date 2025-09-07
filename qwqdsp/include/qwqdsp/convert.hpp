#pragma once
#include <numbers>
#include <cmath>
#include <utility>
#include <span>

namespace qwqdsp::convert {
static inline constexpr float Freq2W(float f, float fs) noexcept {
    return f * std::numbers::pi_v<float> * 2 / fs;
}

static inline float Freq2Pitch(float f, float a4 = 440.0f) noexcept {
    return 69.0f + 12.0f * std::log2(f / a4);
}

static inline float Pitch2Freq(float pitch, float a4 = 440.0f) noexcept {
    return a4 * std::pow(2.0f, (pitch - 69.0f) / 12.0f);
}

static inline float Freq2Mel(float f) noexcept {
    return 1127.0f * std::log(1.0f + f / 700.0f);
}

static inline float Mel2Freq(float mel) noexcept {
    return 700.0f * (std::exp(mel / 1127.0f) - 1.0f);
}

static inline float Samples2Decay(float samples, float gain) noexcept {
    if (samples < 1.0f) {
        return 0.0f;
    }
    return std::pow(gain, 1.0f / samples);
}

static inline float Samples2DecayDb(float samlpes, float db) noexcept {
    if (samlpes < 1.0f) {
        return 0.0f;
    }
    return std::pow(10.0f, db / (20.0f * samlpes));
}

namespace analog {
/**
 * 倍频程表示 f2 = 2^N * f1
 */
static inline float Bandwidth2Octave(float f1, float f2) noexcept {
    float y = f2 / f1;
    return std::log2(y);
}

/**
 *              f0
 * Q表示 Q = ---------
 *           f2 - f1
 */
static inline float Octave2Q(float octave) noexcept {
    return 0.5f / std::sinh(std::numbers::ln2_v<float> * 0.5f * octave);
}

static inline float Q2Octave(float Q) noexcept {
    auto a = (2.0f * Q * Q + 1.0f) / (2.0f * Q);
    auto c = (2.0f * Q * Q + 1.0f) / (Q * Q);
    auto b = std::sqrt(c * c * 0.25f - 1.0f);
    auto x0 = a + b;
    auto x1 = a - b;
    return x0 > 0 ? x0 : x1;
}

/**
 * @return [f+, f-]
 */
static inline std::pair<float, float> Octave2Frequency(float f0, float octave) noexcept {
    auto a = std::exp2(octave * 0.5f);
    return {f0 * a, f0 / a};
}
} // analog

/**
 * @return 模拟频率(hz)
 */
static inline float DigitalFreq2AnalogBilinear(float freq, float fs) noexcept {
    auto w = 2.0f * fs * std::tan(std::numbers::pi_v<float> * freq / fs);
    return w / (std::numbers::pi_v<float> * 2.0f);
}

static inline float DigitalOctave2AnalogQ(float w, float octave) noexcept {
    auto a = std::numbers::ln2_v<float> * 0.5f * octave * w / std::sin(w);
    return 0.5f / std::sinh(a);
}

/**
 * @param w 数字角频率 0~pi  rad/sec
 * @param bw 数字角频率 rad/sec |= bw(hz) * 2pi / fs
 * @see Freq2W
 */
static inline float DigitalBW2AnalogQ(float w, float bw) noexcept {
    auto w0 = w - bw * 0.5f;
    auto w1 = w + bw * 0.5f;
    auto octave = w1 / w0;
    return DigitalOctave2AnalogQ(w, octave);
}

static inline float Gain2Db(float gain) noexcept {
    return 20.0f * std::log10(gain + 1e-18f);
}

static inline float Db2Gain(float db) noexcept {
    return std::pow(10.0f, db / 20.0f);
}
} // qwqdsp::convert