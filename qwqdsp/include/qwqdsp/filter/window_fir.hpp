#pragma once
#include <cassert>
#include <cstddef>
#include <numeric>
#include <span>
#include <numbers>
#include <cmath>

namespace qwqdsp::filter {
struct WindowFIR {
    static void Lowpass(std::span<float> x, float wc) noexcept {
        float center = (static_cast<float>(x.size()) - 1.0f) / 2.0f;
        for (size_t i = 0; i < x.size(); ++i) {
            float t = static_cast<float>(i) - center;
            x[i] = Sinc(wc, t);
        }
    }

    static void Highpass(std::span<float> x, float wc) noexcept {
        assert(x.size() % 2 == 1);
        float center = (static_cast<float>(x.size()) - 1.0f) / 2.0f;
        for (size_t i = 0; i < x.size(); ++i) {
            float t = static_cast<float>(i) - center;
            x[i] = -Sinc(wc, t);
        }
        x[x.size() / 2] += 1;
    }

    static void Bandpass(std::span<float> x, float w1, float w2) noexcept {
        if (w1 < w2) {
            std::swap(w1, w2);
        }
        float center = (static_cast<float>(x.size()) - 1.0f) / 2.0f;
        for (size_t i = 0; i < x.size(); ++i) {
            float t = static_cast<float>(i) - center;
            x[i] = Sinc(w2, t) - Sinc(w1, t);
        }
    }

    static void Bandstop(std::span<float> x, float w1, float w2) noexcept {
        assert(x.size() % 2 == 1);
        if (w1 < w2) {
            std::swap(w1, w2);
        }
        float center = (static_cast<float>(x.size()) - 1.0f) / 2.0f;
        for (size_t i = 0; i < x.size(); ++i) {
            float t = static_cast<float>(i) - center;
            x[i] = -(Sinc(w2, t) - Sinc(w1, t));
        }
        x[x.size() / 2] -= 1;
    }

    static void Normalize(std::span<float> x) noexcept {
        float g = 1.0f / std::accumulate(x.begin(), x.end(), 0.0f);
        for (auto& f : x) {
            f *= g;
        }
    }
private:
    static float Sinc(float wc, float x) noexcept {
        if (x == 0.0f) {
            return wc / std::numbers::pi_v<float>;
        }
        else {
            return std::sin(wc * x) / (std::numbers::pi_v<float> * x);
        }
    }
};
}