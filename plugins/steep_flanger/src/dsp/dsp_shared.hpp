#pragma once
#include "pluginshared/simd/inst.hpp"
#include "pluginshared/simd/simd.hpp"
#include "qwqdsp/misc/smoother.hpp"

namespace steep_flanger {

// ------------------------------------------------------------
// DspShared
// FIR/IIR 共用的运行上下文：参数快照、采样率、延迟时间 LFO、barberpole
// ------------------------------------------------------------
template <simd::Inst inst, class SimdT>
struct DspShared {
    // 处理所需的参数快照（由 Update 从 Params 拷贝，仅音频线程访问）
    struct ParamSnapshot {
        float delay_ms{};
        float depth_ms{};
        float lfo_freq{};
        float lfo_phase{};
        float drywet{};
        float fir_cutoff{};
        size_t fir_coeff_len{};
        float fir_side_lobe{};
        bool fir_min_phase{};
        bool fir_highpass{};
        float feedback{};
        float damp_pitch{};
        float barber_phase{};
        float barber_speed{};
        bool barber_enable{};
        float barber_stereo_phase{};
        bool iir_mode{};
        size_t iir_num_filters{};
        float ripple{};
    };
    ParamSnapshot param_;

    float fs_{};

    // delay time lfo
    float phase_{};
    simd::Float128 last_exp_delay_samples_{};
    simd::Float128 last_delay_samples_{};

    // barberpole
    qwqdsp_misc::ExpSmoother barber_phase_smoother_;
    float barber_phase_{}; // rad
};

} // namespace steep_flanger
