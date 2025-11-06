#pragma once
#include <algorithm>
#include "qwqdsp/osciilor/blep_coeff.hpp"
#include "qwqdsp/polymath.hpp"

namespace qwqdsp::oscillor {
/**
 * @note
 * 一个省脑子的编写Hardsync的方法是先写出无嵌套分支的blep代码，然后假设所有跳跃都会发生
 *       屏蔽发生在sync之后的跳跃,最后再检测一次重置之后的跳跃
 */
template<qwqdsp::oscillor::blep_coeff::CBlepCoeff TCoeff>
class PolyBlepSync {
public:
    static constexpr size_t kDelay = static_cast<size_t>(TCoeff::kHalfLen);
    static constexpr size_t kDelaySize = 16;
    static constexpr size_t kDelayMask = kDelaySize - 1;

    void SetFreq(float f, float fs) noexcept {
        phase_inc_ = f / fs;
    }

    void SetPWM(float width) noexcept {
        pwm_ = width;
    }

    float GetPhase() noexcept {
        return phase_;
    }

    float Sawtooth(bool reset, float sync_samples_before) noexcept {
        phase_ += phase_inc_;
        if (phase_ > 1) {
            phase_ -= 1;
            float self_reset_samples_before = phase_ / phase_inc_;
            // 无硬同步或者硬同步发生在溢出之后
            if (!reset || self_reset_samples_before > sync_samples_before) {
                AddBlep(wpos_, self_reset_samples_before, -1);
            }
            // 有硬同步且发生在溢出之前,用于计算jump
            else {
                phase_ += 1;
            }
        }
        if (reset) {
            float sync_going = phase_inc_ * sync_samples_before;
            float jump_size = phase_ - sync_going;
            AddBlep(wpos_, sync_samples_before, -jump_size);
            phase_ = sync_going;
        }
        if (phase_ > 1) {
            phase_ -= 1;
        }
        buffer_[wpos_] += NaiveSaw(phase_);
        float out = buffer_[rpos_];
        buffer_[rpos_] = 0;
        rpos_ = (rpos_ + 1) & kDelayMask;
        wpos_ = (wpos_ + 1) & kDelayMask;
        return 2 * out - 1;
    }

    float PWM(bool reset, float sync_frac_samples_before) noexcept {
        bool high = phase_ < pwm_;
        phase_ += phase_inc_;
        if (!high && phase_ > 1) {
            phase_ -= 1;
            auto t = phase_ / phase_inc_;
            if (!reset || t > sync_frac_samples_before) {
                AddBlep(wpos_, t, 1);
                high = true;
            }
        }
        if (high && phase_ > pwm_) {
            auto t = (phase_ - pwm_) / phase_inc_;
            if (!reset || t > sync_frac_samples_before) {
                AddBlep(wpos_, t, -1);
                high = false;
            }
        }
        if (!high && phase_ > 1) {
            phase_ -= 1;
            auto t = phase_ / phase_inc_;
            if (!reset || t > sync_frac_samples_before) {
                AddBlep(wpos_, t, 1);
                high = true;
            }
        }
        if (reset) {
            phase_ = sync_frac_samples_before * phase_inc_;
            float trans = !high ? 1.0f : 0.0f;
            AddBlep(wpos_, sync_frac_samples_before, trans);

            auto tphase = phase_inc_ * sync_frac_samples_before;
            if (tphase > pwm_) {
                auto t = (tphase - pwm_) / phase_inc_;
                AddBlep(wpos_, t, -1);
            }
        }

        buffer_[wpos_] += NaivePwm(phase_);
        float out = buffer_[rpos_];
        buffer_[rpos_] = 0;
        rpos_ = (rpos_ + 1) & kDelayMask;
        wpos_ = (wpos_ + 1) & kDelayMask;
        return 2 * out - 1;
    }

    float Sine(bool reset, float sync_frac_samples_before) noexcept {
        phase_ += phase_inc_;
        phase_ -= std::floor(phase_);

        if (reset) {
            phase_ -= sync_frac_samples_before * phase_inc_;
            float jump = 0 - std::sin(phase_ * std::numbers::pi_v<float> * 2);
            float old_d = std::numbers::pi_v<float> * 2 * phase_inc_ * std::cos(std::numbers::pi_v<float> * 2 * phase_);
            float new_d = std::numbers::pi_v<float> * 2 * phase_inc_;
            AddBlep(wpos_, sync_frac_samples_before, jump);
            AddBlamp(wpos_, sync_frac_samples_before, new_d - old_d);
            phase_ = sync_frac_samples_before * phase_inc_;
        }

        buffer_[wpos_] += std::sin(phase_ * std::numbers::pi_v<float> * 2);
        float out = buffer_[rpos_];
        buffer_[rpos_] = 0;
        rpos_ = (rpos_ + 1) & kDelayMask;
        wpos_ = (wpos_ + 1) & kDelayMask;
        return out;
    }

