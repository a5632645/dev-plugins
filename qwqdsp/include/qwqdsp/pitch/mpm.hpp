/**
 *  MIT License
 * 
 * Copyright (c) 2018 Sevag Hanssian
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * https://github.com/sevagh/pitch-detection/blob/master/src/mpm.cpp
 */

#pragma once
#include <span>
#include <vector>
#include "qwqdsp/spectral/oouras_real_fft.hpp"
#include "qwqdsp/pitch/pitch.hpp"

namespace qwqdsp_pitch {
class MPM {
public:
    void Init(float fs, size_t block_size) {
        fs_ = fs;
        block_size_ = static_cast<int>(block_size);
        SetMinPitch(min_pitch_);
        SetMaxPitch(max_pitch_);

        fft_in_buffer_.resize(block_size*2);
        fft_out_buffer_.resize(block_size*2+2);
        fft_.Init(block_size*2);
        max_positions_.resize(block_size);
        estimates_.resize(block_size);
    }
    
    void Process(std::span<const float> block) noexcept {
        std::copy_n(block.begin(), block.size(), fft_in_buffer_.begin());
        std::fill_n(fft_in_buffer_.begin() + block.size(), block.size(), 0);
        fft_.FFT(fft_in_buffer_.data(), fft_out_buffer_.data());
        size_t const num_bins = fft_.GetFFTSize() / 2 + 1;
        for (size_t i = 0; i < num_bins; ++i) {
            float re = fft_out_buffer_[2*i];
            float im = fft_out_buffer_[2*i + 1];
            fft_out_buffer_[2*i] = (re*re + im*im);
            fft_out_buffer_[2*i + 1]=0;
        }
        fft_.IFFT(fft_out_buffer_.data(), fft_in_buffer_.data());

        auto const& autocorrelation = fft_in_buffer_;
        peak_picking(autocorrelation);

        float highest_amplitude = -FLT_MAX;
        estimates_.clear();
        for (int i : max_positions_) {
            highest_amplitude =
                std::max(highest_amplitude, autocorrelation[static_cast<size_t>(i)]);
            if (fft_in_buffer_[static_cast<size_t>(i)] > MPM_SMALL_CUTOFF) {
                auto x = parabolic_interpolation(autocorrelation, static_cast<size_t>(i));
                estimates_.push_back(x);
                highest_amplitude = std::max(highest_amplitude, std::get<1>(x));
            }
        }

        if (estimates_.empty()) {
            pitch_.non_period_ratio = 1;
            return;
        }

        float actual_cutoff = MPM_CUTOFF * highest_amplitude;
        float period = 0;

        for (auto i : estimates_) {
            if (std::get<1>(i) >= actual_cutoff) {
                period = std::get<0>(i);
                break;
            }
        }

        float pitch_estimate = fs_ / period;
        if (pitch_estimate > MPM_LOWER_PITCH_CUTOFF) {
            pitch_.pitch_hz = pitch_estimate;
        }
        pitch_.non_period_ratio = GetNonPeriodRatio(block, period);
    }

    Pitch GetPitch() const noexcept {
        return pitch_;
    }

    void SetMinPitch(float min_val) noexcept {
        min_pitch_ = min_val;
        max_bin_ = static_cast<int>(std::round(fs_ / min_val));
        max_bin_ = std::min(max_bin_, block_size_);
    }

    void SetMaxPitch(float max_val) noexcept {
        max_pitch_ = max_val;
        min_bin_ = static_cast<int>(std::round(fs_ / max_val));
        min_bin_ = std::max(min_bin_, 2);
    }
private:
    static constexpr float MPM_CUTOFF = 0.93f;
    static constexpr float MPM_SMALL_CUTOFF = 0.5f;
    static constexpr float MPM_LOWER_PITCH_CUTOFF = 80.0f;
    static constexpr float PMPM_PA = 0.01f;
    static constexpr int PMPM_N_CUTOFFS = 20;
    static constexpr float PMPM_PROB_DIST = 0.05f;
    static constexpr float PMPM_CUTOFF_BEGIN = 0.8f;
    static constexpr float PMPM_CUTOFF_STEP = 0.01f;

