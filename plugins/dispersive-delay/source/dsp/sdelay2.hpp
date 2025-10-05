#pragma once
#include <vector>
#include <numbers>
#include <complex>
#include "curve_v2.h"
#include "qwqdsp/psimd/align_allocator.hpp"

class SDelay {
public:
    static constexpr size_t kMaxCascade = 4096;

    SDelay() {
        magic_beta_ = std::sqrt(beta_ / (1 - beta_));
        filters_.resize(kMaxCascade);
        center_.resize(kMaxCascade + 8);
        bw_.resize(kMaxCascade + 8);
        radius_.resize(kMaxCascade + 8);
    }

    void PrepareProcess(float sample_rate) {
        sample_rate_ = sample_rate;
    }

    void Process(float* first, float* second, size_t num_samples) {
        size_t const cascade_loop_count = (num_cascade_filters_ + 7) / 8;
        size_t const scalar_loop_count = num_cascade_filters_ & 8;
        alignas(32) float vy_first[8]{};
        alignas(32) float vy_second[8]{};
#ifndef __AVX2__
        alignas(32) float vx[9]{};
#endif
        for (size_t xidx = 0; xidx < num_samples; ++xidx) {
            // x永远是最后一个滤波器的输出
            float x_first = first[xidx];
            float x_second = second[xidx];
            auto* filter_ptr = filters_.data();
            for (size_t i = 0; i < cascade_loop_count; ++i) {
#ifdef __AVX2__
                float const begin_x_input_first = x_first;
                float const begin_x_input_second = x_second;
#else
                vx[0] = x;
#endif
                // left channel serial
                float* a2_ptr = filter_ptr->a2;
                float* lag1_ptr = filter_ptr->lag1;
                for (size_t j = 0; j < 8; ++j) {
                    x_first = x_first * *a2_ptr + *lag1_ptr;
                    vy_first[j] = x_first;
                    ++a2_ptr;
                    ++lag1_ptr;
                }
                // right channel serial
                a2_ptr = filter_ptr->a2;
                float* lag1_ptr_2 = filter_ptr->lag1_second;
                for (size_t j = 0; j < 8; ++j) {
                    x_second = x_second * *a2_ptr + *lag1_ptr_2;
                    vy_second[j] = x_second;
                    ++a2_ptr;
                    ++lag1_ptr_2;
                }
                a2_ptr = filter_ptr->a2;

                // vy现在是每个滤波器的输出
                // left channel
                auto shuffle_mask = _mm256_set_epi32(6, 5, 4, 3, 2, 1, 0, 7);
                auto y = _mm256_load_ps(vy_first);
#ifndef __AVX2__
                _mm256_storeu_ps(vx + 1, y);
                auto x = _mm256_load_ps(vx);
#else
                auto shuffled_reg = _mm256_permutevar8x32_ps(y, shuffle_mask);
                auto iwantx = _mm256_set1_ps(begin_x_input_first);
                auto x = _mm256_blend_ps(shuffled_reg, iwantx, 0b00000001);
#endif
                // update lag1
                auto a1 = _mm256_load_ps(filter_ptr->a1);
                auto lag2 = _mm256_load_ps(filter_ptr->lag2);
                auto lag1 = _mm256_add_ps(lag2, _mm256_sub_ps(_mm256_mul_ps(x, a1), _mm256_mul_ps(y, a1)));
                _mm256_store_ps(filter_ptr->lag1, lag1);

                // update lag2
                auto a2 = _mm256_load_ps(a2_ptr);
                lag2 = _mm256_sub_ps(x, _mm256_mul_ps(y, a2));
                _mm256_store_ps(filter_ptr->lag2, lag2);


                // right channel
                y = _mm256_load_ps(vy_second);
#ifndef __AVX2__
                _mm256_storeu_ps(vx + 1, y);
                auto x = _mm256_load_ps(vx);
#else
                shuffled_reg = _mm256_permutevar8x32_ps(y, shuffle_mask);
                iwantx = _mm256_set1_ps(begin_x_input_second);
                x = _mm256_blend_ps(shuffled_reg, iwantx, 0b00000001);
#endif
                // update lag1
                lag2 = _mm256_load_ps(filter_ptr->lag2_second);
                lag1 = _mm256_add_ps(lag2, _mm256_sub_ps(_mm256_mul_ps(x, a1), _mm256_mul_ps(y, a1)));
                _mm256_store_ps(filter_ptr->lag1_second, lag1);

                // update lag2
                lag2 = _mm256_sub_ps(x, _mm256_mul_ps(y, a2));
                _mm256_store_ps(filter_ptr->lag2_second, lag2);

                ++filter_ptr;
            }

            //// 处理凑不齐8个的
#ifdef __AVX2__
            float begin_x_input_first = x_first;
            float begin_x_input_second = x_second;
#else
            vx[0] = x;
#endif
            // left channel
            float* a2_ptr = filter_ptr->a2;
            float* lag1_ptr = filter_ptr->lag1;
            for (size_t j = 0; j < scalar_loop_count; ++j) {
                x_first = x_first * *a2_ptr + *lag1_ptr;
                vy_first[j] = x_first;
                ++a2_ptr;
                ++lag1_ptr;
            }
            // right channel
            a2_ptr = filter_ptr->a2;
            float* lag1_ptr_2 = filter_ptr->lag1_second;
            for (size_t j = 0; j < scalar_loop_count; ++j) {
                x_second = x_second * *a2_ptr + *lag1_ptr_2;
                vy_second[j] = x_second;
                ++a2_ptr;
                ++lag1_ptr_2;
            }
            a2_ptr = filter_ptr->a2;

            // left channel
            auto shuffle_mask = _mm256_set_epi32(6, 5, 4, 3, 2, 1, 0, 7);
            auto y = _mm256_load_ps(vy_first);
#ifndef __AVX2__
            _mm256_storeu_ps(vx + 1, y);
            auto x = _mm256_load_ps(vx);
#else
            auto shuffled_reg = _mm256_permutevar8x32_ps(y, shuffle_mask);
            auto iwantx = _mm256_set1_ps(begin_x_input_first);
            auto x = _mm256_blend_ps(shuffled_reg, iwantx, 0b00000001);
#endif
            // update lag1
            auto a1 = _mm256_load_ps(filter_ptr->a1);
            auto lag2 = _mm256_load_ps(filter_ptr->lag2);
            auto lag1 = _mm256_add_ps(lag2, _mm256_sub_ps(_mm256_mul_ps(x, a1), _mm256_mul_ps(y, a1)));
            _mm256_store_ps(filter_ptr->lag1, lag1);

            // update lag2
            auto a2 = _mm256_load_ps(a2_ptr);
            lag2 = _mm256_sub_ps(x, _mm256_mul_ps(y, a2));
            _mm256_store_ps(filter_ptr->lag2, lag2);


            // right channel
            y = _mm256_load_ps(vy_second);
#ifndef __AVX2__
            _mm256_storeu_ps(vx + 1, y);
            auto x = _mm256_load_ps(vx);
#else
            shuffled_reg = _mm256_permutevar8x32_ps(y, shuffle_mask);
            iwantx = _mm256_set1_ps(begin_x_input_second);
            x = _mm256_blend_ps(shuffled_reg, iwantx, 0b00000001);
#endif
            // update lag1
            lag2 = _mm256_load_ps(filter_ptr->lag2_second);
            lag1 = _mm256_add_ps(lag2, _mm256_sub_ps(_mm256_mul_ps(x, a1), _mm256_mul_ps(y, a1)));
            _mm256_store_ps(filter_ptr->lag1_second, lag1);

            // update lag2
            lag2 = _mm256_sub_ps(x, _mm256_mul_ps(y, a2));
            _mm256_store_ps(filter_ptr->lag2_second, lag2);

            first[xidx] = x_first;
            second[xidx] = x_second;
        }
    }

