#pragma once
#include <span>
#include <qwqdsp/convert.hpp>
#include <qwqdsp/polymath.hpp>
#include "wrap_parameters.hpp"
#include "constant.hpp"

namespace analogsynth {
class Synth;

// ----------------------------------------
// copy of filter
// ----------------------------------------
class SvfTPT {
public:
    void Reset() noexcept {
        s1_ = 0;
        s2_ = 0;
    }

    /**
     * @param w [0, pi]
     * @param r2 <0:不稳定 =0:无阻尼 0~1:复共轭极点 >1:分裂成两个单极点
     */
    void SetCoeffSVF(float w, float r2) noexcept {
        g_ = std::tan(w / 2);
        R2_ = r2;
        d_ = 1 / (1 + r2 * g_ + g_ * g_);
    }

    /**
     * @return [hp, bp, lp]
     */
    std::array<float, 3> TickMultiMode(float x) noexcept {
        float hp = (x - (R2_ + g_) * s1_ - s2_) * d_;
        float v1 = g_ * hp;
        float bp = v1 + s1_;
        float v2 = g_ * bp;
        float lp = v2 + s2_;
        s1_ = qwqdsp::polymath::ArctanPade((bp + v1) / 2) * 2;
        // s2_ = qwqdsp::polymath::TanhFastest((lp + v2) / 2) * 2;
        s2_ = lp + v2;
        return {hp, bp, lp};
    }
private:
    float R2_{};
    float g_{};
    float d_{};
    float s1_{};
    float s2_{};
};

class Ladder {
public:
    void Reset() noexcept {
        s1_ = 0;
        s2_ = 0;
        s3_ = 0;
        s4_ = 0;
    }

    auto TickMultiMode(float x) noexcept {
        struct Output {
            float hp;
            float lp1;
            float lp2;
            float lp3;
            float lp4;
        };
        Output r;

        float S = g2_ * s2_ + glp_ * (s3_ + s1_ * glp_) + s4_;
        S /= (1 + g_);
        r.hp = (x - k_ * S) / (1 + k_ * g4_);
        r.lp1 = TickLpTPT(qwqdsp::polymath::ArctanPade(r.hp), s1_, glp_);
        r.lp2 = TickLpTPT(r.lp1, s2_, glp_);
        r.lp3 = TickLpTPT(r.lp2, s3_, glp_);
        r.lp4 = TickLpTPT(r.lp3, s4_, glp_);
        return r;
    }

    float TickHighpass(float x) noexcept {
        auto r = TickMultiMode(x);
        return r.hp - 4 * (r.lp1 + r.lp3) + r.lp4 + 6 * r.lp2;
    }

    float TickBandpass(float x) noexcept {
        auto r = TickMultiMode(x);
        return r.lp2 + r.lp4 - 2 * r.lp3;
    }

    /**
     * @param w [0, pi-0.1]
     * @param k [0, 3.99] => [实数极点, 自震荡]
     */
    void Set(float w, float k) noexcept {
        g_ = std::tan(w / 2);
        glp_ = g_ / (1 + g_);
        g2_ = glp_ * glp_;
        g4_ = g2_ * g2_;
        k_ = k;
    }
private:
    static float TickLpTPT(float x, float& s, float geps) noexcept {
        float const delta = geps * (x - s);
        s += delta;
        float const y = s;
        s += delta;
        return y;
    }

    static float TickHpTPT(float x, float& s, float geps) noexcept {
        float const delta = geps * (x - s);
        s += delta;
        float const y = s;
        s += delta;
        return x - y;
    }

    float k_{};
    float g_{};
    float g2_{};
    float g4_{};
    float glp_{};
    float s1_{};
    float s2_{};
    float s3_{};
    float s4_{};
};

class Ladder8Pole {
public:
    void Reset() noexcept {
        std::fill_n(s_, 8, 0.0f);
    }

    /**
     * @param k [0,1.884]
     */
    void Set(float w, float k) noexcept {
        g_ = std::tan(w / 2) * (1 + std::numbers::sqrt2_v<float>);
        k_ = k;
    }

    auto Tick(float x) noexcept {
        struct Output {
            float lp1;
            float lp2;
            float lp3;
            float lp4;
            float lp5;
            float lp6;
            float lp7;
            float lp8;
        } output;

        float B = 1 / (1 + g_);
        float A = g_ * B;
        float S = s_[7] + A * (s_[6] + A * (s_[5] + A * (s_[4] + A * (s_[3] + A * (s_[2] + A * (s_[1] + A * s_[0]))))));
        float u = x - B * k_ * S;
        u /= (1 + k_ * A * A * A * A * A * A * A * A);

        output.lp1 = TickLpTPT(qwqdsp::polymath::ArctanPade(u), s_[0], A);
        output.lp2 = TickLpTPT(output.lp1, s_[1], A);
        output.lp3 = TickLpTPT(output.lp2, s_[2], A);
        output.lp4 = TickLpTPT(output.lp3, s_[3], A);
        output.lp5 = TickLpTPT(output.lp4, s_[4], A);
        output.lp6 = TickLpTPT(output.lp5, s_[5], A);
        output.lp7 = TickLpTPT(output.lp6, s_[6], A);
        output.lp8 = TickLpTPT(output.lp7, s_[7], A);

        return output;
    }
private:
    static float TickLpTPT(float x, float& s, float geps) noexcept {
        float const delta = geps * (x - s);
        s += delta;
        float const y = s;
        s += delta;
        return y;
    }

