#pragma once
#include <vector>
#include <span>
#include <cmath>
#include "qwqdsp/spectral/oouras_real_fft.hpp"
#include "qwqdsp/osciilor/noise.hpp"
#include "qwqdsp/window/hann.hpp"

namespace qwqdsp::pitch {
class ACF {
public:
    void Init(float fs, size_t block_size) {
        fs_ = fs;
        buffer_.resize(block_size + 2);
        window_.resize(block_size);
        window::Hann::Window(window_, true);
        SetMinPitch(min_pitch_);
        SetMaxPitch(max_pitch_);
        fft_.Init(block_size);
    }

    void Process(std::span<const float> block) noexcept {
        fft_.FFT(block.data(), buffer_.data());
        size_t const fft_size = fft_.GetFFTSize();
        size_t const num_complex = fft_size / 2 + 1;
        float const scale = 2.0f / static_cast<float>(fft_size);
        for (size_t i = 0; i < num_complex; ++i) {
            float re = buffer_[2 * i];
            float im = buffer_[2 * i + 1];
            re = re * re + im * im;
            buffer_[2 * i] = re * scale;
            buffer_[2 * i + 1] = 0;
        }
        fft_.IFFT(buffer_.data());

        auto& acf = fft_.GetIfftOutput();
        size_t where = 0;
        float max_acf = 0;
        for (size_t i = min_bin_; i < max_bin_; ++i) {
            float prev = acf[i - 1];
            float curr = acf[i];
            float next = acf[i + 1];
            if (curr > next && curr > prev && curr > max_acf) {
                where = i;
                max_acf = curr;
            }
        }
        if (where != 0) {
            float preiod = static_cast<float>(where);
            pitch_.pitch = fs_ / preiod;
            pitch_.non_period_ratio = 0;
        }
        else {
            pitch_.pitch = 0;
            pitch_.non_period_ratio = 1;
        }
    }

    struct Result {
        float pitch;
        // larger means the result is like a noise
        float non_period_ratio;
    };
    Result GetPitch() const noexcept {
        return pitch_;
    }

    void SetMinPitch(float min_val) noexcept {
        min_pitch_ = min_val;
        max_bin_ = std::round(fs_ / min_val);
        size_t max_tau = buffer_.size() / 2;
        max_bin_ = std::min(max_tau - 1, max_bin_);
    }

    void SetMaxPitch(float max_val) noexcept {
        max_pitch_ = max_val;
        min_bin_ = std::round(fs_ / max_val);
        min_bin_ = std::max<size_t>(min_bin_, 2);
    }

    void SetThreshold(float threshold) noexcept {
        threshold_ = threshold;
    }
private:
    spectral::OourasRealFFT fft_;
    std::vector<float> buffer_;
    std::vector<float> window_;
    oscillor::WhiteNoise noise_;
    float fs_{};
    Result pitch_{};
    float threshold_{0.15f};
    float min_pitch_{};
    float max_pitch_{};
    size_t min_bin_{};
    size_t max_bin_{};
};
}