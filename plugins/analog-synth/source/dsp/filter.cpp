#include "filter.hpp"

namespace analogsynth {
void Filter::Process(Synth& synth, size_t channel, std::span<float> block) noexcept {
    if (!param_enable.Get()) return;

    float pitch = param_cutoff.GetModCR(channel);
    float freq = qwqdsp::convert::Pitch2Freq(pitch);
    float w = qwqdsp::convert::Freq2W(freq, fs_);
    auto type = static_cast<FilterType>(param_type.Get());
    switch (type) {
        case FilterType_SVF: {
            auto& filter = svf_[channel];

            float r2 = param_resonance.GetNormalModCR(channel);
            r2 = std::lerp(2.0f, -0.01f, r2);
            filter.SetCoeffSVF(w, r2);
            
            float morph = param_morph.GetNormalModCR(channel);
            float lp_mix = std::max(0.0f, 1 - 2 * morph);
            float hp_mix = std::max(0.0f, 2 * morph - 1);
            float bp_mix = 1 - 2 * std::abs(morph - 0.5f);

            float wet_mix = param_mix.GetModCR(channel);
            float dry_mix = 1 - wet_mix;
            
            for (auto& x : block) {
                auto[hp,bp,lp] = filter.TickMultiMode(x);
                float wet = hp * hp_mix + bp * bp_mix + lp * lp_mix;
                x *= dry_mix;
                x += wet * wet_mix;
            }
            break;
        }
        case FilterType_Ladder4: {
            auto& filter = ladder_[channel];

            float k = param_resonance.GetNormalModCR(channel);
            k = std::lerp(0.0f, 4.01f, k);
            float Q = param_var1.GetNormalModCR(channel);
            Q = std::lerp(0.1f, 10.0f, Q);
            ladder_[channel].Set(w, k);

            float morph = param_morph.GetNormalModCR(channel);
            float lp_mix = std::max(0.0f, 1 - 2 * morph);
            float hp_mix = std::max(0.0f, 2 * morph - 1);
            float bp_mix = 1 - 2 * std::abs(morph - 0.5f);

            float wet_mix = param_mix.GetModCR(channel);
            float dry_mix = 1 - wet_mix;

            for (auto& x : block) {
                auto o = filter.TickMultiMode(x);
                float wet = o.lp4;
                x *= dry_mix;
                x += wet * wet_mix;
            }
        }
        case FilterType_Ladder8: {
            auto& filter = ladder8_[channel];

            float k = param_resonance.GetNormalModCR(channel);
            k = std::lerp(0.0f, 1.89f, k);
            float Q = param_var1.GetNormalModCR(channel);
            Q = std::lerp(0.1f, 10.0f, Q);
            filter.Set(w, k);

            float morph = param_morph.GetNormalModCR(channel);
            float lp_mix = std::max(0.0f, 1 - 2 * morph);
            float hp_mix = std::max(0.0f, 2 * morph - 1);
            float bp_mix = 1 - 2 * std::abs(morph - 0.5f);

            float wet_mix = param_mix.GetModCR(channel);
            float dry_mix = 1 - wet_mix;

            for (auto& x : block) {
                auto o = filter.Tick(x);
                float wet = o.lp8;
                x *= dry_mix;
                x += wet * wet_mix;
            }
            break;
        }
        case FilterType_SallenKey: {
            auto& filter = sallen_key_[channel];

            float k = param_resonance.GetNormalModCR(channel);
            k = std::lerp(0.0f, 2.01f, k);
            float Q = param_var1.GetNormalModCR(channel);
            Q = std::lerp(0.1f, 10.0f, Q);
            filter.Set(w, k);

            float morph = param_morph.GetNormalModCR(channel);
            float lp_mix = std::max(0.0f, 1 - 2 * morph);
            float hp_mix = std::max(0.0f, 2 * morph - 1);
            float bp_mix = 1 - 2 * std::abs(morph - 0.5f);

            float wet_mix = param_mix.GetModCR(channel);
            float dry_mix = 1 - wet_mix;

            for (auto& x : block) {
                auto o = filter.Tick(x);
                float wet = o.hp2 * hp_mix + o.bp2 * bp_mix + o.lp2 * lp_mix;
                x *= dry_mix;
                x += wet * wet_mix;
            }
            break;
        }
    }
}
}