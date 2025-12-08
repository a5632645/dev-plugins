#pragma once
#include <algorithm>
#include "blep_coeff.hpp"

namespace qwqdsp_oscillator {
template<qwqdsp_oscillator::blep_coeff::CBlepCoeff TCoeff>
class PolyBlep {
public:
    static constexpr float kRangeMultiply = TCoeff::kHalfLen;

    void SetFreq(float f, float fs) noexcept {
        phase_inc_ = f / fs;
    }

    void SetPWM(float width) noexcept {
        pwm_ = width;
    }

    float Sawtooth() noexcept {
        phase_ += phase_inc_;
        phase_ -= std::floor(phase_);
        return Sawtooth(phase_, phase_inc_);
    }

    static float Sawtooth(float phase, float phase_inc) noexcept {
        phase += phase_inc;
        phase -= std::floor(phase);
        // -------------------- high quality --------------------
        float const t = phase * 2 - 1;
        float const blep = 2 * Blep(phase, phase_inc);
        return t - blep;
        // -------------------- faster --------------------
        // float dt = std::min(phase_inc_, 0.5f / TCoeff::kHalfLen);
        // float naive = Frac(phase_ + 0.5f);
        // float blep = BlepOffset(phase_, dt, 0.5f);
        // return 2 * (naive - blep) - 1;
    }

    float Sqaure() noexcept {
        phase_ += phase_inc_;
        phase_ -= std::floor(phase_);
        return Sqaure(phase_, phase_inc_);
    }

    static float Sqaure(float phase, float phase_inc) noexcept {
        phase += phase_inc;
        phase -= std::floor(phase);
        float const t = phase < static_cast<float>(0.5) ? 1 : -1;
        // -------------------- high quality --------------------
        // float dt = phase_inc_;
        // float const blep = Blep(phase_, phase_inc_)
        //     - BlepOffset(phase_, dt, 0.5f) 
        //     - BlepOffset(phase_, dt, 1.5f) 
        //     - BlepOffset(phase_, dt, -0.5f);
        // -------------------- fast --------------------
        float dt = std::min(phase_inc, 0.5f / TCoeff::kHalfLen);
        float const blep = Blep(phase, phase_inc) - BlepOffset(phase, dt, 0.5f);
        return t + 2 * blep;
    }

    float PWM_NoDC() noexcept {
        phase_ += phase_inc_;
        phase_ -= std::floor(phase_);
        return PWM_NoDC(phase_, phase_inc_, pwm_);
    }

    static float PWM_NoDC(float phase, float phase_inc, float pwm) noexcept {
        phase += phase_inc;
        phase -= std::floor(phase);
        float const t = phase * 2 - 1;
        float const saw1 = t - 2 * Blep(phase, phase_inc);
        float const phase2 = phase + pwm;
        float const phase2_wrap = phase2 - std::floor(phase2);
        float const t2 = phase2_wrap * 2 - 1;
        float const saw2 = t2 - 2 * Blep(phase2_wrap, phase_inc);
        return saw1 - saw2;
    }

    float PWM_Classic() noexcept {
        phase_ += phase_inc_;
        phase_ -= std::floor(phase_);
        return PWM_Classic(phase_, phase_inc_, pwm_);
    }

    /**
     * @note 如果是faster代码,该波形需要限制pwm在[phase_inc*blep_len,1-phase_inc*blep_len]之间
     */
    static float PWM_Classic(float phase, float phase_inc, float pwm) noexcept {
        phase += phase_inc;
        phase -= std::floor(phase);
        // -------------------- faster --------------------
        // float const t = phase_ < static_cast<float>(pwm_) ? 1 - pwm : -pwm;
        // float const blep = Blep(phase_, phase_inc_) - BlepOffset(phase_, phase_inc_, pwm_);
        // -------------------- faster auto clamp --------------------
        float dt = std::min(phase_inc, 0.5f / TCoeff::kHalfLen);
        pwm = std::clamp(pwm, TCoeff::kHalfLen * dt, 1 - TCoeff::kHalfLen * dt);
        float const t = phase < static_cast<float>(pwm) ? 1 - pwm : -pwm;
        float const blep = Blep(phase, phase_inc) - BlepOffset(phase, dt, pwm);
        // -------------------- high quality --------------------
        // float const t = phase_ < static_cast<float>(pwm_) ? 1 - pwm : pwm;
        // float const blep = Blep(phase_, phase_inc_) 
        //     - BlepOffset(phase_, phase_inc_, pwm_) 
        //     - BlepOffset(phase_, phase_inc_, pwm_ + 1) 
        //     - BlepOffset(phase_, phase_inc_, pwm_ - 1);
        auto v = t + blep;
        return v * 2;
    }

    float Triangle() noexcept {
        phase_ += phase_inc_;
        phase_ -= std::floor(phase_);
        return Triangle(phase_, phase_inc_);
    }

    static float Triangle(float phase, float phase_inc) noexcept {
        phase += phase_inc;
        phase -= std::floor(phase);
        float t = 0;
        if (phase < static_cast<float>(0.5)) {
            t = 1 - 4 * phase;
        }
        else {
            t = 4 * phase - 3;
        }
        float const phase2 = phase + static_cast<float>(0.5);
        float const phase2_wrap = phase2 - std::floor(phase2);
        return t + 8 * phase_inc * (-Blamp(phase, phase_inc) + Blamp(phase2_wrap, phase_inc));
    }
private:
    /**
     * @param t [0..1]
     * @param dt [0..1]
     */
    static constexpr float Blep(float t, float dt) noexcept {
        auto t1 = std::min(t / dt, TCoeff::kHalfLen);
        auto t2 = std::min((1.0f - t) / dt, TCoeff::kHalfLen);
        return TCoeff::GetBlepHalf(t1) - TCoeff::GetBlepHalf(t2);
    }

    /**
     * @brief 以offset为0的blep
     */
    static float BlepOffset(float t, float dt, float offset) noexcept {
        float x = (t - offset) / dt;
        x = std::clamp(x, -TCoeff::kHalfLen, TCoeff::kHalfLen);
        return -std::copysign(TCoeff::GetBlepHalf(std::abs(x)), x);
    }

    static constexpr float Blamp(float t, float dt) noexcept {
        auto t1 = std::min(t / dt, TCoeff::kHalfLen);
        auto t2 = std::min((1.0f - t) / dt, TCoeff::kHalfLen);
        return TCoeff::GetBlampHalf(t1) + TCoeff::GetBlampHalf(t2);
    }

    float phase_{};
    float phase_inc_{0.00001f};
    float pwm_{};
};
}
