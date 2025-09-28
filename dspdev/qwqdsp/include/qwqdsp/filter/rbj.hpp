#pragma once
#include <cmath>
#include <numbers>

namespace qwqdsp::filter {
/**
 * @ref https://www.w3.org/TR/audio-eq-cookbook/
 */
struct RBJ {
    float b0;
    float b1;
    float b2;
    float a1;
    float a2;

    /**
     * 使用数字倍频程表示带宽具有一阶prewarp Q
     */
    static float DigitalOctave2AnalogQ(float w, float octave) noexcept {
        auto a = std::numbers::ln2_v<float> * 0.5f * octave * w / std::sin(w);
        return 0.5f / std::sinh(a);
    }

    static float DigitalBW2AnalogQ(float w, float bw) noexcept {
        auto f0 = w - bw * 0.5f;
        auto f1 = w + bw * 0.5f;
        auto octave = f1 / f0;
        return DigitalOctave2AnalogQ(w, octave);
    }

    void Lowpass(float w, float Q) noexcept {
        auto a = std::sin(w) / (2 * Q);
        auto cosw = std::cos(w);
        b0 = (1 - cosw) / 2.0f;
        b1 = 1 - cosw;
        b2 = b0;
        a1 = -2 * cosw;
        a2 = 1 - a;
        float inva0 = 1.0f / (1 + a);
        b0 *= inva0;
        b1 *= inva0;
        b2 *= inva0;
        a1 *= inva0;
        a2 *= inva0;
    }

    void Highpass(float w, float Q) noexcept {
        auto a = std::sin(w) / (2 * Q);
        auto cosw = std::cos(w);
        b0 = (1 + cosw) / 2.0f;
        b1 = -(1 + cosw);
        b2 = b0;
        a1 = -2 * cosw;
        a2 = 1 - a;
        float inva0 = 1.0f / (1 + a);
        b0 *= inva0;
        b1 *= inva0;
        b2 *= inva0;
        a1 *= inva0;
        a2 *= inva0;
    }

    /**
     * |H(z=exp(jw))| = Q
     */
    void Bandpass(float w, float Q) noexcept {
        auto a = std::sin(w) / (2 * Q);
        auto cosw = std::cos(w);
        b0 = Q * a;
        b1 = 0;
        b2 = -Q * a;
        a1 = -2 * cosw;
        a2 = 1 - a;
        float inva0 = 1.0f / (1 + a);
        b0 *= inva0;
        b1 *= inva0;
        b2 *= inva0;
        a1 *= inva0;
        a2 *= inva0;
    }

    /**
     * |H(z=exp(jw))| = 0
     */
    void BandpassKeep0(float w, float Q) noexcept {
        auto a = std::sin(w) / (2 * Q);
        auto cosw = std::cos(w);
        b0 = a;
        b1 = 0;
        b2 = -a;
        a1 = -2 * cosw;
        a2 = 1 - a;
        float inva0 = 1.0f / (1 + a);
        b0 *= inva0;
        b1 *= inva0;
        b2 *= inva0;
        a1 *= inva0;
        a2 *= inva0;
    }

    void Peak(float w, float Q, float g) noexcept {
        auto a = std::sin(w) / (2 * Q);
        auto cosw = std::cos(w);
        auto A = std::pow(10.0f, g / 40.0f);
        b0 = 1 + a * A;
        b1 = -2 * cosw;
        b2 = 1 - a * A;
        a1 = -2 * cosw;
        a2 = 1 - a / A;
        float inva0 = 1.0f / (1 + a / A);
        b0 *= inva0;
        b1 *= inva0;
        b2 *= inva0;
        a1 *= inva0;
        a2 *= inva0;
    }

    void Lowshelf(float w, float Q, float g) noexcept {
        auto a = std::sin(w) / (2 * Q);
        auto cosw = std::cos(w);
        auto A = std::pow(10.0f, g / 40.0f);
        auto sqrtA = std::pow(10.0f, g / 80.0f);
        b0 = A * ((A + 1) - (A - 1) * cosw + 2 * sqrtA * a);
        b1 = 2 * A * ((A - 1) - (A + 1) * cosw);
        b2 = A * ((A + 1) - (A - 1) * cosw - 2 * sqrtA * a);
        a1 = -2 * ((A - 1) + (A + 1) * cosw);
        a2 = (A + 1) + (A - 1) * cosw - 2 * sqrtA * a;
        float inva0 = 1.0f / ((A + 1) + (A - 1) * cosw + 2 * sqrtA * a);
        b0 *= inva0;
        b1 *= inva0;
        b2 *= inva0;
        a1 *= inva0;
        a2 *= inva0;
    }

    void HighShelf(float w, float Q, float g) noexcept {
        auto a = std::sin(w) / (2 * Q);
        auto cosw = std::cos(w);
        auto A = std::pow(10.0f, g / 40.0f);
        auto sqrtA = std::pow(10.0f, g / 80.0f);
        b0 = A * ((A + 1) + (A - 1) * cosw + 2 * sqrtA * a);
        b1 = -2 * A * ((A - 1) + (A + 1) * cosw);
        b2 = A * ((A + 1) + (A - 1) * cosw - 2 * sqrtA * a);
        a1 = 2 * ((A - 1) - (A + 1) * cosw);
        a2 = (A + 1) - (A - 1) * cosw - 2 * sqrtA * a;
        float inva0 = 1.0f / ((A + 1) - (A - 1) * cosw + 2 * sqrtA * a);
        b0 *= inva0;
        b1 *= inva0;
        b2 *= inva0;
        a1 *= inva0;
        a2 *= inva0;
    }

    void Notch(float w, float Q) noexcept {
        auto a = std::sin(w) / (2 * Q);
        auto cosw = std::cos(w);
        b0 = 1;
        b1 = -2 * cosw;
        b2 = 1;
        a1 = -2 * cosw;
        a2 = 1 - a;
        float inva0 = 1.0f / (1 + a);
        b0 *= inva0;
        b1 *= inva0;
        b2 *= inva0;
        a1 *= inva0;
        a2 *= inva0;
    }

    void Allpass(float w, float Q) noexcept {
        auto a = std::sin(w) / (2 * Q);
        auto cosw = std::cos(w);
        b0 = 1 - a;
        b1 = -2 * cosw;
        b2 = 1 + a;
        a1 = -2 * cosw;
        a2 = 1 - a;
        float inva0 = 1.0f / (1 + a);
        b0 *= inva0;
        b1 *= inva0;
        b2 *= inva0;
        a1 *= inva0;
        a2 *= inva0;
    }
};
}