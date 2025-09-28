#pragma once
#include <vector>
#include <span>
#include <cassert>
#include "qwqdsp/interpolation.hpp"
#include "qwqdsp/window/kaiser.hpp"

namespace qwqdsp::fx {
class Resample {
public:
    /**
     * @param atten (>0)dB 这决定了target_fs/2处的衰减值
     * @param kernel_len >=3 必须是奇数，越大过渡带越小，计算量越大
     * @param oversample >0 越大质量越好，内存需求越大
     */
    void Init(float source_fs, float target_fs, float atten, size_t kernel_len, size_t oversample) {
        assert(kernel_len % 2 == 1);

        phase_inc_ = source_fs / target_fs;
        kernel_len_ = kernel_len;
        oversample_plus1_ = oversample + 1;

        size_t const lut_size = kernel_len + (kernel_len - 1) * oversample;
        kernel_.resize(lut_size + 1, 0.0f);
        float const beta = qwqdsp::window::Kaiser::Beta(atten);
        float const width = qwqdsp::window::Kaiser::MainLobeWidth(beta) * std::numbers::pi_v<float> * 2.0f / static_cast<float>(kernel_len);
        
        std::span kernel_block{kernel_.data(), lut_size};
        {
            float cutoff = 0.0f;
            if (target_fs < source_fs) {
                cutoff = std::numbers::pi_v<float> * target_fs / source_fs - width;
            }
            else {
                cutoff = std::numbers::pi_v<float> - width;
            }

            float const center = (static_cast<float>(kernel_block.size()) - 1.0f) / 2.0f;
            float const omega = cutoff / static_cast<float>(oversample_plus1_);
            for (size_t i = 0; i < kernel_block.size(); ++i) {
                float t = static_cast<float>(i) - center;
                [[unlikely]]
                if (t == 0.0f) {
                    kernel_block[i] = cutoff / std::numbers::pi_v<float>;
                }
                else {
                    kernel_block[i] = std::sin(omega * t) * static_cast<float>(oversample_plus1_) / (std::numbers::pi_v<float> * t);
                }
            }
        }

        qwqdsp::window::Kaiser::ApplyWindow(kernel_block, beta, false);
    }

    std::vector<float> Process(std::span<float> x) {
        std::vector<float> r;

        float phase = 0.0f;
        int const ikernel_len = static_cast<int>(kernel_len_);
        int const xsize = static_cast<int>(x.size());
        int const half_len = (static_cast<int>(kernel_len_) - 1) / 2;
        int xrpos = -(static_cast<int>(kernel_len_) - 1) / 2;

        while (xrpos < xsize - half_len) {
            float sum{};

            if (phase == 0.0f) {
                int const begin = std::max(0, -xrpos);
                int const xibegin = std::max(0, xrpos);
                int const len = std::min(ikernel_len, xsize - xibegin);
                for (int i = begin; i < len; ++i) {
                    size_t const krpos = static_cast<size_t>(i) * oversample_plus1_;
                    float const kernel_v = kernel_[krpos];
                    sum += kernel_v * x[static_cast<size_t>(xibegin + i)];
                }
            }
            else {
                float const frac = 1.0f - phase;
                int const begin = std::max(0, -xrpos);
                int const xibegin = std::max(0, xrpos + 1);
                int const len = std::min(ikernel_len - 1, xsize - xibegin);
                for (int i = begin; i < len; ++i) {
                    float const krpos = static_cast<float>(static_cast<size_t>(i) * oversample_plus1_) + frac * static_cast<float>(oversample_plus1_);
                    size_t const ikrpos = static_cast<size_t>(krpos);
                    float const frac_krpos = krpos - std::floor(krpos);
                    float const kernel_v = qwqdsp::Interpolation::Linear(kernel_[ikrpos], kernel_[ikrpos + 1], frac_krpos);
                    sum += kernel_v * x[static_cast<size_t>(xibegin + i)];
                }
            }
            r.push_back(sum);

            phase += phase_inc_;
            xrpos += static_cast<int>(std::floor(phase));
            phase = phase - std::floor(phase);
        }

        return r;
    }

private:
    float phase_inc_{};
    size_t oversample_plus1_{};
    size_t kernel_len_{};
    std::vector<float> kernel_;
};
}