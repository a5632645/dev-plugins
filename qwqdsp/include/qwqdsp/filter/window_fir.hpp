#pragma once
#include <cassert>
#include <numeric>
#include <span>
#include <numbers>
#include <cmath>

namespace qwqdsp_filter {
/**
 * @ref https://ccrma.stanford.edu/~jos/WinFlt/WinFlt_2up.pdf
 */
struct WindowFIR {
    static void Lowpass(std::span<float> x, float wc) noexcept {
        float center = (static_cast<float>(x.size()) - 1.0f) / 2.0f;
        for (size_t i = 0; i < x.size(); ++i) {
            float t = static_cast<float>(i) - center;
            x[i] = Sinc(wc, t);
        }
    }

    static void Highpass(std::span<float> x, float wc) noexcept {
        Lowpass(x, std::numbers::pi_v<float> - wc);
        for (size_t i = 0; i < x.size(); ++i) {
            if (i % 2 == 0) {
                x[i] = -x[i];
            }
        }
    }

    static void Bandpass(std::span<float> x, float w1, float w2) noexcept {
        if (w1 > w2) {
            std::swap(w1, w2);
        }
        float center_w = (w1 + w2) * 0.5f;
        float half_bw = w2 - center_w;
        Lowpass(x, half_bw);
        float center = (static_cast<float>(x.size()) - 1.0f) / 2.0f;
        for (size_t i = 0; i < x.size(); ++i) {
            x[i] *= 2 * std::cos((static_cast<float>(i) - center) * center_w);
        }
    }

    /**
     * @note 偶数长度会产生非线性相位
     */
    static void Bandstop(std::span<float> x, float w1, float w2) noexcept {
        if (w1 > w2) {
            std::swap(w1, w2);
        }
        float center = (static_cast<float>(x.size()) - 1.0f) / 2.0f;
        for (size_t i = 0; i < x.size(); ++i) {
            float t = static_cast<float>(i) - center;
            x[i] = Sinc(w1, t);
        }
        float highpass_w = std::numbers::pi_v<float> - w2;
        for (size_t i = 0; i < x.size(); ++i) {
            float t = static_cast<float>(i) - center;
            if (i % 2 == 0) {
                x[i] -= Sinc(highpass_w, t);
            }
            else {
                x[i] += Sinc(highpass_w, t);
            }
        }
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
