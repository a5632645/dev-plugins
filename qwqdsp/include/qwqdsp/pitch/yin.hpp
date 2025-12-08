#pragma once
#include <vector>
#include <span>
#include <cmath>
#include "qwqdsp/spectral/oouras_real_fft.hpp"
#include "qwqdsp/pitch/pitch.hpp"

namespace qwqdsp_pitch {
/**
 * @ref https://zhuanlan.zhihu.com/p/1932764646655390362
 */
class Yin {
public:
    void Init(float fs, int block_size) {
        fs_ = fs;
        block_size_ = block_size;
        SetMinPitch(min_pitch_);
        SetMaxPitch(max_pitch_);

        delta_corr_.resize(block_size);
        fft_in_buffer_.resize(block_size*2);
        fft_out_buffer_.resize(block_size*2+2);
        fft_.Init(block_size*2);
    }

    void Process(std::span<float const> block) noexcept {
        int num_samples = static_cast<int>(block.size());

        // step1 delta auto correlation
        // this is a linear autocorrelation x[0:N] and x[0:N]
        {
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

            for (int i = 0; i < num_samples; ++i) {
                delta_corr_[0] += block[i] * block[i];
            }
            for (int tau = 1; tau < num_samples; ++tau) {
                delta_corr_[tau] = delta_corr_[tau - 1] - block[tau - 1] * block[tau - 1];
            }

            float const first_power = delta_corr_[0];
            for (int i = 0; i < num_samples; ++i) {
                delta_corr_[i] = first_power + delta_corr_[i] - 2 * fft_in_buffer_[i];
            }
        }

        int max_tal = num_samples;
        // step2 CMNDF
        {
            float sum = 0.0f;
            delta_corr_[0] = 1;
            for (int tal = 1; tal < max_tal; ++tal) {
                sum += delta_corr_[tal];
                if (sum != 0.0f) {
                    delta_corr_[tal] *= tal / sum;
                }
                else {
                    delta_corr_[tal] = 1.0f;
                }
            }
        }

        // step3 find tau
        int max_ifbin = std::min(max_bin_, max_tal - 1);
        int where = -1;
        for (int i = min_bin_; i < max_ifbin; ++i) {
            if (delta_corr_[i] < threshold_ && delta_corr_[i] < delta_corr_[i + 1]) {
                where = i;
                break;
            }
        }
        if (where == -1) {
            float min = delta_corr_.front();
            for (int i = min_bin_; i < max_ifbin; ++i) {
                if (delta_corr_[i] < min) {
                    min = delta_corr_[i];
                    where = i;
                }
            }
        }

        // step4 parabola interpolation
        float preiod = where;
        if (where > 0 && where < max_tal - 1) {
            float s0 = delta_corr_[where - 1];
            float s1 = delta_corr_[where];
            float s2 = delta_corr_[where + 1];
            if (s1 < s0 && s1 < s2) {
                float frac = 0.5f * (s2 - s0) / (2.0f * s1 - s2 - s0 + 1e-18f);
                preiod = where + frac;
                pitch_.pitch_hz = fs_ / preiod;
                pitch_.non_period_ratio = delta_corr_[where];
            }
            else {
                // 无峰值，大概是噪声或者在外面吧
                pitch_.non_period_ratio = 1.0f;
            }
        }
        else {
            // 在两侧，可能是噪声
            pitch_.non_period_ratio = 1.0f;
        }
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

    void SetThreshold(float threshold) noexcept {
        threshold_ = threshold;
    }
private:
    qwqdsp_spectral::OourasRealFFT fft_;
    std::vector<float> fft_in_buffer_;
    std::vector<float> fft_out_buffer_;
    std::vector<float> delta_corr_;
    float fs_{};
    Pitch pitch_{};
    float threshold_{0.2f};
    float min_pitch_{50.0f};
    float max_pitch_{500.0f};
    int min_bin_{};
    int max_bin_{};
    int block_size_{};
};
}
