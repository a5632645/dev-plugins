#pragma once
#include <array>
#include <atomic>
#include <numbers>
#include "global.hpp"
#include "pluginshared/bpm_sync_lfo.hpp"
#include "pluginshared/wrap_parameters.hpp"

// ---------------------------------------- parameters ----------------------------------------
namespace steep_flanger {

class Params : public juce::AudioProcessorParameter::Listener {
public:
    // FIR 系数来源
    enum FirSource {
        kWindowSinc,
        kTimeCoeff
    };

    // -------------------- delay time --------------------
    pluginshared::FloatParam delay{
        "delay", {0.0f, global::kMaxDelayMs, 0.01f},
         1.0f
    };
    pluginshared::FloatParam depth{
        "depth", {0.0f, global::kModuDelayMs, 0.01f},
         1.0f
    };
    pluginshared::FloatParam lfo_phase{
        "phase", {0.0f, 1.0f, 0.01f},
         0.03f
    };

    // -------------------- fir design --------------------
    pluginshared::FloatParam fir_cutoff{
        "cutoff", {0.01f, 3.0f, 0.01f},
         std::numbers::pi_v<float> / 2
    };
    pluginshared::FloatParam fir_coeff_len{
        "coeff_len", {4.0f, static_cast<float>(global::kMaxCoeffLen), 1.0f},
         8.0f
    };
    pluginshared::FloatParam fir_side_lobe{
        "side_lobe", {20.0f, 100.0f, 0.1f},
         40.0f
    };
    pluginshared::BoolParam fir_min_phase{"minum_phase", false};
    pluginshared::BoolParam fir_highpass{"highpass", false};
    pluginshared::BoolParam iir_mode{"iir_enable", false};

    // -------------------- drywet --------------------
    pluginshared::FloatParam drywet{
        "drywet", {0.0f, 1.0f, 0.01f},
         1.0f
    };

    // -------------------- feedback --------------------
    pluginshared::FloatParam feedback{
        "fb_value", {-0.95f, 0.95f, 0.01f},
         0.0f
    };
    pluginshared::FloatParam damp_pitch{
        "fb_damp", {0.0f, 140.0f, 0.01f},
         90.0f
    };

    // -------------------- barberpole --------------------
    pluginshared::FloatParam barber_phase{
        "barber_phase", {0.0f, 1.0f, 0.01f},
         0.0f
    };
    pluginshared::FloatParam barber_stereo{
        "barber_stereo", {0.0f, 1.0f, 0.01f},
         0.0f
    };
    pluginshared::BoolParam barber_enable{"barber_enable", false};

    // -------------------- iir --------------------
    pluginshared::IntParam iir_filter_num{"iir_filter_num", 1, static_cast<int>(global::kIirMaxNumFilters), 4};
    pluginshared::FloatParam iir_ripple{
        "ripple", {0.1f, 20.0f, 0.1f},
         1.0f
    };

    // -------------------- bpm sync lfo --------------------
    pluginshared::BpmSyncLFO delay_lfo{"speed", 0.0f, 10.0f, 0.01f, 0.4f, true, "0", "1/64T", 0.2f, "4", false};
    pluginshared::BpmSyncLFO barber_lfo{"barber_speed", -10.0f,  10.0f, 0.01f, 0.4f, true,
                                        "-1/64T",       "1/64T", 0.2f,  "1",   false};

    // 共享 DSP 控制状态（UI 线程写入，音频线程在 DspImpl::Update 中读取/清除）
    std::atomic<bool> should_update_fir_{};
    std::atomic<bool> should_update_iir_{};
    std::atomic<FirSource> fir_source{kWindowSinc};
    juce::SpinLock custom_coeffs_lock_;
    std::array<float, global::kMaxCoeffLen> custom_coeffs_{};
    std::array<float, global::kMaxCoeffLen> custom_spectral_gains{};

    // 状态恢复暂存（setStateInformation 写入，音频线程 processBlock 消费后清除）
    std::atomic<bool> has_fir_source_from_state_{};
    FirSource fir_source_from_state_{kWindowSinc};

