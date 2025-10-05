#pragma once
#include <cassert>
#include <vector>
#include <numbers>
#include <complex>
#include <ipp/ipps.h>
#include "curve_v2.h"

static inline struct {
    void operator=(IppStatus s) noexcept {
        if (s != ippStsNoErr) {
            __debugbreak();
        }
    }
} check;

class SDelay {
public:
    static constexpr size_t kMaxCascade = 4096;

    SDelay() {
        magic_beta_ = std::sqrt(beta_ / (1 - beta_));
        center_.resize(kMaxCascade + 8);
        bw_.resize(kMaxCascade + 8);
        radius_.resize(kMaxCascade + 8);

        biquad_taps_ = ippsMalloc_32f(6 * kMaxCascade);
        output_buffer_ = ippsMalloc_32f(1024);
        int state_size = 0;
        check = ippsIIRGetStateSize_BiQuad_32f(kMaxCascade, &state_size);
        ptr_buffer_ = ippsMalloc_8u(state_size);
    }

    ~SDelay() {
        // 这里居然没有让juce爆内存泄露？
        // qwqfixme: 增加释放内存
    }

    void PrepareProcess(float sample_rate) {
        sample_rate_ = sample_rate;
    }

    void Process(float* first, float* second, size_t num_samples) {
        check = ippsIIR_32f(
            first,
            output_buffer_,
            num_samples,
            iir_state_ptr_
        );
        std::copy_n(output_buffer_, num_samples, first);
        std::copy_n(output_buffer_, num_samples, second);
    }

    void PaincFilterFb() {
        
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
        
        num_cascade_filters_ = 0;
        float intergal = 0.0f;
        float allpass_begin_w = begin_w;
        float allpass_end_w = begin_w;

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

                    float const a1 = -2 * pole_radius * std::cos(center);
                    float const a2 = pole_radius * pole_radius;
                    float* biquad_coeff = biquad_taps_ + num_cascade_filters_ * 6;
                    biquad_coeff[0] = a2; // b0
                    biquad_coeff[1] = a1; // b1
                    biquad_coeff[2] = 1;  // b2
                    biquad_coeff[3] = 1;  // a0
                    biquad_coeff[4] = a1;// a1
                    biquad_coeff[5] = a2;// a2

                    ++num_cascade_filters_;
                }
            }
        }

        check = ippsIIRInit_BiQuad_32f(
            &iir_state_ptr_,
            biquad_taps_,
            num_cascade_filters_,
            nullptr,
            ptr_buffer_
        );
    }

    void SetMinBw(float bw) {
        constexpr auto twopi = std::numbers::pi_v<float> * 2;
        min_bw_ = bw / sample_rate_ * twopi;
    }

    void SetBeta(float beta) {
        beta_ = beta;
        magic_beta_ = std::sqrt(beta_ / (1 - beta_));
        for (size_t i = 0; i < num_cascade_filters_; ++i) {
            auto pole_radius = GetPoleRadius(bw_[i]);
            float const a1 = -2 * pole_radius * std::cos(center_[i]);
            float const a2 = pole_radius * pole_radius;
            float* biquad_coeff = biquad_taps_ + num_cascade_filters_ * 6;
            biquad_coeff[0] = a2; // b0
            biquad_coeff[1] = a1; // b1
            biquad_coeff[2] = 1;  // b2
            biquad_coeff[3] = 1;  // a0
            biquad_coeff[4] = a1;// a1
            biquad_coeff[5] = a2;// a2
        }

        check = ippsIIRInit_BiQuad_32f(
            &iir_state_ptr_,
            biquad_taps_,
            num_cascade_filters_,
            nullptr,
            ptr_buffer_
        );
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
    Ipp8u* ptr_buffer_{};
    IppsIIRState_32f* iir_state_ptr_{};
    Ipp32f* output_buffer_{};
    Ipp32f* biquad_taps_{};

    std::vector<float> center_;
    std::vector<float> bw_;
    std::vector<float> radius_;

    float sample_rate_{48000.0f};
    float beta_{0.5f}; // 最大群延迟的分数延迟
    float magic_beta_{};
    float min_bw_{};
};