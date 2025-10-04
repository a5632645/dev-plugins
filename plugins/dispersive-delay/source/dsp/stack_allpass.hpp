#pragma once
#include <cmath>
#include <ranges>
#include "qwqdsp/psimd/vec8.hpp"

/*
* 堆叠N个全通的级联滤波器
*/
class StackAllPassFilter {
public:
    static constexpr auto kNumStack = 8;

    StackAllPassFilter() = default;
    StackAllPassFilter(float theta[kNumStack], float radius[kNumStack], float bw[kNumStack]) {
        Set(theta, radius, bw);
    }

    void Process(float* input, int num_samples) {
        auto x2 = x2_;
        auto x1 = x1_;
        auto y2 = y2_;
        auto y1 = y1_;
        auto ca = a_;
        auto cb = b_;
        qwqdsp::psimd::Vec8f32 x_tmp{};
        qwqdsp::psimd::Vec8f32 y_tmp{};

        for (int n = 0; n < num_samples; ++n) {
            auto tmp = x2 + x1 * ca - y1 * ca - y2 * cb;
            float t2 = input[n];
            for (int i = 0; i < kNumStack; ++i) {
                x_tmp.x[i] = t2;
                auto filter_i_output = tmp.x[i] + t2 * b_.x[i];
                y_tmp.x[i] = filter_i_output;
                t2 = filter_i_output;
            }
            // output
            input[n] = t2;

            // write and swap register
            y2 = y1;
            y1 = y_tmp;
            x2 = x1;
            x1 = x_tmp;
        }

        // store
        y2_ = y2;
        y1_ = y1;
        x2_ = x2;
        x1_ = x1;
    }

    void Set(float theta[kNumStack], float radius[kNumStack], float bw[kNumStack]) {
        // calc coeff
        for (int i = 0; i < kNumStack; ++i) {
            b_.x[i] = radius[i] * radius[i];
            a_.x[i] = -2 * radius[i] * std::cos(theta[i]);
        }

        // copy additional info
        std::ranges::copy(theta, theta + kNumStack, theta_);
        std::ranges::copy(radius, radius + kNumStack, radius_);
        std::ranges::copy(bw, bw + kNumStack, bw_);
    }

    float GetBw(size_t i) const {
        return bw_[i];
    }

    float GetTheta(size_t i) const {
        return theta_[i];
    }

    float GetGroupDelay(float w) const {
        constexpr auto interval = 1.0f / 10000.0f;
        return -(GetPhaseResponse(w + interval) - GetPhaseResponse(w)) / interval;
    }

    float GetPhaseResponse(float w) const {
        float ret = 0.0f;
        for (int i = 0; i < kNumStack; ++i) {
            ret += GetSinglePhaseResponse(w, theta_[i], radius_[i]);
        }
        return ret;
    }

    void PaincFb() {
        std::fill(x2_.x, x2_.x + kNumStack, 0.0f);
        std::fill(x1_.x, x1_.x + kNumStack, 0.0f);
        std::fill(y2_.x, y2_.x + kNumStack, 0.0f);
        std::fill(y1_.x, y1_.x + kNumStack, 0.0f);
    }
private:
    inline static float GetSinglePhaseResponse(float w, float theta, float radius) {
        return -2 * w
            - 2 * std::atan(radius * std::sin(w - theta) / (1 - radius * std::cos(w - theta)))
            - std::atan(radius * std::sin(w + theta) / (1 - radius * std::cos(w + theta)));
    }

    float theta_[kNumStack]{};
    float radius_[kNumStack]{};
    float bw_[kNumStack]{};

    qwqdsp::psimd::Vec8f32 a_{};
    qwqdsp::psimd::Vec8f32 b_{};
    qwqdsp::psimd::Vec8f32 x1_{};
    qwqdsp::psimd::Vec8f32 x2_{};
    qwqdsp::psimd::Vec8f32 y1_{};
    qwqdsp::psimd::Vec8f32 y2_{};
};
