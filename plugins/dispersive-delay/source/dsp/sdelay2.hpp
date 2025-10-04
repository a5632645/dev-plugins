#pragma once
#include <vector>
#include <numbers>
#include <complex>
#include "convert.hpp"
#include "curve_v2.h"
#include "qwqdsp/psimd/align_allocator.hpp"

class SDelay {
public:
    static constexpr size_t kMaxCascade = 4096;

    SDelay() {
        magic_beta_ = std::sqrt(beta_ / (1 - beta_));
        x0_.resize(kMaxCascade);
        y0_.resize(kMaxCascade);
        lag1_.resize(kMaxCascade);
        lag2_.resize(kMaxCascade);
        a1_.resize(kMaxCascade);
        a2_.resize(kMaxCascade);
        center_.resize(kMaxCascade);
        bw_.resize(kMaxCascade);
        radius_.resize(kMaxCascade);
    }

    void PrepareProcess(float sample_rate) {
        sample_rate_ = sample_rate;
    }

    void Process(float* input, size_t num_samples) {
        size_t const cascade_loop_count = (num_cascade_filters_ + 7) / 8;
        for (size_t xidx = 0; xidx < num_samples; ++xidx) {
            {
                float* x_ptr = x0_.data();
                float* y_ptr = y0_.data();
                float* a2_ptr = a2_.data();
                float* lag1_ptr = lag1_.data();
                float x = input[xidx];
                for (size_t fidx = 0; fidx < num_cascade_filters_; ++fidx) {
                    *x_ptr = x;
                    x = x * *a2_ptr + *lag1_ptr;
                    *y_ptr = x;

                    ++x_ptr;
                    ++y_ptr;
                    ++a2_ptr;
                    ++lag1_ptr;
                }
            }

            float* x_ptr = x0_.data();
            float* y_ptr = y0_.data();
            float* lag1_ptr = lag1_.data();
            float* lag2_ptr = lag2_.data();
            float* a1_ptr = a1_.data();
            float* a2_ptr = a2_.data();
            for (size_t i = 0; i < cascade_loop_count; ++i) {
                auto x = _mm256_load_ps(x_ptr);
                auto a1 = _mm256_load_ps(a1_ptr);
                auto lag2 = _mm256_load_ps(lag2_ptr);
                auto y = _mm256_load_ps(y_ptr);
                
                // update lag1
                auto lag1 = _mm256_add_ps(lag2, _mm256_sub_ps(_mm256_mul_ps(x, a1), _mm256_mul_ps(y, a1)));
                _mm256_store_ps(lag1_ptr, lag1);

                // update lag2
                auto a2 = _mm256_load_ps(a2_ptr);
                lag2 = _mm256_sub_ps(x, _mm256_mul_ps(y, a2));
                _mm256_store_ps(lag2_ptr, lag2);

                x_ptr += 8;
                y_ptr += 8;
                lag1_ptr += 8;
                lag2_ptr += 8;
                a1_ptr += 8;
                a2_ptr += 8;
            }

            input[xidx] = y0_[num_cascade_filters_ - 1];
        }
    }

    void PaincFilterFb() {
        std::fill(lag1_.begin(), lag1_.end(), 0);
        std::fill(lag2_.begin(), lag2_.end(), 0);
    }

    /**
     * @brief 
     * @param curve unit: ms
     * @param f_begin 0~1
     * @param f_end 0~1
     */
    void SetCurvePitchAxis(mana::CurveV2& curve, size_t resulotion, float max_delay_ms, float p_begin, float p_end) {
        constexpr auto twopi = std::numbers::pi_v<float> * 2;
        float intergal = 0.0f;
        auto freq_begin_hz = SemitoneMap(p_begin);
        auto freq_end_hz = freq_begin_hz;
        const auto real_freq_end_hz = SemitoneMap(p_end);
        auto freq_interval_hz = (real_freq_end_hz - freq_begin_hz) / resulotion;
        auto nor_freq_interval = freq_interval_hz / sample_rate_ * twopi;
        auto nor_freq_begin = freq_begin_hz / sample_rate_ * twopi;

        size_t const old_num = num_cascade_filters_;
        num_cascade_filters_ = 0;
        for (size_t i = 0; i < resulotion;) {
            while (intergal < twopi && i < resulotion) {
                auto st = Hz2Semitone(freq_end_hz);
                auto nor = (st - s_st_begin) / (s_st_end - s_st_begin);
                nor = std::clamp(nor, 0.0f, 1.0f);
                auto delay_ms = curve.GetNormalize(nor) * max_delay_ms;
                auto delay_samples = delay_ms * sample_rate_ / 1000.0f;
                intergal += nor_freq_interval * delay_samples;
                freq_end_hz += freq_interval_hz;
                ++i;
            }

            while (intergal > twopi) {
                intergal -= twopi;
                // 正确的，您应该循环一次就创建一个滤波器
                // 然而那样太卡了
            }

            if (i >= resulotion && intergal < twopi) {
                freq_end_hz = real_freq_end_hz;
            }
            auto freq_end = freq_end_hz / sample_rate_ * twopi;
            auto freq_begin = freq_begin_hz / sample_rate_ * twopi;

            // 创建全通滤波器?
            auto center = freq_begin + (freq_end - freq_begin) / 2.0f;
            auto bw = freq_end - freq_begin;

            if (bw > min_bw_) {
                freq_begin_hz = freq_end_hz;
                if (num_cascade_filters_ < kMaxCascade) {
                    auto pole_radius = GetPoleRadius(bw);
                    center_[num_cascade_filters_] = center;
                    radius_[num_cascade_filters_] = pole_radius;
                    bw_[num_cascade_filters_] = bw;
                    a1_[num_cascade_filters_] = -2 * pole_radius * std::cos(center);
                    a2_[num_cascade_filters_] = pole_radius * pole_radius;
                    ++num_cascade_filters_;
                }
            }
        }

        for (size_t i = old_num; i < num_cascade_filters_; ++i) {
            lag1_[i] = 0;
            lag2_[i] = 0;
        }
    }