    void BuildLayout(juce::AudioProcessorValueTreeState::ParameterLayout& layout) {
        delay_lfo += layout;
        barber_lfo += layout;
        layout += delay;
        layout += depth;
        layout += lfo_phase;
        layout += fir_cutoff;
        layout += fir_coeff_len;
        layout += fir_side_lobe;
        layout += fir_min_phase;
        layout += fir_highpass;
        layout += iir_mode;
        layout += drywet;
        layout += feedback;
        layout += damp_pitch;
        layout += barber_phase;
        layout += barber_stereo;
        layout += barber_enable;
        layout += iir_filter_num;
        layout += iir_ripple;
    }

    void BeginListening() {
        delay.ptr_->addListener(this);
        depth.ptr_->addListener(this);
        lfo_phase.ptr_->addListener(this);
        fir_cutoff.ptr_->addListener(this);
        fir_coeff_len.ptr_->addListener(this);
        fir_side_lobe.ptr_->addListener(this);
        fir_min_phase.ptr_->addListener(this);
        fir_highpass.ptr_->addListener(this);
        iir_mode.ptr_->addListener(this);
        drywet.ptr_->addListener(this);
        feedback.ptr_->addListener(this);
        damp_pitch.ptr_->addListener(this);
        barber_phase.ptr_->addListener(this);
        barber_stereo.ptr_->addListener(this);
        barber_enable.ptr_->addListener(this);
        iir_filter_num.ptr_->addListener(this);
        iir_ripple.ptr_->addListener(this);

        fir_cutoff_idx_ = fir_cutoff.ptr_->getParameterIndex();
        fir_coeff_len_idx_ = fir_coeff_len.ptr_->getParameterIndex();
        fir_side_lobe_idx_ = fir_side_lobe.ptr_->getParameterIndex();
        fir_min_phase_idx_ = fir_min_phase.ptr_->getParameterIndex();
        fir_highpass_idx_ = fir_highpass.ptr_->getParameterIndex();
        iir_filter_num_idx_ = iir_filter_num.ptr_->getParameterIndex();
        iir_ripple_idx_ = iir_ripple.ptr_->getParameterIndex();
    }

    void EndListening() {
        delay.ptr_->removeListener(this);
        depth.ptr_->removeListener(this);
        lfo_phase.ptr_->removeListener(this);
        fir_cutoff.ptr_->removeListener(this);
        fir_coeff_len.ptr_->removeListener(this);
        fir_side_lobe.ptr_->removeListener(this);
        fir_min_phase.ptr_->removeListener(this);
        fir_highpass.ptr_->removeListener(this);
        iir_mode.ptr_->removeListener(this);
        drywet.ptr_->removeListener(this);
        feedback.ptr_->removeListener(this);
        damp_pitch.ptr_->removeListener(this);
        barber_phase.ptr_->removeListener(this);
        barber_stereo.ptr_->removeListener(this);
        barber_enable.ptr_->removeListener(this);
        iir_filter_num.ptr_->removeListener(this);
        iir_ripple.ptr_->removeListener(this);
    }
private:
    void parameterValueChanged(int parameterIndex, float newValue) override {
        juce::ignoreUnused(newValue);
        if (parameterIndex == fir_cutoff_idx_ || parameterIndex == fir_highpass_idx_) {
            fir_source.store(kWindowSinc, std::memory_order_release);
            should_update_fir_.store(true, std::memory_order_release);
            should_update_iir_.store(true, std::memory_order_release);
        }
        else if (parameterIndex == fir_side_lobe_idx_) {
            fir_source.store(kWindowSinc, std::memory_order_release);
            should_update_fir_.store(true, std::memory_order_release);
        }
        else if (parameterIndex == fir_coeff_len_idx_ || parameterIndex == fir_min_phase_idx_) {
            should_update_fir_.store(true, std::memory_order_release);
        }
        else if (parameterIndex == iir_filter_num_idx_ || parameterIndex == iir_ripple_idx_) {
            should_update_iir_.store(true, std::memory_order_release);
        }
    }

    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {
        juce::ignoreUnused(parameterIndex, gestureIsStarting);
    }

    int fir_cutoff_idx_{-1};
    int fir_coeff_len_idx_{-1};
    int fir_side_lobe_idx_{-1};
    int fir_min_phase_idx_{-1};
    int fir_highpass_idx_{-1};
    int iir_filter_num_idx_{-1};
    int iir_ripple_idx_{-1};
};

} // namespace steep_flanger
