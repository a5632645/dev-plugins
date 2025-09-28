#pragma once
#include <complex>
#include <cstddef>
#include <numbers>
#include <numeric>
#include <vector>
#include "qwqdsp/window/hamming.hpp"
#include "qwqdsp/window/helper.hpp"
#include "qwqdsp/spectral/real_fft.hpp"

namespace qwqdsp::spectral {
/**
 * @ref https://github.com/bzamecnik/tfr/blob/master/tfr/reassignment.py
 */
class Reassignment {
public:
    void Init(size_t fft_size) {
        fft_.Init(fft_size);
        buffer_.resize(fft_size);
        window_.resize(fft_size);
        common_.resize(fft_.NumBins());
        time_.resize(fft_.NumBins());
        frequency_.resize(fft_.NumBins());
        ChangeWindow([](auto win) {
            window::Hamming::Window(win, true);
        });
    }

    /**
     * @tparam func void(std::span<float> window)
     */
    template<class Func>
    void ChangeWindow(Func&& func) noexcept(noexcept(func(std::declval<std::span<float>>()))) {
        func(std::span<float>{window_});
        gain_scaleback_ = std::accumulate(window_.begin(), window_.end(), 0.0f) / 2.0f;
        window::Helper::Normalize(window_);
    }

    void Process(std::span<const float> time) noexcept {
        const size_t n = fft_.FFTSize();
        for (size_t i = 0; i < n; ++i) {
            buffer_[i] = time[i] * window_[i];
        }
        fft_.FFT(buffer_, common_);

        buffer_[0] = 0.0f;
        for (size_t i = 1; i < n; ++i) {
            buffer_[i] = time[i - 1] * window_[i];
        }
        fft_.FFT(buffer_, frequency_);

        std::copy(common_.begin(), common_.end(), time_.begin());
        for (int i = time_.size() - 1; i > 0; --i) {
            time_[i] = time_[i - 1];
        }
        time_[0] = std::complex<float>{};

        const size_t num_bins = fft_.NumBins();
        for (size_t i = 0; i < num_bins; ++i) {
            time_[i] = common_[i] * std::conj(time_[i]);
            frequency_[i] = common_[i] * std::conj(frequency_[i]);
        }
    }

    float GetGain(size_t bin) const noexcept {
        return std::abs(common_[bin]);
    }

    void GetGain(std::span<float> gain) const noexcept {
        for (size_t i = 0; i < gain.size(); ++i) {
            gain[i] = std::abs(common_[i]);
        }
    }

    /**
     * @return 0 ~ 1 or 0 ~ fs
     */
    float GetFrequency(size_t bin) const noexcept {
        return Arg(frequency_[bin]);
    }

    void GetFrequency(std::span<float> freq) const noexcept {
        for (size_t i = 0; i < freq.size(); ++i) {
            freq[i] = Arg(frequency_[i]);
        }
    }

    /**
     * @return -0.5 ~ 0.5
     */
    float GetTime(size_t idx) const noexcept {
        return 0.5f - Arg(time_[idx]);
    }

    void GetTime(std::span<float> time) const noexcept {
        for (size_t i = 0; i < time.size(); ++i) {
            time[i] = 0.5f - Arg(time_[i]);
        }
    }

    size_t NumData() const noexcept {
        return fft_.NumBins();
    }

    float GetGainScaleback() const noexcept {
        return gain_scaleback_;
    }
private:
    static float Arg(std::complex<float> z) noexcept {
        float a = std::arg(z);
        a /= 2.0f * std::numbers::pi_v<float>;
        return std::fmod(a, 1.0f);
    }

    float gain_scaleback_{};
    RealFFT fft_;
    std::vector<float> buffer_;
    std::vector<float> window_;
    std::vector<std::complex<float>> common_;
    std::vector<std::complex<float>> time_;
    std::vector<std::complex<float>> frequency_;
};

class ReassignmentCorrect {
public:
    void Init(size_t fft_size) {
        fft_.Init(fft_size);
        buffer_.resize(fft_size);
        xh_data_.resize(fft_.NumBins());
        xdh_data_.resize(fft_.NumBins());
        xth_data_.resize(fft_.NumBins());
        window_.resize(fft_size);
        dwindow_.resize(fft_size);
        twindow_.resize(fft_size);
        ChangeWindow([](auto win, auto dwin) {
            window::Hamming::Window(win, true);
            window::Hamming::DWindow(dwin);
        });
    }

    /**
     * @tparam func void(std::span<float> window, std::span<float> dwindow)
     */
    template<class Func>
    void ChangeWindow(Func&& func) noexcept(noexcept(func(std::declval<std::span<float>>(), std::declval<std::span<float>>()))) {
        func(std::span<float>{window_}, std::span<float>{dwindow_});
        window::Helper::TWindow(twindow_, window_);
        window_scale_ = window::Helper::NormalizeGain(window_);
        dwindow_scale_ = window_scale_ / (2.0f * std::numbers::pi_v<float>);
    }

    void Process(std::span<const float> time) noexcept {
        const size_t fft_size = fft_.FFTSize();
        for (size_t i = 0; i < fft_size; ++i) {
            buffer_[i] = time[i] * window_[i];
        }
        fft_.FFT(buffer_, xh_data_);

        for (size_t i = 0; i < fft_size; ++i) {
            buffer_[i] = time[i] * dwindow_[i];
        }
        fft_.FFT(buffer_, xdh_data_);

        for (size_t i = 0; i < fft_size; ++i) {
            buffer_[i] = time[i] * twindow_[i];
        }
        fft_.FFT(buffer_, xth_data_);
    }

    float GetFrequency(size_t idx) const noexcept {
        auto xdh = xdh_data_[idx] * dwindow_scale_;
        auto xh = xh_data_[idx] * window_scale_;
        auto up = xdh.imag() * xh.real() - xdh.real() * xh.imag();
        auto down = std::norm(xh);
        auto freq_c = -up / down;
        return (idx + freq_c) / fft_.FFTSizeFloat();
    }

    void GetFrequency(std::span<float> freq) const noexcept {
        for (size_t i = 0; i < freq.size(); ++i) {
            freq[i] = GetFrequency(i);
        }
    }

    /**
     * @return 0 ~ 1
     */
    float GetTime(size_t idx) const noexcept {
        auto X_h = xh_data_[idx] * window_scale_;
        auto X_Th = xth_data_[idx] * dwindow_scale_;

        auto num = X_h.real() * X_Th.real() + X_h.imag() * X_Th.imag();
        auto magSquared = norm(X_h);

        return num * std::numbers::pi_v<float> / magSquared;
    }

    void GetTime(std::span<float> time) const noexcept {
        for (size_t i = 0; i < time.size(); ++i) {
            time[i] = GetTime(i);
        }
    }

    float GetGain(size_t idx) const noexcept {
        return std::abs(xh_data_[idx]) * window_scale_;
    }

    void GetGain(std::span<float> gain) const noexcept {
        for (size_t i = 0; i < gain.size(); ++i) {
            gain[i] = GetGain(i);
        }
    }

    size_t NumData() const noexcept {
        return fft_.NumBins();
    }
private:
    RealFFT fft_;
    std::vector<float> buffer_;
    std::vector<float> window_;
    std::vector<float> dwindow_;
    std::vector<float> twindow_;
    std::vector<std::complex<float>> xh_data_;
    std::vector<std::complex<float>> xdh_data_;
    std::vector<std::complex<float>> xth_data_;
    float window_scale_{};
    float dwindow_scale_{};
};
}