    /**
     * @brief 
     * @param curve 
     * @param resulotion 
     * @param max_delay_ms 
     * @param f_begin 0~pi
     * @param f_end 0~pi
     */
    void SetCurve(mana::CurveV2& curve, size_t resulotion, float max_delay_ms, float f_begin, float f_end) {
        constexpr auto twopi = std::numbers::pi_v<float> *2;
        float intergal = 0.0f;
        auto freq_begin = f_begin;
        auto freq_end = f_begin;
        auto freq_interval = (f_end - f_begin) / resulotion;

        size_t const old_num = num_cascade_filters_;
        num_cascade_filters_ = 0;
        for (size_t i = 0; i < resulotion;) {
            while (intergal < twopi && i < resulotion) {
                auto nor = i / (resulotion - 1.0f);
                auto delay_ms = curve.GetNormalize(nor) * max_delay_ms;
                auto delay_samples = delay_ms * sample_rate_ / 1000.0f;
                intergal += freq_interval * delay_samples;
                freq_end += freq_interval;
                ++i;
            }

            while (intergal > twopi) {
                intergal -= twopi;
            }

            if (i >= resulotion && intergal < twopi) {
                freq_end = f_end;
            }

            // 创建一个全通滤波器
            auto center = freq_begin + (freq_end - freq_begin) / 2.0f;
            auto bw = freq_end - freq_begin;
            if (bw > min_bw_) {
                freq_begin = freq_end;
                if (num_cascade_filters_ < kMaxCascade) {
                    auto pole_radius = GetPoleRadius(bw);
                    radius_[num_cascade_filters_] = pole_radius;
                    center_[num_cascade_filters_] = center;
                    bw_[num_cascade_filters_] = bw;
                    a1_[num_cascade_filters_] = -2 * pole_radius * std::cos(center);
                    a2_[num_cascade_filters_] = pole_radius * pole_radius;
                    ++num_cascade_filters_;
                }
            }
        }

        for (size_t i = old_num; i < num_cascade_filters_; ++i) {
            lag1_[i] = 0;
            lag2_[i] = 0;
        }
    }

    void SetMinBw(float bw) {
        constexpr auto twopi = std::numbers::pi_v<float> * 2;
        min_bw_ = bw / sample_rate_ * twopi;
    }

    void SetBeta(float beta) {
        beta_ = beta;
        magic_beta_ = std::sqrt(beta_ / (1 - beta_));

        for (size_t i = 0; i < num_cascade_filters_; ++i) {
            auto radius = GetPoleRadius(bw_[i]);
            radius_[i] = radius;
            a1_[i] = -2 * radius * std::cos(center_[i]);
            a2_[i] = radius * radius;
        }
    }

    float GetGroupDelay(float w) const {
        auto const z = std::polar(1.0f, w);
        std::complex<float> delay{};
        for (size_t i = 0; i < num_cascade_filters_; ++i) {
            auto pole = std::polar(radius_[i], center_[i]);
            delay += (pole * z) / (pole * z - 1.0f);
            delay -= z / (z - pole);
            pole = std::conj(pole);
            delay += (pole * z) / (pole * z - 1.0f);
            delay -= z / (z - pole);
        }
        return std::abs(delay.real());
    }

    size_t GetNumFilters() const {
        return num_cascade_filters_;
    }

    void CopyFrom(const SDelay& other) {
        size_t const old_num = num_cascade_filters_;
        num_cascade_filters_ = other.num_cascade_filters_;
        std::copy_n(other.a1_.begin(), num_cascade_filters_, a1_.begin());
        std::copy_n(other.a2_.begin(), num_cascade_filters_, a2_.begin());
        for (size_t i = old_num; i < num_cascade_filters_; ++i) {
            lag1_[i] = 0;
            lag2_[i] = 0;
        }
    }
private:
    float GetPoleRadius(float bw) const {
        float ret{};
        if (bw < 0.01f) {
            ret = (1.0f - magic_beta_ * (0.5f * (bw)));
        }
        else {
            auto n = (1.0f - beta_ * std::cos(-bw * 0.5f)) / (1.0f - beta_);
            ret =  (n - std::sqrt(n * n - 1));
        }
        return std::min(std::max(0.0f, ret), 0.999995f);
    }

    size_t num_cascade_filters_{};
    using SimdAllocator = qwqdsp::psimd::AlignedAllocator<float, 32>;
    std::vector<float, SimdAllocator> x0_;
    std::vector<float, SimdAllocator> y0_;
    std::vector<float, SimdAllocator> lag1_;
    std::vector<float, SimdAllocator> lag2_;
    std::vector<float, SimdAllocator> a2_;
    std::vector<float, SimdAllocator> a1_;

    std::vector<float> center_;
    std::vector<float> bw_;
    std::vector<float> radius_;

    float sample_rate_{48000.0f};
    float beta_{0.5f}; // 最大群延迟的分数延迟
    float magic_beta_{};
    float min_bw_{};
};