    void PaincFilterFb() {
        size_t const simd_loop = (num_cascade_filters_ + 7) / 8;
        for (size_t i = 0; i < simd_loop; ++i) {
            std::fill_n(filters_[i].lag1, 8, 0);
            std::fill_n(filters_[i].lag2, 8, 0);
            std::fill_n(filters_[i].lag1_second, 8, 0);
            std::fill_n(filters_[i].lag2_second, 8, 0);
        }
    }

    inline static float Hz2Mel(float hz) {
        return 1127.0f * std::log(1.0f + hz / 700.0f);
    }

    inline static float Mel2Hz(float mel) {
        return 700.0f * (std::exp(mel / 1127.0f) - 1.0f);
    }

    inline static const auto kMinMel = Hz2Mel(20.0f);
    inline static const auto kMaxMel = Hz2Mel(20000.0f);

    /**
     * @param curve unit: ms
     * @param p_begin 0~1
     * @param p_end 0~1
     */
    void SetCurve(
        mana::CurveV2& curve, size_t resulotion, float max_delay_ms,
        float p_begin, float p_end,
        bool log_scale
    ) {
        constexpr auto twopi = std::numbers::pi_v<float> * 2;
        float begin_w = 0;
        float end_w = 0;
        float w_width = 0;
        float width_ratio_mul = 0;

        if (log_scale) {
            float const begin_mel = kMinMel + (kMaxMel - kMinMel) * p_begin;
            float const end_mel = kMinMel + (kMaxMel - kMinMel) * p_end;
            float const first_mel = begin_mel + (end_mel - begin_mel) / resulotion;
            begin_w = Mel2Hz(begin_mel) * twopi / sample_rate_;
            end_w = Mel2Hz(end_mel) * twopi / sample_rate_;
            w_width = Mel2Hz(first_mel) * twopi / sample_rate_ - begin_w;
            width_ratio_mul = std::exp((end_mel - begin_mel) / (1127.0f * resulotion));
        }
        else {
            float const begin_f = 20.0f + (20000.0f - 20.0f) * p_begin;
            float const end_f = 20.0f + (20000.0f - 20.0f) * p_end;
            begin_w = begin_f * twopi / sample_rate_;
            end_w = end_f * twopi / sample_rate_;
            w_width = (end_w - begin_w) / resulotion;
            width_ratio_mul = 1;
        }
        
        size_t const old_num = num_cascade_filters_;
        num_cascade_filters_ = 0;
        float intergal = 0.0f;
        float allpass_begin_w = begin_w;
        float allpass_end_w = begin_w;

        size_t simd_counter = 0;
        size_t scalar_counter = 0;

        for (size_t i = 0; i < resulotion;) {
            while (intergal < twopi && i < resulotion) {
                auto nor = i / (resulotion - 1.0f);
                auto delay_ms = curve.GetNormalize(nor) * max_delay_ms;
                auto delay_samples = delay_ms * sample_rate_ / 1000.0f;
                intergal += w_width * delay_samples;
                allpass_end_w += w_width;
                w_width *= width_ratio_mul;
                ++i;
            }

            while (intergal >= twopi) {
                intergal -= twopi;
                // 正确的，您应该在此warp一次就创建一个全通滤波器
            }

            if (i >= resulotion && intergal < twopi) {
                allpass_end_w = end_w;
            }

            // 创建全通滤波器?
            auto center = allpass_begin_w + (allpass_end_w - allpass_begin_w) / 2.0f;
            auto bw = allpass_end_w - allpass_begin_w;

            if (bw > min_bw_) {
                allpass_begin_w = allpass_end_w;
                if (num_cascade_filters_ < kMaxCascade) {
                    auto pole_radius = GetPoleRadius(bw);
                    center_[num_cascade_filters_] = center;
                    radius_[num_cascade_filters_] = pole_radius;
                    bw_[num_cascade_filters_] = bw;
                    filters_[simd_counter].a1[scalar_counter] = -2 * pole_radius * std::cos(center);
                    filters_[simd_counter].a2[scalar_counter] = pole_radius * pole_radius;
                    ++num_cascade_filters_;
                    ++scalar_counter;
                    if (scalar_counter > 7) {
                        scalar_counter = 0;
                        ++simd_counter;
                    }
                }
            }
        }

        // 清除新滤波器的数据
        size_t const old_num_simd_loop = (old_num + 7) / 8;
        size_t const old_num_scalar = old_num & 7;
        size_t const new_num_simd_loop = (num_cascade_filters_ + 7) / 8;
        size_t const new_num_scalar = num_cascade_filters_ & 7;
        filters_[old_num_simd_loop].ClearLags(old_num_scalar);
        for (size_t i = old_num_simd_loop; i < new_num_simd_loop; ++i) {
            filters_[i].ClearLags();
        }
        filters_[new_num_simd_loop].ClearLags(new_num_scalar);
    }

