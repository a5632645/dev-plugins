#pragma once
#include <span>
#include <array>
#include <numbers>
#include <cmath>
#include <complex>
#include <qwqdsp/spectral/ipp_real_fft.hpp>
#include <qwqdsp/segement/analyze_synthsis_online.hpp>
#include <qwqdsp/window/hann.hpp>
#include <qwqdsp/window/blackman.hpp>

namespace qwqdsp_fx {
class PhaseVocoder {
public:
    static constexpr size_t kHop = 128;
    static constexpr size_t kFFT = 2048;
    static constexpr size_t kBins = kFFT / 2 + 1;

    PhaseVocoder() {
        segement_.SetHop(kHop);
        segement_.SetSize(kFFT);
        fft_.Init(kFFT);
        qwqdsp_window::Hann::Window(hann_window_, true);
        qwqdsp_window::Hann::Window(analyze_window_, true);
    }

    void Reset() {
        segement_.Reset();
    }

    void Process(float* left, float* right, size_t num_samples) {
        segement_.Process({left, num_samples}, *this);
        std::copy_n(left, num_samples, right);
    }

    void operator()(std::span<const float> input, std::span<float> output) {
        std::array<std::array<float, 2>, kBins> reim;
        std::array<std::array<float, 2>, kBins> reim2;
        for (size_t i = 0; i < kFFT; ++i) {
            output[i] = input[i] * analyze_window_[i];
        }
        fft_.FFT(output.data(), reinterpret_cast<float*>(reim.data()));
        for (size_t i = 1; i < kFFT; ++i) {
            output[i] = input[i - 1] * analyze_window_[i];
        }
        output[0] = 0.0f;
        fft_.FFT(output.data(), reinterpret_cast<float*>(reim2.data()));

        std::array<float, kBins> detail_freq;
        std::array<float, kBins> detail_gain;
        for (size_t i = 0; i < kBins; ++i) {
            std::complex common{reim[i][0], reim[i][1]};
            std::complex freq{reim2[i][0], reim2[i][1]};
            freq = common * std::conj(freq);
            float omega = std::arg(freq);
            
            detail_freq[i] = omega;
            detail_gain[i] = std::abs(common);
        }

        float omega_per_bin = (std::numbers::pi_v<float> * 2.0f) / static_cast<float>(kFFT);
        float freq_mul = std::exp2(pitch_shift / 12.0f);
        std::array<float, kBins> synthsis_amp{};
        std::array<float, kBins> synthsis_freq{};
        for (size_t i = 0; i < kBins; ++i) {
            float f = detail_freq[i] * freq_mul;
            size_t idx = static_cast<size_t>(f / omega_per_bin);
            if (idx >= 0 && idx < kBins && synthsis_amp[idx] < detail_gain[i]) {
                synthsis_amp[idx] = detail_gain[i];
                synthsis_freq[idx] = f;
            }
        }

        for (size_t i = 0; i < kBins; ++i) {
            float last = synthsis_phase_[i];
            float omega = synthsis_freq[i];
            last += omega * kHop;
            last = WrapPi(last);
            synthsis_phase_[i] = last;
        }

        for (size_t i = 0; i < kBins; ++i) {
            float g = synthsis_amp[i];
            float p = synthsis_phase_[i];
            reim[i][0] = g * std::cos(p);
            reim[i][1] = g * std::sin(p);
        }
        fft_.IFFT(reinterpret_cast<float*>(reim.data()), output.data());
        for (size_t i = 0; i < kFFT; ++i) {
            output[i] *= hann_window_[i];
        }
    }

    float pitch_shift{};
private:
    static float WrapPi(float x) {
        constexpr float pi = std::numbers::pi_v<float>;
        while (x > pi) x -= pi * 2;
        while (x < -pi) x += pi * 2;
        return x;
    }

    std::array<float, kFFT> hann_window_;
    std::array<float, kFFT> analyze_window_;
    std::array<float, kBins> synthsis_phase_{};
    qwqdsp_segement::AnalyzeSynthsisOnline segement_;
    qwqdsp_spectral::IppRealFFT fft_;
};
} // namespace qwqdsp_fx
