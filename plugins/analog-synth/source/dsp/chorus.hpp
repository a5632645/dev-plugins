#pragma once
#include <span>
#include <qwqdsp/fx/delay_line_simd.hpp>
#include <qwqdsp/psimd/float32x4.hpp>
#include <qwqdsp/osciilor/vic_sine_osc.hpp>
#include <qwqdsp/convert.hpp>

namespace analogsynth {
class Chorus {
public:
    using SimdType = qwqdsp::psimd::Float32x4;
    static constexpr float kBaseDelayMs = 15.0f;
    static constexpr float kModulateDelayMs = 30.0f;
    static constexpr float kMaxDelayMs = kBaseDelayMs + kModulateDelayMs;

    void Init(float _fs) noexcept {
        delay_.Init(kMaxDelayMs, _fs);
    }

    void Reset() noexcept {
        delay_.Reset();
        mod_osc_.Reset();
    }

    void SyncBpm(float phase) noexcept {
        mod_osc_.Reset(phase * std::numbers::pi_v<float> * 2);
    }

    void Process(std::span<qwqdsp::psimd::Float32x4> block) noexcept {
        mod_osc_.SetFreq(rate, fs);
        mod_osc_.KeepAmp();
        float base_samples = delay * kBaseDelayMs * fs / 1000.0f;
        SimdType base_delay{
            base_samples * 0.25f, base_samples * 0.5f, base_samples, base_samples * 0.75f
        };
        float mod_samples = depth * kModulateDelayMs * fs / 1000.0f;
        float dry_mix = 1 - mix;
        float wet_mix = mix * 0.5f;
        for (auto& x : block) {
            mod_osc_.Tick();
            auto first = mod_osc_.GetCpx();
            SimdType mod_val{
                first.real(), -first.real(), first.imag(), -first.imag()
            };
            mod_val = 0.5f * mod_val + 0.5f;
            SimdType delay_samples = mod_val * SimdType::FromSingle(mod_samples) + base_delay;
            delay_samples = SimdType::Max(delay_samples, SimdType::FromSingle(1));

            auto v = delay_.GetBeforePush(delay_samples);
            auto fb = v * SimdType::FromSingle(feedback) + x.Shuffle<0, 1, 0, 1>();
            delay_.Push(fb);
            SimdType output{
                v.x[0] + v.x[2], v.x[1] + v.x[3]
            };
            x = x * SimdType::FromSingle(dry_mix) + output * SimdType::FromSingle(wet_mix);
        }
    }

    float fs{};
    float delay{};
    float rate{};
    float feedback{};
    float mix{};
    float depth{};
private:
    qwqdsp::oscillor::VicSineOsc mod_osc_;
    qwqdsp::fx::DelayLineSIMD<qwqdsp::psimd::Float32x4, qwqdsp::fx::DelayLineInterpSIMD::PCHIP> delay_;
};
}