    qwqdsp_spectral::OourasRealFFT fft_;
    std::vector<float> fft_in_buffer_;
    std::vector<float> fft_out_buffer_;
    std::vector<int> max_positions_;
    std::vector<std::pair<float, float>> estimates_;
    float fs_{};
    Pitch pitch_{};
    float min_pitch_{50.0f};
    float max_pitch_{500.0f};
    int min_bin_{};
    int max_bin_{};
    int block_size_{};

    // get non_period_ratio using YIN's CMNDF defination
    float GetNonPeriodRatio(std::span<float const> block, float period) noexcept {
        // parabola interpolation should give [x-0.5,x+0.5]
        size_t const tau = static_cast<size_t>(period + 0.5f);
        if (tau == 0 || tau == block.size()) {
            return 1.0f;
        }

        float first_power = 0;
        for (float x : block) {
            first_power += x*x;
        }

        float sum = 0;
        float power_tau = first_power;
        float delta_corr = 0;
        for (size_t i = 1; i < tau; ++i) {
            power_tau -= block[i - 1] * block[i - 1];
            delta_corr = first_power + power_tau - 2 * fft_in_buffer_[i];
            sum += delta_corr;
        }
        if (sum != 0.0f) {
            return delta_corr * tau / sum;
        }
        else {
            return 1.0f;
        }
    }

    void peak_picking(const std::vector<float> &nsdf) noexcept
    {
        max_positions_.clear();

        int pos = min_bin_;
        int cur_max_pos = 0;
        int size = max_bin_;

        while (pos < (size - 1) / 3 && nsdf[static_cast<size_t>(pos)] > 0)
            pos++;
        while (pos < size - 1 && nsdf[static_cast<size_t>(pos)] <= 0.0)
            pos++;

        if (pos == 0)
            pos = 1;

        while (pos < size - 1) {
            if (nsdf[static_cast<size_t>(pos)] > nsdf[static_cast<size_t>(pos) - 1] && nsdf[static_cast<size_t>(pos)] >= nsdf[static_cast<size_t>(pos) + 1] &&
                (cur_max_pos == 0 || nsdf[static_cast<size_t>(pos)] > nsdf[static_cast<size_t>(cur_max_pos)])) {
                cur_max_pos = pos;
            }
            pos++;
            if (pos < size - 1 && nsdf[static_cast<size_t>(pos)] <= 0) {
                if (cur_max_pos > 0) {
                    max_positions_.push_back(cur_max_pos);
                    cur_max_pos = 0;
                }
                while (pos < size - 1 && nsdf[static_cast<size_t>(pos)] <= 0.0) {
                    pos++;
                }
            }
        }
        if (cur_max_pos > 0) {
            max_positions_.push_back(cur_max_pos);
        }
    }

    static std::pair<float, float> parabolic_interpolation(const std::vector<float> &array, size_t x) noexcept {
        size_t x_adjusted;

        if (x < 1) {
            x_adjusted = (array[x] <= array[x + 1]) ? x : x + 1;
        }
        else if (x > array.size() - 1) {
            x_adjusted = (array[x] <= array[x - 1]) ? x : x - 1;
        }
        else {
            float den = array[x + 1] + array[x - 1] - 2 * array[x];
            float delta = array[x - 1] - array[x + 1];
            return (den != 0.0f)
                    ? std::make_pair(static_cast<float>(x), array[x])
                    : std::make_pair(static_cast<float>(x) + delta / (2 * den),
                            array[x] - delta * delta / (8 * den));
        }
        return std::make_pair(static_cast<float>(x_adjusted), array[x_adjusted]);
    }
};
}