    float s_[8]{};
    float g_{};
    float k_{};
};

class TransposeSallenKey {
public:
    void Reset() noexcept {
        s1_ = 0;
        s2_ = 0;
    }

    auto Tick(float x) noexcept {
        struct Output {
            float lp2;
            float bp2;
            float hp2;
        } output;

        float G = (1 + g_);
        float u = x * G * G - G * k_ * s2_ + k_ * s1_;
        u /= (G * G - g_ * k_);
        float lp1 = TickLpTPT(qwqdsp::polymath::ArctanPade(u / 2) * 2, s1_, glp_);
        float hp1 = u - lp1;
        output.lp2 = TickLpTPT(lp1, s2_, glp_);
        output.bp2 = lp1 - output.lp2;
        output.hp2 = hp1 - output.bp2;
        return output;
    }

    /**
     * @param w [0,pi]
     * @param k [0,2] => [实数极点,自震荡]
     */
    void Set(float w, float k) noexcept {
        g_ = std::tan(w / 2);
        glp_ = g_ / (1 + g_);
        k_ = k;
    }
private:
    static float TickLpTPT(float x, float& s, float geps) noexcept {
        float const delta = geps * (x - s);
        s += delta;
        float const y = s;
        s += delta;
        return y;
    }

    float g_{};
    float glp_{};
    float k_{};
    float s1_{};
    float s2_{};
};

// ----------------------------------------
// filter
// ----------------------------------------
class Filter {
public:
    void Init(float fs) noexcept {
        fs_ = fs;
    }

    void Reset() noexcept {
        for (auto& f : svf_) {
            f.Reset();
        }
        for (auto& f : ladder_) {
            f.Reset();
        }
        for (auto& f : sallen_key_) {
            f.Reset();
        }
        for (auto& f : ladder8_) {
            f.Reset();
        }
    }

    void Process(Synth& synth, size_t channel, std::span<float> block) noexcept;

    void BuildParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout) {
        layout.add(param_cutoff.Build());
        layout.add(param_resonance.Build());
        layout.add(param_mix.Build());
        layout.add(param_morph.Build());
        layout.add(param_var1.Build());
        layout.add(param_var2.Build());
        layout.add(param_var3.Build());
        layout.add(param_enable.Build());
        layout.add(param_type.Build());
    }

    // filter
    BoolParam param_enable{"filter.enable", false};
    ChoiceParam param_type{"filter.type",
        juce::StringArray{
            "SVF", "Ladder4", "SallenKey", "Ladder8"
        },
        "SVF"
    };
    enum FilterType {
        FilterType_SVF = 0,
        FilterType_Ladder4,
        FilterType_SallenKey,
        FilterType_Ladder8
    };
    FloatParam param_cutoff{"filter.cutoff",
        juce::NormalisableRange<float>{qwqdsp::convert::Freq2Pitch(20.0f), qwqdsp::convert::Freq2Pitch(20000.0f), 0.1f},
        qwqdsp::convert::Freq2Pitch(1000.0f)
    };
    FloatParam param_resonance{"filter.resonance",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.01f},
        1.0f / std::numbers::sqrt2_v<float>
    };
    FloatParam param_mix{"filter.mix",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.01f},
        1.0f
    };
    FloatParam param_morph{"filter.morph",
        juce::NormalisableRange<float>{0.0f,1.0f,0.01f},
        0.0f
    };
    FloatParam param_var1{"filter.var1",
        juce::NormalisableRange<float>{0.0f,1.0f,0.01f},
        0.0f
    };
    FloatParam param_var2{"filter.var2",
        juce::NormalisableRange<float>{0.0f,1.0f,0.01f},
        0.0f
    };
    FloatParam param_var3{"filter.var3",
        juce::NormalisableRange<float>{0.0f,1.0f,0.01f},
        0.0f
    };
private:
    float fs_{};
    SvfTPT svf_[kMaxPoly];
    Ladder ladder_[kMaxPoly];
    Ladder8Pole ladder8_[kMaxPoly];
    TransposeSallenKey sallen_key_[kMaxPoly];
};
}