    float Triangle(bool reset, float sync_frac_samples_before) noexcept {
        bool high = phase_ < 0.5f;
        phase_ += phase_inc_;
        if (!high && phase_ > 1) {
            phase_ -= 1;
            auto t = phase_ / phase_inc_;
            if (!reset || t > sync_frac_samples_before) {
                AddBlamp(wpos_, t, -8 * phase_inc_);
                high = true;
            }
        }
        if (high && phase_ > 0.5f) {
            auto t = (phase_ - 0.5f) / phase_inc_;
            if (!reset || t > sync_frac_samples_before) {
                AddBlamp(wpos_, t, 8 * phase_inc_);
                high = false;
            }
        }
        if (!high && phase_ > 1) {
            phase_ -= 1;
            auto t = phase_ / phase_inc_;
            if (!reset || t > sync_frac_samples_before) {
                AddBlamp(wpos_, t, -8 * phase_inc_);
                high = true;
            }
        }
        if (reset) {
            phase_ -= sync_frac_samples_before * phase_inc_;
            phase_ -= std::floor(phase_);
            auto jump = NaiveTriangle(0) - NaiveTriangle(phase_);
            AddBlep(wpos_, sync_frac_samples_before, jump);
            if (!high) {
                AddBlamp(wpos_, sync_frac_samples_before, 8 * phase_inc_);
            }

            auto tphase = phase_inc_ * sync_frac_samples_before;
            if (tphase > 0.5f) {
                auto t = (tphase - 0.5f) / phase_inc_;
                AddBlamp(wpos_, t, 8 * phase_inc_);
            }
            phase_ = tphase;
        }

        buffer_[wpos_] += NaiveTriangle(phase_);
        float out = buffer_[rpos_];
        buffer_[rpos_] = 0;
        rpos_ = (rpos_ + 1) & kDelayMask;
        wpos_ = (wpos_ + 1) & kDelayMask;
        return out;
    }
private:
    static float NaiveSaw(float phase) noexcept {
        return phase;
    }

    float NaivePwm(float phase) noexcept {
        return phase < pwm_ ? 1.0f : 0.0f;
    }

    float NaiveTriangle(float phase) noexcept {
        float naive_tri = 0;
        if (phase < static_cast<float>(0.5)) {
            naive_tri = 1 - 4 * phase;
        }
        else {
            naive_tri = 4 * phase - 3;
        }
        return naive_tri;
    }

    static constexpr float Blamp(float t, float dt) noexcept {
        auto t1 = std::min(t / dt, TCoeff::kHalfLen);
        auto t2 = std::min((1.0f - t) / dt, TCoeff::kHalfLen);
        return TCoeff::GetBlampHalf(t1) + TCoeff::GetBlampHalf(t2);
    }

    /**
     * @brief 在buffer_idx前frac_samples处(采样点)插入一个scale大小的blep
     * @param buffer_idx 缓冲区绝对索引
     * @param frac_samples_before 分数采样点
     * @param scale blep大小
     */
    void AddBlep(size_t buffer_idx, float frac_samples_before, float scale) noexcept {
        size_t begin_idx = buffer_idx - kDelay;
        float x = frac_samples_before - kDelay;
        for (size_t i = 0; i < 2 * kDelay; ++i) {
            begin_idx &= kDelayMask;
            float t = std::clamp(x, -TCoeff::kHalfLen, TCoeff::kHalfLen);
            buffer_[begin_idx] -= scale * std::copysign(TCoeff::GetBlepHalf(std::abs(t)), t);
            ++begin_idx;
            x += 1.0f;
        }
    }

    void AddBlamp(size_t buffer_idx, float frac_samples_before, float scale) noexcept {
        size_t begin_idx = buffer_idx - kDelay;
        float x = frac_samples_before - kDelay;
        for (size_t i = 0; i < 2 * kDelay; ++i) {
            begin_idx &= kDelayMask;
            float t = std::clamp(x, -TCoeff::kHalfLen, TCoeff::kHalfLen);
            buffer_[begin_idx] += scale * TCoeff::GetBlampHalf(std::abs(t));
            ++begin_idx;
            x += 1.0f;
        }
    }

    float phase_{};
    float phase_inc_{0.011f};
    float pwm_{};

    float buffer_[kDelaySize]{};
    size_t wpos_{kDelay};
    size_t rpos_{};
};
}