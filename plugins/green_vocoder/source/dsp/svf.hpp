#pragma once
#include <cmath>

namespace dsp {
struct SVF {
    float m0{};
    float m1{};
    float m2{};
    float ic2eq{};
    float ic1eq{};
    float a1{};
    float a2{};
    float a3{};

    float Tick(float v0) {
        float v3 = v0 - ic2eq;
        float v1 = a1 * ic1eq + a2 * v3;
        float v2 = ic2eq + a2 * ic1eq + a3 * v3;
        ic1eq = 2 * v1 - ic1eq;
        ic2eq = 2 * v2 - ic2eq;
        float out = m0 * v0 + m1 * v1 + m2 * v2;
        return out;
    }

    void MakeBell(float omega, float q, float gain) {
        float A = std::pow(10.0f, gain / 40.0f);
        float g = std::tan(omega / 2);
        float k = 1.0f / (q * A);
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
        m0 = 1.0f;
        m1 = k * (A * A - 1.0f);
        m2 = 0.0f;
    }

    void MakeLowShelf(float omega, float q, float gain) {
        float A = std::pow(10.0f, gain / 40.0f);
        float g = std::tan(omega / 2) / std::sqrt(A);
        float k = 1.0f / q;
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
        m0 = 1.0f;
        m1 = k * (A - 1);
        m2 = A * A - 1;
    }

    void MakeHighShelf(float omega, float q, float gain) {
        float A = std::pow(10.0f, gain / 40.0f);
        float g = std::tan(omega / 2) * std::sqrt(A);
        float k = 1.0f / q;
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
        m0 = A * A;
        m1 = k * (1 - A) * A;
        m2 = 1 - A * A;
    }

    void MakeLowpass(float omega, float Q) {
        float g = std::tan(omega / 2);
        float k = 1.0f / Q;
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
        m0 = 0;
        m1 = 0;
        m2 = 1;
    }

    void MakeHighpass(float omega, float Q) {
        float g = std::tan(omega / 2);
        float k = 1.0f / Q;
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
        m0 = 1;
        m1 = -k;
        m2 = -1;
    }

    void MakeBandpass(float omega, float Q) {
        float g = std::tan(omega / 2);
        float k = 1.0f / Q;
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
        m0 = 0;
        m1 = 1;
        m2 = 0;
    }

    void MakeNormalizedBandpass(float omega, float Q) {
        float g = std::tan(omega / 2);
        float k = 1.0f / Q;
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
        m0 = 0;
        m1 = k;
        m2 = 0;
    }

    void MakeNotch(float omega, float Q) {
        float g = std::tan(omega / 2);
        float k = 1.0f / Q;
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
        m0 = 1;
        m1 = -k;
        m2 = 0;
    }

    void MakePeak(float omega, float Q) {
        float g = std::tan(omega / 2);
        float k = 1.0f / Q;
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
        m0 = 1;
        m1 = -k;
        m2 = -2;
    }

    void MakeAllpass(float omega, float Q) {
        float g = std::tan(omega / 2);
        float k = 1.0f / Q;
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
        m0 = 1;
        m1 = -2 * k;
        m2 = -0;
    }

    void MakeFromBiquad(float b0, float b1, float b2, float ba1, float ba2) {
        float v1 = std::sqrt(-1.0f - ba1 - ba2);
        float v2 = std::sqrt(-1.0f + ba1 - ba2);
        float g = v1 / v2;
        float k = 2 * (-1.0f + ba2) / (v1 * v2);
        m0 = (b0 - b1 + b2) / (1.0f - ba1 + ba2);
        m1 = 2 * (b0 - b2) / (v1 * v2);
        m2 = (b0 + b1 + b2) / (1 + ba1 + ba2);
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }
};
}