#pragma once
#include <numbers>
#include <cmath>
#include <utility>
#include <span>

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

namespace analog {
/**
 * 倍频程表示 f2 = 2^N * f1
 */
static float Bandwidth2Octave(float f1, float f2) {
    float y = f2 / f1;
    return std::log2(y);
}

/**
 *              f0
 * Q表示 Q = ---------
 *           f2 - f1
 */
static float Octave2Q(float octave) {
    return 0.5f / std::sinh(std::numbers::ln2_v<float> * 0.5f * octave);
}

static float Q2Octave(float Q) {
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
static std::pair<float, float> Octave2Frequency(float f0, float octave) {
    auto a = std::exp2(octave * 0.5f);
    return {f0 * a, f0 / a};
}
} // analog

/**
 * @return 模拟频率(hz)
 */
static float DigitalFreq2AnalogBilinear(float freq, float fs) {
    auto w = 2.0f * fs * std::tan(std::numbers::pi_v<float> * freq / fs);
    return w / (std::numbers::pi_v<float> * 2.0f);
}

static float DigitalOctave2AnalogQ(float w, float octave) {
    auto a = std::numbers::ln2_v<float> * 0.5f * octave * w / std::sin(w);
    return 0.5f / std::sinh(a);
}

/**
 * @param w 数字角频率 0~pi  rad/sec
 * @param bw 数字角频率 rad/sec |= bw(hz) * 2pi / fs
 * @see Freq2W
 */
static float DigitalBW2AnalogQ(float w, float bw) {
    auto w0 = w - bw * 0.5f;
    auto w1 = w + bw * 0.5f;
    auto octave = w1 / w0;
    return DigitalOctave2AnalogQ(w, octave);
}

static float Gain2Db(float gain) {
    return 20.0f * std::log10(gain + 1e-18f);
}

static float Db2Gain(float db) {
    return std::pow(10.0f, db / 20.0f);
}

static void Lattice2Tf(std::span<const float> lattice, std::span<float> tf) {

}

static void Tf2Lattice(std::span<const float> tf, std::span<float> lattice) {
    
}
} // qwqdsp::convert