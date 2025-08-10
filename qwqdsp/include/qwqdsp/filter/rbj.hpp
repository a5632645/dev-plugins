#pragma once
#include <cmath>

namespace qwqdsp {
struct RBJ {
    float b0;
    float b1;
    float b2;
    float a0;
    float a1;
    float a2;

    void Lowpass(float w, float Q) {
        auto a = std::sin(w) / (2 * Q);
        auto cosw = std::cos(w);
        b0 = (1 - cosw) / 2.0f;
        b1 = 1 - cosw;
        b2 = b0;
        a0 = 1 + a;
        a1 = -2 * cosw;
        a2 = 1 - a;
    }

    void Highpass(float w, float Q) {
        auto a = std::sin(w) / (2 * Q);
        auto cosw = std::cos(w);
        b0 = (1 + cosw) / 2.0f;
        b1 = -(1 + cosw);
        b2 = b0;
        a0 = 1 + a;
        a1 = -2 * cosw;
        a2 = 1 - a;
    }

    void Bandpass(float w, float Q) {
        auto a = std::sin(w) / (2 * Q);
        auto cosw = std::cos(w);
        b0 = Q * a;
        b1 = 0;
        b2 = -Q * a;
        a0 = 1 + a;
        a1 = -2 * cosw;
        a2 = 1 - a;
    }

    void Peak(float w, float Q, float g) {
        auto a = std::sin(w) / (2 * Q);
        auto cosw = std::cos(w);
        auto A = std::pow(10.0f, g / 40.0f);
        b0 = 1 + a * A;
        b1 = -2 * cosw;
        b2 = 1 - a * A;
        a0 = 1 + a / A;
        a1 = -2 * cosw;
        a2 = 1 - a / A;
    }

    void Lowshelf(float w, float Q, float g) {
        auto a = std::sin(w) / (2 * Q);
        auto cosw = std::cos(w);
        auto A = std::pow(10.0f, g / 40.0f);
        auto sqrtA = std::pow(10.0f, g / 80.0f);
        b0 = A * ((A + 1) - (A - 1) * cosw + 2 * sqrtA * a);
        b1 = 2 * A * ((A - 1) - (A + 1) * cosw);
        b2 = A * ((A + 1) - (A - 1) * cosw - 2 * sqrtA * a);
        a0 = (A + 1) + (A - 1) * cosw + 2 * sqrtA * a;
        a1 = -2 * ((A - 1) + (A + 1) * cosw);
        a2 = (A + 1) + (A - 1) * cosw - 2 * sqrtA * a;
    }

    void HighShelf(float w, float Q, float g) {
        auto a = std::sin(w) / (2 * Q);
        auto cosw = std::cos(w);
        auto A = std::pow(10.0f, g / 40.0f);
        auto sqrtA = std::pow(10.0f, g / 80.0f);
        b0 = A * ((A + 1) + (A - 1) * cosw + 2 * sqrtA * a);
        b1 = -2 * A * ((A - 1) + (A + 1) * cosw);
        b2 = A * ((A + 1) + (A - 1) * cosw - 2 * sqrtA * a);
        a0 = (A + 1) - (A - 1) * cosw + 2 * sqrtA * a;
        a1 = 2 * ((A - 1) - (A + 1) * cosw);
        a2 = (A + 1) - (A - 1) * cosw - 2 * sqrtA * a;
    }

    void Notch(float w, float Q) {
        auto a = std::sin(w) / (2 * Q);
        auto cosw = std::cos(w);
        b0 = 1;
        b1 = -2 * cosw;
        b2 = 1;
        a0 = 1 + a;
        a1 = -2 * cosw;
        a2 = 1 - a;
    }

    void Allpass(float w, float Q) {
        auto a = std::sin(w) / (2 * Q);
        auto cosw = std::cos(w);
        b0 = 1 - a;
        b1 = -2 * cosw;
        b2 = 1 + a;
        a0 = 1 + a;
        a1 = -2 * cosw;
        a2 = 1 - a;
    }
};
}