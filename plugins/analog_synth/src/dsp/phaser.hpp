#pragma once
#include <span>
#include <cmath>
#include <array>
#include <qwqdsp/psimd/float32x4.hpp>
#include <qwqdsp/polymath.hpp>

namespace analogsynth {
class SvfTPTSimd {
public:
    using SimdType = qwqdsp_psimd::Float32x4;

    void Reset() noexcept {
        s1_ = SimdType{};
        s2_ = SimdType{};
    }

    /**
     * @param w [0, pi]
     * @param r2 <0:不稳定 =0:无阻尼 0~1:复共轭极点 >1:分裂成两个单极点
     */
    void SetCoeffSVF(SimdType const& w, SimdType const& r2) noexcept {
        g_ = w * SimdType::FromSingle(0.5f);
        for (auto& x : g_.x) {
            x = std::tan(x);
        }
        R2_ = r2;
        g1_ = r2 + g_;
        d_ = SimdType::FromSingle(1) / (SimdType::FromSingle(1) + r2 * g_ + g_ * g_);
    }

    void SetCoeffQ(SimdType const& w, SimdType const& Q) noexcept {
        SetCoeffSVF(w, SimdType::FromSingle(1) / Q);
    }

    SimdType TickBandpass(SimdType const& x) noexcept {
        SimdType bp = d_ * (g_ * (x - s2_) + s1_);
        SimdType bp2 = bp + bp;
        s1_ = bp2 - s1_;
        SimdType v22 = g_ * bp2;
        s2_ += v22;
        return bp;
    }

    SimdType TickAllpass(SimdType x) noexcept {
        return x - SimdType::FromSingle(2) * TickBandpass(x * R2_);
    }
private:
    SimdType R2_{};
    SimdType g_{};
    SimdType g1_{};
    SimdType d_{};
    SimdType s1_{};
    SimdType s2_{};
};

class Phaser {
public:
    using SimdType = SvfTPTSimd::SimdType;

    void Reset() noexcept {
        for (auto& s : states_) {
            s.Reset();
        }
        fb_value_ = SimdType{};
    }

    void SyncBpm(float phase) noexcept {
        phase_ = phase;
    }

    void Process(std::span<qwqdsp_psimd::Float32x4> block) noexcept {
        float phase_inc = rate * static_cast<float>(block.size()) / fs;
        phase_ += phase_inc;
        phase_ -= std::floor(phase_);
        float left_phase = phase_;
        float t;
        float right_phase = std::modf(phase_ + stereo, &t);

        constexpr float twopi = std::numbers::pi_v<float> * 2;
        constexpr float pi = std::numbers::pi_v<float>;
        left_phase = qwqdsp::polymath::SinParabola(left_phase * twopi - pi);
        right_phase = qwqdsp::polymath::SinParabola(right_phase * twopi - pi);
        left_phase = 0.5f * left_phase + 0.5f;
        right_phase = 0.5f * right_phase + 0.5f;
        float left_w = std::lerp(begin_w, end_w, left_phase);
        float right_w = std::lerp(begin_w, end_w, right_phase);
        for (auto& filter : states_) {
            filter.SetCoeffQ(SimdType{left_w, right_w}, SimdType::FromSingle(Q));
        }
        
        float drt_mix = 1.0f - mix;
        float wet_mix = mix;
        for (auto& x : block) {
            SimdType push = x + fb_value_ * SimdType::FromSingle(feedback);
            for (auto& filter : states_) {
                push = filter.TickAllpass(push);
            }
            fb_value_ = push;
            x = x * SimdType::FromSingle(drt_mix) + push * SimdType::FromSingle(wet_mix);
        }
    }

    float mix{};     // [0, 1]
    float begin_w{}; // [0, pi]
    float end_w{};   // [0, pi]
    float rate{};    // hz
    float feedback{};// [-1, 1]
    float Q{};       // [0.1, 10]
    float stereo{};  // [0, 1]
    float fs{};
private:
    float phase_{};
    SimdType fb_value_{};
    std::array<SvfTPTSimd, 4> states_;
};
}
