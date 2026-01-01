#pragma once
#include <span>
#include <array>
#include <numbers>
#include <cmath>
#include <qwqdsp/spectral/ipp_real_fft.hpp>
#include <qwqdsp/segement/analyze_synthsis_online.hpp>
#include <qwqdsp/window/hann.hpp>
#include <qwqdsp/window/blackman.hpp>

namespace qwqdsp_fx {
class PhaseVocoder {
public:
    static constexpr size_t kHop = 256;
    static constexpr size_t kFFT = 2048;
    static constexpr size_t kBins = kFFT / 2 + 1;

    PhaseVocoder() {
        segement_.SetHop(kHop);
        segement_.SetSize(kFFT);
        fft_.Init(kFFT);
        qwqdsp_window::Hann::Window(hann_window_, true);
        qwqdsp_window::Blackman::Window(analyze_window_, true);
    }

    void Reset() {
        segement_.Reset();
        inited_ = false;
    }

    void Process(float* ptr, size_t num_samples) {
        segement_.Process({ptr, num_samples}, *this);
    }

    void operator()(std::span<const float> input, std::span<float> output) {
        for (size_t i = 0; i < kFFT; ++i) {
            output[i] = input[i] * analyze_window_[i];
        }
        fft_.FFT(output.data(), reinterpret_cast<float*>(reim_.data()));

        if (inited_) {
            std::array<float, kBins> detail_freq;
            std::array<float, kBins> detail_gain;
            for (size_t i = 0; auto[re, im] : reim_) {
                float curr_phase = std::atan2(im, re);
                float last_phase = last_phase_[i];
                last_phase_[i] = curr_phase;
                float omega = static_cast<float>(i) * (std::numbers::pi_v<float> * 2.0f) / static_cast<float>(kFFT);
                detail_freq[i] = WrapPi(curr_phase - (last_phase + omega * kHop)) / kHop + omega;
                detail_gain[i] = std::sqrt(re * re + im * im);
                ++i;
            }

            float omega_per_bin = (std::numbers::pi_v<float> * 2.0f) / static_cast<float>(kFFT);
            float freq_mul = std::exp2(pitch_shift / 12.0f);
            std::array<float, kBins> synthsis_amp{};
            std::array<float, kBins> synthsis_freq{};
            for (size_t i = 0; i < kBins; ++i) {
                float f = detail_freq[i] * freq_mul;
                size_t idx = f / omega_per_bin;
                if (idx < kBins && synthsis_amp[idx] < detail_gain[i]) {
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
                reim_[i][0] = g * std::cos(p);
                reim_[i][1] = g * std::sin(p);
            }
            fft_.IFFT(reinterpret_cast<float*>(reim_.data()), output.data());
            for (size_t i = 0; i < kFFT; ++i) {
               output[i] *= hann_window_[i];
            }
        }
        else {
            inited_ = true;

            auto it = last_phase_.begin();
            for (auto[re, im] : reim_) {
                *it = std::atan2(im, re);
                ++it;
            }

            std::copy(last_phase_.begin(), last_phase_.end(), synthsis_phase_.begin());
            std::fill(output.begin(), output.end(), 0.0f);
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

    bool inited_{false};
    std::array<float, kFFT> hann_window_;
    std::array<float, kFFT> analyze_window_;
    std::array<std::array<float, 2>, kBins> reim_;
    std::array<float, kBins> last_phase_;
    std::array<float, kBins> synthsis_phase_;
    qwqdsp_segement::AnalyzeSynthsisOnline segement_;
    qwqdsp_spectral::IppRealFFT fft_;
};
} // namespace qwqdsp_fx
