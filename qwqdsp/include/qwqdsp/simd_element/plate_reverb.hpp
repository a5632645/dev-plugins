#pragma once
#include <numbers>
#include <array>
#include "qwqdsp/simd_element/delay_allpass.hpp"
#include "qwqdsp/simd_element/delay_line.hpp"
#include "qwqdsp/simd_element/one_pole_tpt.hpp"
#include "qwqdsp/psimd/float32x4.hpp"
#include "qwqdsp/convert.hpp"

namespace qwqdsp_simd_element {
using SimdType = qwqdsp_psimd::Float32x4;

// template<class SimdType>
class LFO {
public:
    void Reset() noexcept {
        phase_ = SimdType{};
    }

    SimdType Tick() noexcept {
        phase_ += phase_inc_;
        phase_ = phase_.Frac();
        constexpr float pi = std::numbers::pi_v<float>;
        constexpr float twopi = pi * 2;
        return SinParabola(phase_ * twopi - pi);
    }

    void SetFreq(SimdType const& f, float fs) noexcept {
        phase_inc_ = f / fs;
    }
private:
    static inline SimdType SinParabola(SimdType const& x) noexcept {
        constexpr float pi = std::numbers::pi_v<float>;
        constexpr float B = 4 / pi;
        constexpr float C = -4 / (pi * pi);
        SimdType const y = B * x + C * x * SimdType::Abs(x);
        constexpr float P = static_cast<float>(0.225);
        return y + P * (y * SimdType::Abs(y) - y);
    }

    SimdType phase_{};
    SimdType phase_inc_{};
};

class PlateReverb {
public:
    static constexpr float kMaxPredelayMs = 300.0f;
    static constexpr float kMaxSize = 2.0f;

    void Init(float fs) {
        fs_ = fs;

        float ratio = fs / 29761.0f;
        predelay_.Init(fs, kMaxPredelayMs);

        diffuser_delays_[0] = 142.0f * ratio;
        diffuser_delays_[1] = 107.0f * ratio;
        diffuser_delays_[2] = 379.0f * ratio;
        diffuser_delays_[3] = 277.0f * ratio;
        diffuser1_.Init(static_cast<size_t>(diffuser_delays_[0]) + 1);
        diffuser2_.Init(static_cast<size_t>(diffuser_delays_[1]) + 1);
        diffuser3_.Init(static_cast<size_t>(diffuser_delays_[2]) + 1);
        diffuser4_.Init(static_cast<size_t>(diffuser_delays_[3]) + 1);

        max_mod_depth_ = 8.0f * kMaxSize * ratio;
        base_output_taps_[0] = SimdType{266, 353, 266, 353} * ratio;
        base_output_taps_[1] = SimdType{2974, 3627, 2974, 3627} * ratio;
        base_output_taps_[2] = SimdType{1913, 1228, 1913, 1228} * ratio;
        base_output_taps_[3] = SimdType{1996, 2673, 1996, 2673} * ratio;
        base_output_taps_[4] = SimdType{1990, 2111, 1990, 2111} * ratio;
        base_output_taps_[5] = SimdType{187, 335, 187, 335} * ratio;
        base_output_taps_[6] = SimdType{1066, 121, 1066, 121} * ratio;

        max_apf1_delay_ = SimdType{672, 908, 672, 908} * ratio * kMaxSize;
        max_apf2_delay_ = SimdType{1800, 2656, 1800, 2656} * ratio * kMaxSize;
        max_delay1_delay_ = SimdType{4453, 4217, 4453, 4217} * ratio * kMaxSize;
        max_delay2_delay_ = SimdType{3720, 3163, 3720, 3163} * ratio * kMaxSize;

        tank_apf1_.Init(static_cast<size_t>(908 * ratio * kMaxSize) + 1);
        tank_apf2_.Init(static_cast<size_t>(2656 * ratio * kMaxSize) + 1);
        tank_delay1_.Init(static_cast<size_t>(4453 * ratio * kMaxSize) + 1);
        tank_delay2_.Init(static_cast<size_t>(3720 * ratio * kMaxSize) + 1);

        tank_lfo_.SetFreq({1.0f, 0.95f, 1.0f, 0.95f}, fs);
    }

    void Reset() noexcept {
        input_lowpass_.Reset();
        predelay_.Reset();
        tank_out_ = SimdType{};
        tank_damping_.Reset();
        tank_lfo_.Reset();
        tank_delay1_.Reset();
        tank_delay2_.Reset();
        tank_apf1_.Reset();
        tank_apf2_.Reset();
        diffuser1_.Reset();
        diffuser2_.Reset();
        diffuser3_.Reset();
        diffuser4_.Reset();
    }

