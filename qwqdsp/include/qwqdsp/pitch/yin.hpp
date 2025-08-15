#pragma once
#include <vector>
#include <span>
#include <cmath>

namespace qwqdsp::pitch {
class Yin {
public:
    void Init(float fs, int size) {
        fs_ = fs;
        delta_corr_.resize(size);
        dicimate_ = std::round(fs / 6000.0f);
    }

    void Process(std::span<float> block) {
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
                delta_corr_[tal] = delta_corr_[tal] / (sum / tal + 1e-18f);
            }
        }

        // step3 find tau
        constexpr float non_period_energy_ratio = 0.1f;
        int where = -1;
        for (int i = 0; i < max_tal; ++i) {
            if (delta_corr_[i] < non_period_energy_ratio) {
                where = i;
                break;
            }
        }
        if (where == -1) {
            float min = delta_corr_.front();
            for (int i = 0; i < max_tal; ++i) {
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
            float frac = 0.5f * (s2 - s0) / (2.0f * s1 - s2 - s0 + 1e-18f);
            preiod = where + frac;
        }
        pitch_ = fs_ / preiod;
    }

    float GetPitch() const {
        return pitch_;
    }
private:
    std::vector<float> delta_corr_;
    float fs_{};
    float pitch_{};
    int dicimate_{};
};
}