#pragma once
#include <vector>
#include <numbers>
#include "stack_allpass.hpp"
#include "convert.hpp"
#include "curve_v2.h"

class SDelay {
public:
    using Filter = StackAllPassFilter;

    SDelay() {
        filters_.reserve(512);
        magic_beta_ = std::sqrt(beta_ / (1 - beta_));
    }

    void PrepareProcess(float sample_rate) {
        sample_rate_ = sample_rate;
    }

    void Process(float* input, int num_samples) {
        for (size_t i = 0; i < add_filter_counter_; ++i) {
            filters_[i].Process(input, num_samples);
        }
    }

    void PaincFilterFb() {
        for (auto& f : filters_) {
            f.PaincFb();
        }
    }

    /**
     * @brief 
     * @param curve unit: ms
     * @param f_begin 0~1
     * @param f_end 0~1
     */
    void SetCurvePitchAxis(mana::CurveV2& curve, int resulotion, float max_delay_ms, float p_begin, float p_end) {
        constexpr auto twopi = std::numbers::pi_v<float> * 2;
        float intergal = 0.0f;
        auto freq_begin_hz = SemitoneMap(p_begin);
        auto freq_end_hz = freq_begin_hz;
        const auto real_freq_end_hz = SemitoneMap(p_end);
        auto freq_interval_hz = (real_freq_end_hz - freq_begin_hz) / resulotion;
        auto nor_freq_interval = freq_interval_hz / sample_rate_ * twopi;
        auto nor_freq_begin = freq_begin_hz / sample_rate_ * twopi;

        ClearFilters();
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
                auto pole_radius = GetPoleRadius(bw);
                AddFilter(center, pole_radius, bw);
                freq_begin_hz = freq_end_hz;
            }
        }
        EndAddFilter();
    }

    /**
     * @brief 
     * @param curve 
     * @param resulotion 
     * @param max_delay_ms 
     * @param f_begin 0~pi
     * @param f_end 0~pi
     */
    void SetCurve(mana::CurveV2& curve, int resulotion, float max_delay_ms, float f_begin, float f_end) {
        constexpr auto twopi = std::numbers::pi_v<float> *2;
        float intergal = 0.0f;
        auto freq_begin = f_begin;
        auto freq_end = f_begin;
        auto freq_interval = (f_end - f_begin) / resulotion;

        ClearFilters();
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
                auto pole_radius = GetPoleRadius(bw);
                AddFilter(center, pole_radius, bw);
                freq_begin = freq_end;
            }
        }
        EndAddFilter();
    }

    void SetMinBw(float bw) {
        constexpr auto twopi = std::numbers::pi_v<float> * 2;
        min_bw_ = bw / sample_rate_ * twopi;
    }

    void SetBeta(float beta) {
        beta_ = beta;
        magic_beta_ = std::sqrt(beta_ / (1 - beta_));

        for (size_t i = 0; i < add_filter_counter_; ++i) {
            auto& f = filters_[i];
            float center[Filter::kNumStack]{
                f.GetTheta(0), f.GetTheta(1), f.GetTheta(2), f.GetTheta(3), f.GetTheta(4), f.GetTheta(5), f.GetTheta(6), f.GetTheta(7)
            };
            float bw[Filter::kNumStack]{
                f.GetBw(0), f.GetBw(1), f.GetBw(2), f.GetBw(3), f.GetBw(4), f.GetBw(5), f.GetBw(6), f.GetBw(7)
            };
            float radius[Filter::kNumStack]{
                GetPoleRadius(bw[0]), GetPoleRadius(bw[1]), GetPoleRadius(bw[2]), GetPoleRadius(bw[3]), GetPoleRadius(bw[4]), GetPoleRadius(bw[5]), GetPoleRadius(bw[6]), GetPoleRadius(bw[7])
            };
            f.Set(center, radius, bw);
        }
    }

    float GetGroupDelay(float w) const {
        float delay = 0.0f;
        for (size_t i = 0; i < add_filter_counter_; ++i) {
            delay += filters_[i].GetGroupDelay(w);
        }
        return delay;
    }

    size_t GetNumFilters() const {
        return add_filter_counter_ * Filter::kNumStack;
    }
private:
    inline float GetPoleRadius(float bw) const {
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

    inline void AddFilter(float center, float radius, float bw) {
        if (stack_filter_counter_ == Filter::kNumStack) {
            if (add_filter_counter_ < filters_.size()) {
                filters_[add_filter_counter_].Set(center_, radius_, bw_);
                ++add_filter_counter_;
            }
            else {
                filters_.emplace_back(center_, radius_, bw_);
                ++add_filter_counter_;
            }
            stack_filter_counter_ = 0;
        }
        else {
            center_[stack_filter_counter_] = center;
            radius_[stack_filter_counter_] = radius;
            bw_[stack_filter_counter_] = bw;
            ++stack_filter_counter_;
        }
    }

    inline void EndAddFilter() {
        constexpr auto pi = std::numbers::pi_v<float>;
        if (stack_filter_counter_ > 0) {
            // 复制最后一个滤波器
            auto center = center_[stack_filter_counter_ - 1];
            auto radius = radius_[stack_filter_counter_ - 1];
            auto bw = bw_[stack_filter_counter_ - 1];
            for (; stack_filter_counter_ < Filter::kNumStack; ++stack_filter_counter_) {
                center_[stack_filter_counter_] = center;
                radius_[stack_filter_counter_] = radius;
                bw_[stack_filter_counter_] = bw;
            }
            AddFilter(center, radius, bw);
        }
    }

    inline void ClearFilters() {
        stack_filter_counter_ = 0;
        add_filter_counter_ = 0;
    }

    std::vector<Filter> filters_;
    size_t stack_filter_counter_{};
    size_t add_filter_counter_{};
    float center_[Filter::kNumStack]{};
    float radius_[Filter::kNumStack]{};
    float bw_[Filter::kNumStack]{};

    float sample_rate_{48000.0f};
    float beta_{ 0.5f }; // 最大群延迟的分数延迟
    float magic_beta_{};
    float min_bw_{};
};