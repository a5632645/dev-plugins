#pragma once
#include <span>
#include <qwqdsp/fx/delay_line_simd.hpp>
#include <qwqdsp/psimd/float32x4.hpp>
#include <qwqdsp/filter/one_pole_tpt_simd.hpp>
#include <qwqdsp/convert.hpp>

namespace analogsynth {
class Delay {
public:
    using SimdType = qwqdsp::psimd::Float32x4;
    static constexpr float kMaxDelayMs = 1000.0f;

    void Init(float _fs) noexcept {
        delay_.Init(kMaxDelayMs, _fs);
    }

    void Reset() noexcept {
        delay_.Reset();
    }

    void Process(std::span<qwqdsp::psimd::Float32x4> block) noexcept {
        if (pingpong) {
            ProcessPingpong(block);
        }
        else {
            ProcessMono(block);
        }
    }

    bool pingpong{false};
    float fs{};
    float delay_ms{};
    float lowcut_f{};
    float highcut_f{};
    float feedback{};
    float mix{};
private:
    void ProcessMono(std::span<qwqdsp::psimd::Float32x4> block) noexcept {
        float delay_samples = delay_ms * fs / 1000.0f;
        delay_samples = std::max(delay_samples, 1.0f);
        float lp_coeff = lp_filter_.ComputeCoeff(qwqdsp::convert::Freq2W(lowcut_f, fs));
        float hp_coeff = hp_filter_.ComputeCoeff(qwqdsp::convert::Freq2W(highcut_f, fs));
        float dry_mix = 1 - mix;
        float wet_mix = mix;
        for (auto& x : block) {
            auto v = delay_.GetBeforePush(delay_samples);
            auto fb = v * SimdType::FromSingle(feedback);
            fb = lp_filter_.TickLowpass(fb, SimdType::FromSingle(lp_coeff));
            fb = hp_filter_.TickHighpass(fb, SimdType::FromSingle(hp_coeff));
            delay_.Push(x + fb);
            x = x * SimdType::FromSingle(dry_mix) + v * SimdType::FromSingle(wet_mix);
        }
    }

    void ProcessPingpong(std::span<qwqdsp::psimd::Float32x4> block) noexcept {
        float delay_samples = delay_ms * fs / 1000.0f;
        delay_samples = std::max(delay_samples, 1.0f);
        float lp_coeff = lp_filter_.ComputeCoeff(qwqdsp::convert::Freq2W(lowcut_f, fs));
        float hp_coeff = hp_filter_.ComputeCoeff(qwqdsp::convert::Freq2W(highcut_f, fs));
        float dry_mix = 1 - mix;
        float wet_mix = mix;
        for (auto& x : block) {
            auto v = delay_.GetBeforePush(delay_samples);
            auto fb = v * SimdType::FromSingle(feedback);
            fb = lp_filter_.TickLowpass(fb, SimdType::FromSingle(lp_coeff));
            fb = hp_filter_.TickHighpass(fb, SimdType::FromSingle(hp_coeff));
            fb = fb.Shuffle<1, 0, 0, 0>();
            fb.x[0] += x.x[0];
            fb.x[0] += x.x[1];
            delay_.Push(fb);
            x = x * SimdType::FromSingle(dry_mix) + v * SimdType::FromSingle(wet_mix);
        }
    }

    qwqdsp::fx::DelayLineSIMD<qwqdsp::psimd::Float32x4, qwqdsp::fx::DelayLineInterpSIMD::PCHIP> delay_;
    qwqdsp::filter::OnePoleTPTSimd<qwqdsp::psimd::Float32x4> lp_filter_;
    qwqdsp::filter::OnePoleTPTSimd<qwqdsp::psimd::Float32x4> hp_filter_;
};
}