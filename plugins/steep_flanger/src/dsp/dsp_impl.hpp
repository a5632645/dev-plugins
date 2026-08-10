#pragma once
#include <numbers>
#include <string_view>

#include "dsp_shared.hpp"
#include "fir_dsp.hpp"
#include "global.hpp"
#include "idsp.hpp"
#include "iir_dsp.hpp"
#include "params.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"

namespace steep_flanger {

// ------------------------------------------------------------
// DspImpl
// 组合 FirDsp / IirDsp，按 iir_mode 分派处理
// ------------------------------------------------------------
template <simd::Inst inst, class SimdT>
class DspImpl : public Idsp {
public:
    ~DspImpl() override = default;

    void Init(float fs) override {
        shared_.fs_ = fs;
        shared_.barber_phase_smoother_.SetSmoothTime(20.0f, fs);
        shared_.barber_phase_ = 0;

        fir_.Init(fs);
        iir_.Init(fs);
    }

    void Reset() override {
        shared_.phase_ = 0;
        shared_.barber_phase_smoother_.Reset();
        shared_.barber_phase_ = 0;

        fir_.Reset();
        iir_.Reset();
    }

    void Update(Params& p, juce::AudioPlayHead* playhead) override {
        if (p.has_fir_source_from_state_.exchange(false, std::memory_order_acq_rel)) {
            p.fir_source.store(p.fir_source_from_state_, std::memory_order_release);
        }

        // 快照音频线程所需的参数值
        auto& s = shared_.param_;
        s.delay_ms = p.delay.Get();
        s.depth_ms = p.depth.Get();
        s.lfo_phase = p.lfo_phase.Get();
        s.fir_cutoff = p.fir_cutoff.Get();
        s.fir_coeff_len = static_cast<size_t>(p.fir_coeff_len.Get());
        s.fir_side_lobe = p.fir_side_lobe.Get();
        s.fir_min_phase = p.fir_min_phase.Get();
        s.fir_highpass = p.fir_highpass.Get();
        s.iir_mode = p.iir_mode.Get();
        s.drywet = p.drywet.Get();
        s.feedback = p.feedback.Get();
        s.damp_pitch = p.damp_pitch.Get();
        s.barber_phase = p.barber_phase.Get();
        s.barber_enable = p.barber_enable.Get();
        s.barber_stereo_phase = p.barber_stereo.Get() * std::numbers::pi_v<float> / 2;
        s.iir_num_filters = static_cast<size_t>(p.iir_filter_num.Get());
        s.ripple = p.iir_ripple.Get();

        // bpm 同步 LFO：同步相位，并更新快照中的每 block 频率
        auto lfo_info = p.delay_lfo.SyncBpm2(playhead);
        if (lfo_info.sync_lfo) {
            shared_.phase_ = lfo_info.lfo_phase;
        }
        auto barber_lfo_info = p.barber_lfo.SyncBpm2(playhead);
        if (barber_lfo_info.sync_lfo) {
            shared_.barber_phase_ = barber_lfo_info.lfo_phase * std::numbers::pi_v<float> * 2;
        }
        shared_.param_.lfo_freq = lfo_info.lfo_freq;
        shared_.param_.barber_speed = barber_lfo_info.lfo_freq;

        // 系数重建（由 Params::parameterValueChanged 或 UI 设置的标志触发）
        if (!shared_.param_.iir_mode && p.should_update_fir_.exchange(false, std::memory_order_acq_rel)) {
            fir_.UpdateCoeff(p);
        }
        if (shared_.param_.iir_mode && p.should_update_iir_.exchange(false, std::memory_order_acq_rel)) {
            iir_.UpdateCoeff();
        }
    }

    void Process(float* left, float* right, int num_samples) override {
        if (!shared_.param_.iir_mode) {
            fir_.Process(left, right, num_samples);
        }
        else {
            iir_.Process(left, right, num_samples);
        }
    }

    std::string_view InstName() override {
        return INST_NAME;
    }

    void GetCoeffs(float* out, int n) override {
        fir_.GetCoeffs(out, n);
    }

    bool ExchangeNewCoeff() override {
        return fir_.ExchangeNewCoeff();
    }
private:
    // ----------------------------------------
    // members
    // ----------------------------------------
    DspShared<inst, SimdT> shared_;
    FirDsp<inst, SimdT> fir_{shared_};
    IirDsp<inst, SimdT> iir_{shared_};
};

} // namespace steep_flanger

#pragma GCC diagnostic pop
