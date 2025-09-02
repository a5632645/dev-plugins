#pragma once
#include <vector>
#include <span>
#include <cmath>

namespace qwqdsp::pitch {
/**
 * @ref http://recherche.ircam.fr/equipes/pcm/cheveign/ps/2002_JASA_YIN_proof.pdf
 */
class Yin {
public:
    void Init(float fs, int size) {
        fs_ = fs;
        delta_corr_.resize(size);
        dicimate_ = std::round(fs / 6000.0f);
        SetMinPitch(min_pitch_);
        SetMaxPitch(max_pitch_);
    }

    void Process(std::span<float> block) noexcept {
        int num_samples = block.size();
        int max_tal = num_samples / 2;

        // step1 delta auto correlation
        for (int i = 0; i < max_tal; ++i) {
            float sum = 0.0f;
            for (int j = 0; j < num_samples; j += dicimate_) {
                float a = block[j];
                float b = block[(i + j) % num_samples];
                sum += (a - b) * (a - b);
            }
            delta_corr_[i] = sum;
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
                pitch_.pitch = fs_ / preiod;
                pitch_.non_period_ratio = delta_corr_[where];
            }
            else {
                // 无峰值，大概是噪声或者在外面吧
                pitch_.pitch = 0.0f;
                pitch_.non_period_ratio = 1.0f;
            }
        }
        else {
            // 在两侧，可能是噪声
            pitch_.pitch = 0.0f;
            pitch_.non_period_ratio = 1.0f;
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
    }

    void SetMaxPitch(float max_val) noexcept {
        max_pitch_ = max_val;
        min_bin_ = std::round(fs_ / max_val);
    }

    void SetThreshold(float threshold) noexcept {
        threshold_ = threshold;
    }
private:
    std::vector<float> delta_corr_;
    float fs_{};
    Result pitch_{};
    int dicimate_{};
    float threshold_{0.15f};
    float min_pitch_{};
    float max_pitch_{};
    int min_bin_{};
    int max_bin_{};
};
}