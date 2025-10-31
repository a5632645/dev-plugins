#pragma once
#include <algorithm>
#include <cstddef>
#include <vector>
#include <span>
#include <cmath>
#include "qwqdsp/spectral/real_fft.hpp"
#include "qwqdsp/pitch/pitch.hpp"

namespace qwqdsp::pitch {
/**
 * @ref https://github.com/JorenSix/TarsosDSP/blob/master/core/src/main/java/be/tarsos/dsp/pitch/FastYin.java#L197
 */
class FastYin {
public:
    void Init(float fs, int size) {
        fs_ = fs;
        delta_corr_.resize(size / 2);
        powers_.resize(size / 2);
        temp_.resize(size);
        SetMinPitch(min_pitch_);
        SetMaxPitch(max_pitch_);
        fft_.Init(size);
        fft1_.resize(fft_.NumBins());
        fft2_.resize(fft_.NumBins());
    }

    void Process(std::span<const float> block) noexcept {
        int num_samples = block.size();
        int max_tal = num_samples / 2;

        // step1 delta auto correlation(x[0:N/2] and x[0:N])
        // using x = block, len(x) = N
        // using y = difference correlation, len(y) = N/2
        // y[tau] = sum (x[i]-x[tau+i])^2, i from 0 to N/2
        //        = sum x[i]^2 + sum x[tau+i]^2 - 2*sum x[i]*x[i+tau]
        {
            std::fill(powers_.begin(), powers_.end(), 0.0f);
            // powers[0] = sum x[i]^2, i from 0 to N/2
            for (int i = 0; i < max_tal; ++i) {
                powers_[0] += block[i] * block[i];
            }

            // powers[tau] = sum x[i+tau]^2 = sum x[j]^2, j from tau to N/2+tau
            for (int tau = 1; tau < max_tal; ++tau) {
                powers_[tau] = powers_[tau - 1] - block[tau - 1] * block[tau - 1] + block[tau + max_tal] * block[tau + max_tal];
            }

            // fft circular correlation of x[0:N/2] and x[tau:N/2+tau]
            fft_.FFT(block, fft1_);
            for (int i = 0; i < max_tal; ++i) {
                temp_[i] = block[max_tal - 1 - i]; // why reverse x[0:N/2]?
                temp_[i + max_tal] = 0;
            }
            fft_.FFT(temp_, fft2_);

            const size_t num_bins = fft_.NumBins();
            for (size_t i = 0; i < num_bins; ++i) {
                float real = fft1_[i].real() * fft2_[i].real() - fft1_[i].imag() * fft2_[i].imag();
                float imag = fft1_[i].imag() * fft2_[i].real() + fft1_[i].real() * fft2_[i].imag();
                fft1_[i] = {real, imag};
            }
            fft_.IFFT(temp_, fft1_);

            for (int i = 0; i < max_tal; ++i) {
                delta_corr_[i] = powers_[0] + powers_[i] - 2 * temp_[max_tal - 1 + i]; // why reverse correlation
            }
        }

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
        max_bin_ = std::round(fs_ / min_val);
    }

    void SetMaxPitch(float max_val) noexcept {
        max_pitch_ = max_val;
        min_bin_ = std::round(fs_ / max_val);
    }

    void SetThreshold(float thredshold) noexcept {
        threshold_ = thredshold;
    }
private:
    std::vector<float> delta_corr_;
    std::vector<float> powers_;
    std::vector<float> temp_;

    spectral::RealFFT fft_;
    std::vector<std::complex<float>> fft1_;
    std::vector<std::complex<float>> fft2_;

    float fs_{};
    float threshold_{0.2f};
    Pitch pitch_{};
    float min_pitch_{50.0f};
    float max_pitch_{500.0f};
    int min_bin_{};
    int max_bin_{};
};
}