    void SetMinBw(float bw) {
        constexpr auto twopi = std::numbers::pi_v<float> * 2;
        min_bw_ = bw / sample_rate_ * twopi;
    }

    void SetBeta(float beta) {
        beta_ = beta;
        magic_beta_ = std::sqrt(beta_ / (1 - beta_));

        size_t const simd_loop = (num_cascade_filters_ + 7) / 8;
        size_t const scalar_loop = num_cascade_filters_ & 7;
        size_t fidx = 0;
        for (size_t i = 0; i < simd_loop; ++i) {
            for (size_t j = 0; j < 8; ++j) {
                auto radius = GetPoleRadius(bw_[fidx]);
                radius_[fidx] = radius;
                filters_[i].a1[j] = -2 * radius * std::cos(center_[fidx]);
                filters_[i].a2[j] = radius * radius;
                ++fidx;
            }
        }
        for (size_t j = 0; j < scalar_loop; ++j) {
            auto radius = GetPoleRadius(bw_[fidx]);
            radius_[fidx] = radius;
            filters_[simd_loop].a1[j] = -2 * radius * std::cos(center_[fidx]);
            filters_[simd_loop].a2[j] = radius * radius;
            ++fidx;
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

    struct alignas(32) CascadeFilterContent {
        float lag1[8]{};
        float lag2[8]{};
        float lag1_second[8]{};
        float lag2_second[8]{};
        float a2[8]{};
        float a1[8]{};

        void ClearLags() noexcept {
            std::fill_n(lag1, 8, 0);
            std::fill_n(lag2, 8, 0);
            std::fill_n(lag1_second, 8, 0);
            std::fill_n(lag2_second, 8, 0);
        }
        void ClearLags(size_t offset) noexcept {
            std::fill_n(lag1 + offset, 8 - offset, 0);
            std::fill_n(lag2 + offset, 8 - offset, 0);
            std::fill_n(lag1_second + offset, 8 - offset, 0);
            std::fill_n(lag2_second + offset, 8 - offset, 0);
        }
    };
    std::vector<CascadeFilterContent> filters_;

    std::vector<float> center_;
    std::vector<float> bw_;
    std::vector<float> radius_;

    float sample_rate_{48000.0f};
    float beta_{0.5f}; // 最大群延迟的分数延迟
    float magic_beta_{};
    float min_bw_{};
};