    /**
     * @param x [left, right, ?, ?]
     * @return  [left, right, ?, ?]
     */
    SimdType Tick(SimdType const& x) noexcept {
        float mid = x.x[0] + x.x[1];
        float side = x.x[0] - x.x[1];

        SimdType input{mid, side};
        input = input_lowpass_.TickLowpass(input, SimdType::FromSingle(lowpass_coeff_));

        input = diffuser1_.Tick(input, diffuser_delays_[0], 0.75f);
        input = diffuser2_.Tick(input, diffuser_delays_[1], 0.75f);
        input = diffuser3_.Tick(input, diffuser_delays_[2], 0.625f);
        input = diffuser4_.Tick(input, diffuser_delays_[3], 0.625f);

        SimdType tank_val = input.Shuffle<0, 0, 1, 1>() + tank_out_.Shuffle<1, 0, 3, 2>() * tank_decay_;
        tank_val = tank_apf1_.Tick(tank_val, apf1_delay_ + tank_lfo_.Tick() * mod_depth_, -0.7f);
        tank_delay1_.Push(tank_val);
        tank_val = tank_delay1_.GetAfterPush(delay1_delay_);
        tank_val = tank_damping_.TickLowpass(tank_val, SimdType::FromSingle(damp_coeff_));
        tank_val *= tank_decay_;
        tank_val = tank_apf2_.Tick(tank_val, apf2_delay_, 0.5f);
        tank_delay2_.Push(tank_val);
        tank_val = tank_delay2_.GetAfterPush(delay2_delay_);
        tank_out_ = tank_val;

        SimdType v0 = tank_delay1_.GetAfterPush(tank_output_taps_[0]);
        SimdType v1 = tank_delay1_.GetAfterPush(tank_output_taps_[1]);
        SimdType v2 = tank_apf2_.delay_.GetAfterPush(tank_output_taps_[2]);
        SimdType v3 = tank_delay2_.GetAfterPush(tank_output_taps_[3]);
        SimdType v4 = tank_delay1_.GetAfterPush(tank_output_taps_[4]);
        SimdType v5 = tank_apf2_.delay_.GetAfterPush(tank_output_taps_[5]);
        SimdType v6 = tank_delay2_.GetAfterPush(tank_output_taps_[6]);
        SimdType wet_output;
        wet_output.x[0] = v0.x[1] + v1.x[1] - v2.x[1] + v3.x[1] - v4.x[0] - v5.x[0] - v6.x[0];
        wet_output.x[1] = v0.x[0] + v1.x[0] - v2.x[0] + v3.x[0] - v4.x[1] - v5.x[1] - v6.x[1];
        wet_output.x[2] = v0.x[3] + v1.x[3] - v2.x[3] + v3.x[3] - v4.x[2] - v5.x[2] - v6.x[2];
        wet_output.x[3] = v0.x[2] + v1.x[2] - v2.x[2] + v3.x[2] - v4.x[3] - v5.x[3] - v6.x[3];
        SimdType output;
        output.x[0] = wet_output.x[0] + wet_output.x[2];
        output.x[1] = wet_output.x[1] - wet_output.x[3];

        predelay_.Push(output);
        output = predelay_.GetAfterPush(predelay_samples_);
        
        return x + SimdType::FromSingle(mix_) * (output - x);
    }

    // -------------------- parameters --------------------
    void SetMix(float mix) noexcept {
        mix_ = mix;
    }
    void SetPredelay(float ms) noexcept {
        predelay_samples_ = fs_ * ms / 1000.0f;
    }
    void SetLowpass(float freq) noexcept {
        lowpass_coeff_ = input_lowpass_.ComputeCoeff(qwqdsp::convert::Freq2W(freq, fs_));
    }
    void SetDecay(float decay) noexcept {
        tank_decay_ = decay;
    }
    void SetSize(float size) noexcept {
        size /= kMaxSize;
        apf1_delay_ = max_apf1_delay_ * size;
        apf2_delay_ = max_apf2_delay_ * size;
        delay1_delay_ = max_delay1_delay_ * size;
        delay2_delay_ = max_delay2_delay_ * size;
        mod_depth_ = max_mod_depth_ * size;
        for (size_t i = 0; i < 7; ++i) {
            tank_output_taps_[i] = base_output_taps_[i] * size;
        }
    }
    void SetDamping(float freq) noexcept {
        damp_coeff_ = tank_damping_.ComputeCoeff(qwqdsp::convert::Freq2W(freq, fs_));
    }
private:
    float fs_{};
    float mix_{};

    float lowpass_coeff_{};
    OnepoleTPT<SimdType> input_lowpass_;

    float predelay_samples_{};
    DelayLine<SimdType, DelayLineInterp::PCHIP> predelay_;

    float tank_decay_{};
    SimdType tank_out_{};
    SimdType max_apf1_delay_{};
    SimdType max_apf2_delay_{};
    SimdType max_delay1_delay_{};
    SimdType max_delay2_delay_{};
    SimdType apf1_delay_{};
    SimdType apf2_delay_{};
    SimdType delay1_delay_{};
    SimdType delay2_delay_{};
    float damp_coeff_{};
    float mod_depth_{};
    float max_mod_depth_{};
    OnepoleTPT<SimdType> tank_damping_;
    LFO tank_lfo_;
    DelayLine<SimdType, DelayLineInterp::PCHIP> tank_delay1_;
    DelayLine<SimdType, DelayLineInterp::PCHIP> tank_delay2_;
    DelayAllpass<SimdType, DelayLineInterp::PCHIP> tank_apf1_;
    DelayAllpass<SimdType, DelayLineInterp::PCHIP> tank_apf2_;
    std::array<SimdType, 7> tank_output_taps_{};
    std::array<SimdType, 7> base_output_taps_{};

    std::array<float, 4> diffuser_delays_{};
    DelayAllpass<SimdType, DelayLineInterp::PCHIP> diffuser1_;
    DelayAllpass<SimdType, DelayLineInterp::PCHIP> diffuser2_;
    DelayAllpass<SimdType, DelayLineInterp::PCHIP> diffuser3_;
    DelayAllpass<SimdType, DelayLineInterp::PCHIP> diffuser4_;
};
}
