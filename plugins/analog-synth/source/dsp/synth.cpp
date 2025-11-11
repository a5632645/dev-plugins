#include "synth.hpp"

namespace analogsynth {
void Synth::Init(float fs) noexcept {
    fs_ = fs;
    delay_.Init(fs);
    chorus_.Init(fs);
    reverb_.Init(fs);
}

void Synth::Reset() noexcept {
    delay_.Reset();
    chorus_.Reset();
    saturator_.Reset();
    reverb_.Reset();
    osc4_.Reset(0);
}

void Synth::ProcessBlock(float* left, float* right, size_t num_samples) noexcept {
    // -------------------- tick modulators --------------------
    // tick lfos
    Lfo::Parameter lfo_param;
    lfo_param.phase_inc = param_lfo1_freq.GetModCR() / fs_;
    lfo_param.shape = static_cast<Lfo::Shape>(param_lfo1_shape.Get().second);
    lfo1_.Process(lfo_param, num_samples);
    lfo_param.phase_inc = param_lfo2_freq.GetModCR() / fs_;
    lfo_param.shape = static_cast<Lfo::Shape>(param_lfo2_shape.Get().second);
    lfo2_.Process(lfo_param, num_samples);
    lfo_param.phase_inc = param_lfo3_freq.GetModCR() / fs_;
    lfo_param.shape = static_cast<Lfo::Shape>(param_lfo3_shape.Get().second);
    lfo3_.Process(lfo_param, num_samples);
    // tick envelopes
    qwqdsp::AdsrEnvelope::Parameter env_param;
    env_param.attack_ms = param_env_volume_attack.GetModCR();
    env_param.decay_ms = param_env_volume_decay.GetModCR();
    env_param.fs = fs_;
    env_param.release_ms = param_env_volume_release.GetModCR();
    env_param.sustain_level = param_env_volume_sustain.GetModCR();
    volume_env_.envelope_.Update(env_param);
    volume_env_.Process(param_env_volume_exp.Get(), num_samples);
    env_param.attack_ms = param_env_mod_attack.GetModCR();
    env_param.decay_ms = param_env_mod_decay.GetModCR();
    env_param.fs = fs_;
    env_param.release_ms = param_env_mod_release.GetModCR();
    env_param.sustain_level = param_env_mod_sustain.GetModCR();
    mod_env_.envelope_.Update(env_param);
    mod_env_.Process(param_env_mod_exp.Get(), num_samples);

    // -------------------- tick parameters --------------------
    modulation_matrix.Process(num_samples);
    
    // -------------------- tick oscillators --------------------
    // oscillator 1
    float freq1 = qwqdsp::convert::Pitch2Freq(current_note_ + param_osc1_detune.GetModCR());
    osc1_phase_inc_ = freq1 / fs_;
    osc1_pwm_ = param_osc1_pwm.GetModCR();
    float freq2 = qwqdsp::convert::Pitch2Freq(current_note_ + param_osc2_detune.GetModCR());
    osc2_.SetFreq(freq2, fs_);

    std::array<float, kBlockSize> osc_buffer;
    std::array<bool, kBlockSize> sync_buffer;
    std::array<float, kBlockSize> frac_sync_buffer;
    float osc_gain = param_osc1_vol.GetModCR();
    switch (param_osc1_shape.Get().second) {
        case 0:
            for (size_t i = 0; i < num_samples; ++i) {
                osc1_phase_ += osc1_phase_inc_;
                osc_buffer[i] = osc_gain * osc1_.Sawtooth(osc1_phase_, osc1_phase_inc_);
                sync_buffer[i] = osc1_phase_ > 1.0;
                osc1_phase_ -= std::floor(osc1_phase_);
                frac_sync_buffer[i] = osc1_phase_ / osc1_phase_inc_;
            }
            break;
        case 1:
            for (size_t i = 0; i < num_samples; ++i) {
                osc1_phase_ += osc1_phase_inc_;
                osc_buffer[i] = osc_gain * osc1_.Triangle(osc1_phase_, osc1_phase_inc_);
                sync_buffer[i] = osc1_phase_ > 1.0;
                osc1_phase_ -= std::floor(osc1_phase_);
                frac_sync_buffer[i] = osc1_phase_ / osc1_phase_inc_;
            }
            break;
        case 2:
            for (size_t i = 0; i < num_samples; ++i) {
                osc1_phase_ += osc1_phase_inc_;
                osc_buffer[i] = osc_gain * osc1_.PWM_Classic(osc1_phase_, osc1_phase_inc_, osc1_pwm_);
                sync_buffer[i] = osc1_phase_ > 1.0;
                osc1_phase_ -= std::floor(osc1_phase_);
                frac_sync_buffer[i] = osc1_phase_ / osc1_phase_inc_;
            }
            break;
        default:
            jassertfalse;
    }

    // oscillator 2
    osc_gain = param_osc2_vol.GetModCR();
    float osc_pwm = param_osc2_pwm.GetModCR();
    osc2_.SetPWM(osc_pwm);
    if (param_osc2_sync.Get()) {
        switch (param_osc2_shape.Get().second) {
            case 0:
                for (size_t i = 0; i < num_samples; ++i) {
                    osc_buffer[i] += osc_gain * osc2_.Sawtooth(sync_buffer[i], frac_sync_buffer[i]);
                }
                break;
            case 1:
                for (size_t i = 0; i < num_samples; ++i) {
                    osc_buffer[i] += osc_gain * osc2_.Triangle(sync_buffer[i], frac_sync_buffer[i]);
                }
                break;
            case 2:
                for (size_t i = 0; i < num_samples; ++i) {
                    osc_buffer[i] += osc_gain * osc2_.PWM(sync_buffer[i], frac_sync_buffer[i]);
                }
                break;
            case 3:
                for (size_t i = 0; i < num_samples; ++i) {
                    osc_buffer[i] += osc_gain * osc2_.Sine(sync_buffer[i], frac_sync_buffer[i]);
                }
                break;
            default:
                jassertfalse;
        }
    }
    else {
        switch (param_osc2_shape.Get().second) {
            case 0:
                for (size_t i = 0; i < num_samples; ++i) {
                    osc_buffer[i] += osc_gain * osc2_.Sawtooth();
                }
                break;
            case 1:
                for (size_t i = 0; i < num_samples; ++i) {
                    osc_buffer[i] += osc_gain * osc2_.Triangle();
                }
                break;
            case 2:
                for (size_t i = 0; i < num_samples; ++i) {
                    osc_buffer[i] += osc_gain * osc2_.PWM();
                }
                break;
            case 3:
                for (size_t i = 0; i < num_samples; ++i) {
                    osc_buffer[i] += osc_gain * osc2_.Sine();
                }
                break;
            default:
                jassertfalse;
        }
    }

    // oscillator 3
    float osc3_detune = param_osc3_detune.GetModCR();
    float osc3_unison_detune = param_osc3_unison_detune.GetModCR();
    int osc3_unison_num = static_cast<int>(param_osc3_unison.GetNoMod());
    std::array<float, kMaxUnison> phase_incs_;
    switch (param_osc3_uniso_type.Get().second) {
        case 0: {
            auto& unison_look_table = kPanTable[static_cast<size_t>(osc3_unison_num) - 1];
            for (int i = 0; i < osc3_unison_num; ++i) {
                float pitch = current_note_ + osc3_detune + osc3_unison_detune * unison_look_table[size_t(i)];
                phase_incs_[size_t(i)] = qwqdsp::convert::Pitch2Freq(pitch) / fs_;
            }
            break;
        }
        case 1: {
            auto& unison_look_table = osc3_freq_ratios_;
            for (int i = 0; i < osc3_unison_num; ++i) {
                float pitch = current_note_ + osc3_detune + osc3_unison_detune * unison_look_table[size_t(i)];
                phase_incs_[size_t(i)] = qwqdsp::convert::Pitch2Freq(pitch) / fs_;
            }
            break;
        }
        default:
            jassertfalse;
    }
    osc_gain = param_osc3_vol.GetModCR();
    osc_pwm = param_osc3_pwm.GetModCR();
    switch (param_osc3_shape.Get().second) {
        case 0:
            for (size_t i = 0; i < num_samples; ++i) {
                float sum{};
                for (int j = 0; j < osc3_unison_num; ++j) {
                    osc3_phases_[size_t(j)] += phase_incs_[size_t(j)];
                    osc3_phases_[size_t(j)] -= std::floor(osc3_phases_[size_t(j)]);
                    sum += osc_gain * osc1_.Sawtooth(osc3_phases_[size_t(j)], phase_incs_[size_t(j)]);
                }
                osc_buffer[i] += sum;
            }
            break;
        case 1:
            for (size_t i = 0; i < num_samples; ++i) {
                float sum{};
                for (int j = 0; j < osc3_unison_num; ++j) {
                    osc3_phases_[size_t(j)] += phase_incs_[size_t(j)];
                    osc3_phases_[size_t(j)] -= std::floor(osc3_phases_[size_t(j)]);
                    sum += osc_gain * osc1_.Triangle(osc3_phases_[size_t(j)], phase_incs_[size_t(j)]);
                }
                osc_buffer[i] += sum;
            }
            break;
        case 2:
            for (size_t i = 0; i < num_samples; ++i) {
                float sum{};
                for (int j = 0; j < osc3_unison_num; ++j) {
                    osc3_phases_[size_t(j)] += phase_incs_[size_t(j)];
                    osc3_phases_[size_t(j)] -= std::floor(osc3_phases_[size_t(j)]);
                    sum += osc_gain * osc1_.PWM_Classic(osc3_phases_[size_t(j)], phase_incs_[size_t(j)], osc_pwm);
                }
                osc_buffer[i] += sum;
            }
            break;
        default:
            jassertfalse;
    }

    // oscillator 4
    {
        float w0_detune = param_osc4_w0_detune.GetModCR();
        float pitch = w0_detune + current_note_;
        float w0_freq = qwqdsp::convert::Pitch2Freq(pitch);
        float w0 = qwqdsp::convert::Freq2W(w0_freq, fs_);
        osc4_.w0 = w0;
        float w_ratio = param_osc4_w_ratio.GetModCR();
        osc4_.w = w0 * w_ratio;
        osc4_.a = param_osc4_slope.GetModCR();
        osc4_.width = param_osc4_width.GetModCR() * std::numbers::pi_v<float> * 2;
        osc4_.n = static_cast<uint32_t>(param_osc4_n.GetModCR());
        osc4_.use_max_n = param_osc4_use_max_n.Get();
        float osc_vol = param_osc4_vol.GetModCR();
        osc4_.Update();
        switch (param_osc4_shape.Get().second) {
            case 0:
                for (size_t i = 0; i < num_samples; ++i) {
                    osc_buffer[i] += osc_vol * osc4_.Tick<false>();
                }
                break;
            case 1:
                for (size_t i = 0; i < num_samples; ++i) {
                    osc_buffer[i] += osc_vol * osc4_.Tick<true>();
                }
                break;
            default:
                jassertfalse;
                break;
        }
    }

    // -------------------- tick filter --------------------
    float const filter_Q = param_Q.GetModCR();
    float const filter_cutoff = param_cutoff_pitch.GetModCR();
    float const direct_mix = param_filter_direct.GetModCR();
    float const lp_mix = param_filter_lp.GetModCR();
    float const hp_mix = param_filter_hp.GetModCR();
    float const bp_mix = param_filter_bp.GetModCR();
    float freq = qwqdsp::convert::Pitch2Freq(filter_cutoff);
    float w = qwqdsp::convert::Freq2W(freq, fs_);
    tpt_svf_.SetCoeffQ(w, filter_Q);
    for (size_t i = 0; i < num_samples; ++i) {
        auto[hp,bp,lp] = tpt_svf_.TickMultiMode(osc_buffer[i]);
        osc_buffer[i] *= direct_mix;
        osc_buffer[i] += hp * hp_mix + bp * bp_mix + lp * lp_mix;
    }

    // -------------------- tick volume envelope --------------------
    auto const& volume_env_buffer = volume_env_.modulator_output;
    for (size_t i = 0; i < num_samples; ++i) {
        osc_buffer[i] *= volume_env_buffer[i];
    }

    // -------------------- tick effects --------------------
    std::array<qwqdsp::psimd::Float32x4, kBlockSize> fx_temp;
    for (size_t i = 0; i < num_samples; ++i) {
        fx_temp[i].x[0] = osc_buffer[i];
        fx_temp[i].x[1] = osc_buffer[i];
    }
    std::span fx_block{fx_temp.data(), num_samples};
    // delay
    if (param_delay_enable.Get()) {
        delay_.fs = fs_;
        delay_.delay_ms = param_delay_ms.GetModCR();
        delay_.feedback = param_delay_feedback.GetModCR();
        delay_.mix = param_delay_mix.GetModCR();
        delay_.pingpong = param_delay_pingpong.Get();
        delay_.lowcut_f = param_delay_lp.GetModCR();
        delay_.highcut_f = param_delay_hp.GetModCR();
        delay_.Process(fx_block);
    }
    // chorus
    if (param_chorus_enable.Get()) {
        chorus_.fs = fs_;
        chorus_.delay = param_chorus_delay.GetModCR();
        chorus_.depth = param_chorus_depth.GetModCR();
        chorus_.feedback = param_chorus_feedback.GetModCR();
        chorus_.mix = param_chorus_mix.GetModCR();
        chorus_.rate = param_chorus_rate.GetModCR();
        chorus_.Process(fx_block);
    }
    // distortion
    if (param_distortion_enable.Get()) {
        float db = param_distortion_drive.GetModCR();
        float gain = qwqdsp::convert::Db2Gain(db);
        for (auto& x : fx_block) {
            x *= qwqdsp::psimd::Float32x4::FromSingle(gain);
            x = saturator_.ADAA_MV_Compensation(x);
        }
    }

    // -------------------- output --------------------
    for (size_t i = 0; i < num_samples; ++i) {
        left[i] = fx_block[i].x[0];
        right[i] = fx_block[i].x[1];
    }
    // tick reverb
    if (param_reverb_enable.Get()) {
        reverb_.damping = param_reverb_damp.GetNoMod();
        reverb_.decay = param_reverb_decay.GetNoMod();
        reverb_.lowpass = param_reverb_lowpass.GetNoMod();
        reverb_.mix = param_reverb_mix.GetNoMod();
        reverb_.predelay = param_reverb_predelay.GetNoMod();
        reverb_.size = param_reverb_size.GetNoMod();
        reverb_.Process(left, right, num_samples);
    }
}
}