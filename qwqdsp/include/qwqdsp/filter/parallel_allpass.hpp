#pragma once
#include "qwqdsp/filter/allpass.hpp"
#include "qwqdsp/filter/rbj.hpp"

namespace qwqdsp::filter {
/**
 * @brief 双路全通滤波器组成的低通/高通滤波器，只能为奇数阶，先用巴特沃斯摸一下
 * @ref 纯极点 https://radiosystemdesign.com/assets/pdf/downloads/Reducing_IIR_Comp_Workload_Lyons.pdf
 * @ref 可零点 https://www.researchgate.net/publication/278320928_A_Most_Efficient_Digital_Filter_The_Two-Path_Recursive_All-Pass_Filter
 */
class ParallelAllpass {
public:
    void Reset() noexcept {

    }

    void BuildButterworth(size_t order, float w) noexcept {
        assert(order % 2 == 1);
        order_ = order;
        size_t const n2 = (order - 1) / 2;
        num_down2_ = (n2 + 1) / 2;
        num_up2_ = n2 - num_down2_;
        allpass_.resize(n2);
        cutoff_w_ = w;

        {
            // (s + 1)/(s - 1) -> (z^-1-e)/(1-ez^-1)
            float const t = std::tan(w / 2);
            float const e = (1 - t) / (1 + t);
            allpass1_.SetA(-e);
        }
        qwqdsp::filter::RBJ design;
        size_t down_idx = num_up2_;
        size_t up_idx = 0;
        for (size_t i = 1; i < (order_ + 1) / 2; ++i) {
            float const qw = i * std::numbers::pi_v<float> / order_;
            float const Q = 0.5f / std::cos(qw);
            design.Allpass(w, Q);
            if (i % 2 == 1) {
                // to down
                allpass_[down_idx].SetA1(design.a1);
                allpass_[down_idx++].SetA2(design.a2);
            }
            else {
                // to up
                allpass_[up_idx].SetA1(design.a1);
                allpass_[up_idx++].SetA2(design.a2);
            }
        }
    }

    // qwqfixme: 幅度谱对但是不稳定
    void BuildChebyshev1(size_t order, float w, float ripple) {
        assert(order % 2 == 1);
        order_ = order;
        size_t const n2 = (order - 1) / 2;
        num_down2_ = (n2 + 1) / 2;
        num_up2_ = n2 - num_down2_;
        if (allpass_.size() < n2) {
            allpass_.resize(n2);
        }
        cutoff_w_ = w;

        float eps = std::sqrt(std::pow(10.0f, ripple / 10.0f) - 1.0f);
        float A = 1.0f / order * std::asinh(1.0f / eps);
        float k_re = std::sinh(A);
        float k_im = std::cosh(A);
        float t = std::tan(w / 2);

        {
            // (s+p)/(s-p) -> (z^-1-z_pole)/(1-z_pole*z^-1)
            float s_pole = k_re;
            float z_pole = (1 + s_pole * t) / (1 - s_pole * t);
            allpass1_.SetA(-z_pole);
        }

        size_t down_idx = num_up2_;
        size_t up_idx = 0;
        for (size_t i = 1; i < (order + 1) / 2; ++i) {
            float const qw = i * std::numbers::pi_v<float> / order_;
            std::complex<float> s_pole{std::cos(qw) * k_re, std::sin(qw) * k_im};
            auto z_pole = (1.0f + s_pole * t) / (1.0f - s_pole * t);

            float a1 = -2 * z_pole.real();
            float a2 = std::norm(z_pole);
            if (i % 2 == 1) {
                // to down
                allpass_[down_idx].SetA1(a1);
                allpass_[down_idx++].SetA2(a2);
            }
            else {
                // to up
                allpass_[up_idx].SetA1(a1);
                allpass_[up_idx++].SetA2(a2);
            }
        }
    }

    /**
     * @note up + down = LP, up - down = HP
     * @return {up, down}
     */
    std::pair<float, float> Tick(float x) noexcept {
        // up
        float up = x;
        up = allpass1_.Tick(up);
        for (size_t i = 0; i < num_up2_; ++i) {
            up = allpass_[i].Tick(up);
        }
        // down
        float down = x;
        for (size_t i = 0; i < num_down2_; ++i) {
            down = allpass_[i + num_up2_].Tick(down);
        }
        return {up, down};
    }

        /**
     * @note up + down = LP, up - down = HP
     * @return {up, down}
     */
    std::pair<float, float> Tick(float up, float down) noexcept {
        // up
        up = allpass1_.Tick(up);
        for (size_t i = 0; i < num_up2_; ++i) {
            up = allpass_[i].Tick(up);
        }
        // down
        for (size_t i = 0; i < num_down2_; ++i) {
            down = allpass_[i + num_up2_].Tick(down);
        }
        return {up, down};
    }

    /**
     * @return {up, down}
     */
    std::pair<std::complex<float>, std::complex<float>> GetResponce(std::complex<float> z) noexcept {
        // up
        std::complex<float> up = allpass1_.GetResponce(z);
        for (size_t i = 0; i < num_up2_; ++i) {
            up *= allpass_[i].GetResponce(z);
        }
        // down
        std::complex<float> down = 1.0f;
        for (size_t i = 0; i < num_down2_; ++i) {
            down *= allpass_[i + num_up2_].GetResponce(z);
        }
        return {up, down};
    }
private:
    float cutoff_w_{};
    size_t order_{};
    size_t num_up2_{};
    size_t num_down2_{};
    std::vector<qwqdsp::filter::AllpassOrder2> allpass_;
    qwqdsp::filter::AllpassOrder1 allpass1_;
};
}