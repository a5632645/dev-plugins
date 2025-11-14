#include "synth.hpp"

namespace analogsynth {

static_assert(CVoice<Synth>, "Error CRTP");

Synth::Synth() {
    AddModulateModulator(lfo1_);
    AddModulateModulator(lfo2_);
    AddModulateModulator(lfo3_);
    AddModulateModulator(mod_env_);
    AddModulateModulator(volume_env_);
    AddModulateModulator(marco1_);
    AddModulateModulator(marco2_);
    AddModulateModulator(marco3_);
    AddModulateModulator(marco4_);
    
    AddModulateParam(param_osc1_detune);
    AddModulateParam(param_osc1_vol);
    AddModulateParam(param_osc1_pwm);
    AddModulateParam(param_osc2_detune);
    AddModulateParam(param_osc2_vol);
    AddModulateParam(param_osc2_pwm);
    AddModulateParam(param_osc3_detune);
    AddModulateParam(param_osc3_vol);
    AddModulateParam(param_osc3_pwm);
    AddModulateParam(param_osc4_slope);
    AddModulateParam(param_osc4_width);
    AddModulateParam(param_osc4_n);
    AddModulateParam(param_osc4_w0_detune);
    AddModulateParam(param_osc4_w_ratio);
    AddModulateParam(param_osc4_vol);
    AddModulateParam(param_noise_vol);
    AddModulateParam(param_cutoff_pitch);
    AddModulateParam(param_Q);
    AddModulateParam(param_filter_direct);
    AddModulateParam(param_filter_lp);
    AddModulateParam(param_filter_hp);
    AddModulateParam(param_filter_bp);
    AddModulateParam(param_lfo1_freq.freq_);
    AddModulateParam(param_lfo2_freq.freq_);
    AddModulateParam(param_lfo3_freq.freq_);
    AddModulateParam(param_chorus_delay);
    AddModulateParam(param_chorus_feedback);
    AddModulateParam(param_chorus_mix);
    AddModulateParam(param_chorus_depth);
    AddModulateParam(param_chorus_rate.freq_);
    AddModulateParam(param_distortion_drive);
    AddModulateParam(param_reverb_mix);
    AddModulateParam(param_reverb_predelay);
    AddModulateParam(param_reverb_lowpass);
    AddModulateParam(param_reverb_decay);
    AddModulateParam(param_reverb_size);
    AddModulateParam(param_reverb_damp);
    AddModulateParam(param_phaser_mix);
    AddModulateParam(param_phaser_center);
    AddModulateParam(param_phaser_depth);
    AddModulateParam(param_phaser_rate.freq_);
    AddModulateParam(param_phase_feedback);
    AddModulateParam(param_phaser_Q);
    AddModulateParam(param_phaser_stereo);

    InitFxSection();

    modulation_matrix.Add(&lfo1_, &param_cutoff_pitch);
}

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
    for (auto& x : osc4_) {
        x.Reset(0);
    }
    phaser_.Reset();
}

void Synth::InitFxSection() {
    fx_chain_.clear();
    fx_chain_.push_back({"delay", [](Synth& s, std::span<SimdType> block) {
        if (s.param_delay_enable.Get()) {
            s.delay_.fs = s.fs_;
            s.delay_.delay_ms = s.param_delay_ms.GetModCR(s.last_trigger_channel_);
            s.delay_.feedback = s.param_delay_feedback.GetModCR(s.last_trigger_channel_);
            s.delay_.mix = s.param_delay_mix.GetModCR(s.last_trigger_channel_);
            s.delay_.pingpong = s.param_delay_pingpong.Get();
            s.delay_.lowcut_f = s.param_delay_lp.GetModCR(s.last_trigger_channel_);
            s.delay_.highcut_f = s.param_delay_hp.GetModCR(s.last_trigger_channel_);
            s.delay_.Process(block);
        }
    }});
    fx_chain_.push_back({"chorus", [](Synth& s, std::span<SimdType> block) {
        if (s.param_chorus_enable.Get()) {
            s.chorus_.fs = s.fs_;
            s.chorus_.delay = s.param_chorus_delay.GetModCR(s.last_trigger_channel_);
            s.chorus_.depth = s.param_chorus_depth.GetModCR(s.last_trigger_channel_);
            s.chorus_.feedback = s.param_chorus_feedback.GetModCR(s.last_trigger_channel_);
            s.chorus_.mix = s.param_chorus_mix.GetModCR(s.last_trigger_channel_);
            s.chorus_.rate = s.param_chorus_rate.GetModCR(s.last_trigger_channel_);
            s.chorus_.Process(block);
        }
    }});
    fx_chain_.push_back({"phaser", [](Synth& s, std::span<SimdType> block) {
        if (s.param_phaser_enable.Get()) {
            float center_pitch = s.param_phaser_center.GetModCR(s.last_trigger_channel_);
            float depth_pitch = s.param_phaser_depth.GetModCR(s.last_trigger_channel_);
            float begin = center_pitch - depth_pitch;
            float end = center_pitch + depth_pitch;
            begin = std::clamp(begin, kPitch20, kPitch20000);
            end = std::clamp(end, kPitch20, kPitch20000);
            begin = qwqdsp::convert::Pitch2Freq(begin);
            end = qwqdsp::convert::Pitch2Freq(end);
            s.phaser_.begin_w = qwqdsp::convert::Freq2W(begin, s.fs_);
            s.phaser_.end_w = qwqdsp::convert::Freq2W(end, s.fs_);
            s.phaser_.fs = s.fs_;
            s.phaser_.feedback = s.param_phase_feedback.GetModCR(s.last_trigger_channel_);
            s.phaser_.mix = s.param_phaser_mix.GetModCR(s.last_trigger_channel_);
            s.phaser_.Q = s.param_phaser_Q.GetModCR(s.last_trigger_channel_);
            s.phaser_.rate = s.param_phaser_rate.GetModCR(s.last_trigger_channel_);
            s.phaser_.stereo = s.param_phaser_stereo.GetModCR(s.last_trigger_channel_);
            s.phaser_.Process(block);
        }
    }});
    fx_chain_.push_back({"distortion", [](Synth& s, std::span<SimdType> block) {
        if (s.param_distortion_enable.Get()) {
            float db = s.param_distortion_drive.GetModCR(s.last_trigger_channel_);
            float gain = qwqdsp::convert::Db2Gain(db);
            for (auto& x : block) {
                x *= qwqdsp::psimd::Float32x4::FromSingle(gain);
                x = s.saturator_.ADAA_MV_Compensation(x);
            }
        }
    }});
    fx_chain_.push_back({"reverb", [](Synth& s, std::span<SimdType> block) {
        // tick reverb
        if (s.param_reverb_enable.Get()) {
            s.reverb_.damping = s.param_reverb_damp.GetNoMod();
            s.reverb_.decay = s.param_reverb_decay.GetNoMod();
            s.reverb_.lowpass = s.param_reverb_lowpass.GetNoMod();
            s.reverb_.mix = s.param_reverb_mix.GetNoMod();
            s.reverb_.predelay = s.param_reverb_predelay.GetNoMod();
            s.reverb_.size = s.param_reverb_size.GetNoMod();
            s.reverb_.Process(block);
        }
    }});
}

void Synth::SyncBpm(juce::AudioProcessor& p) {
    if (auto* head = p.getPlayHead()) {
        param_lfo1_freq.state_.SyncBpm(head->getPosition());
        param_lfo2_freq.state_.SyncBpm(head->getPosition());
        param_lfo3_freq.state_.SyncBpm(head->getPosition());
        param_chorus_rate.state_.SyncBpm(head->getPosition());
        param_phaser_rate.state_.SyncBpm(head->getPosition());
    }
    if (param_lfo1_freq.state_.ShouldSync()) {
        lfo1_.ResetAll(param_lfo1_freq.state_.GetSyncPhase());
    }
    if (param_lfo2_freq.state_.ShouldSync()) {
        lfo1_.ResetAll(param_lfo2_freq.state_.GetSyncPhase());
    }
    if (param_lfo3_freq.state_.ShouldSync()) {
        lfo1_.ResetAll(param_lfo3_freq.state_.GetSyncPhase());
    }
    if (param_chorus_rate.state_.ShouldSync()) {
        chorus_.SyncBpm(param_chorus_rate.state_.GetSyncPhase());
    }
    if (param_phaser_rate.state_.ShouldSync()) {
        phaser_.SyncBpm(param_phaser_rate.state_.GetSyncPhase());
    }
}

void Synth::ProcessAndAddBlock(size_t channel, float* left, float* right, size_t num_samples) noexcept {
    juce::ignoreUnused(right);
    // -------------------- tick modulators --------------------
    // tick lfos
    Lfo::Parameter lfo_param;
    lfo_param.phase_inc = param_lfo1_freq.GetModCR(channel) / fs_;
    lfo_param.shape = static_cast<Lfo::Shape>(param_lfo1_shape.Get());
    lfo1_.Process(lfo_param, num_samples, channel);
    lfo_param.phase_inc = param_lfo2_freq.GetModCR(channel) / fs_;
    lfo_param.shape = static_cast<Lfo::Shape>(param_lfo2_shape.Get());
    lfo2_.Process(lfo_param, num_samples, channel);
    lfo_param.phase_inc = param_lfo3_freq.GetModCR(channel) / fs_;
    lfo_param.shape = static_cast<Lfo::Shape>(param_lfo3_shape.Get());
    lfo3_.Process(lfo_param, num_samples, channel);
    // tick envelopes
    qwqdsp::AdsrEnvelope::Parameter env_param;
    env_param.attack_ms = param_env_volume_attack.GetModCR(channel);
    env_param.decay_ms = param_env_volume_decay.GetModCR(channel);
    env_param.fs = fs_;
    env_param.release_ms = param_env_volume_release.GetModCR(channel);
    env_param.sustain_level = param_env_volume_sustain.GetModCR(channel);
    volume_env_.envelope_[channel].Update(env_param);
    volume_env_.Process(channel, param_env_volume_exp.Get(), num_samples);
    env_param.attack_ms = param_env_mod_attack.GetModCR(channel);
    env_param.decay_ms = param_env_mod_decay.GetModCR(channel);
    env_param.fs = fs_;
    env_param.release_ms = param_env_mod_release.GetModCR(channel);
    env_param.sustain_level = param_env_mod_sustain.GetModCR(channel);
    mod_env_.envelope_[channel].Update(env_param);
    mod_env_.Process(channel, param_env_mod_exp.Get(), num_samples);
    // tick marcos
    marco1_.Update(channel);
    marco2_.Update(channel);
    marco3_.Update(channel);
    marco4_.Update(channel);

    // -------------------- tick parameters --------------------
    modulation_matrix.Process(num_samples, channel);
    
    // -------------------- tick oscillators --------------------
    float pitch_buffer[kBlockSize];
    for (size_t i = 0; i < num_samples; ++i) {
        gliding_pitch_[channel] = target_pitch_[channel] + gliding_factor_ * (gliding_pitch_[channel] - target_pitch_[channel]);
        pitch_buffer[i] = gliding_pitch_[channel];
    }
    // oscillator 1
    float freq1 = qwqdsp::convert::Pitch2Freq(pitch_buffer[0] + param_osc1_detune.GetModCR(channel));
    float osc1_phase_inc = freq1 / fs_;
    float osc1_pwm = param_osc1_pwm.GetModCR(channel);
    float freq2 = qwqdsp::convert::Pitch2Freq(pitch_buffer[0] + param_osc2_detune.GetModCR(channel));
    osc2_[channel].SetFreq(freq2, fs_);

    std::array<float, kBlockSize> osc_buffer;
    std::array<bool, kBlockSize> sync_buffer;
    std::array<float, kBlockSize> frac_sync_buffer;
    float osc_gain = param_osc1_vol.GetModCR(channel);
    switch (param_osc1_shape.Get()) {
        case 0:
            for (size_t i = 0; i < num_samples; ++i) {
                osc1_phase_[channel] += osc1_phase_inc;
                osc_buffer[i] = osc_gain * osc1_.Sawtooth(osc1_phase_[channel], osc1_phase_inc);
                sync_buffer[i] = osc1_phase_[channel] > 1.0;
                osc1_phase_[channel] -= std::floor(osc1_phase_[channel]);
                frac_sync_buffer[i] = osc1_phase_[channel] / osc1_phase_inc;
            }
            break;
        case 1:
            for (size_t i = 0; i < num_samples; ++i) {
                osc1_phase_[channel] += osc1_phase_inc;
                osc_buffer[i] = osc_gain * osc1_.Triangle(osc1_phase_[channel], osc1_phase_inc);
                sync_buffer[i] = osc1_phase_[channel] > 1.0;
                osc1_phase_[channel] -= std::floor(osc1_phase_[channel]);
                frac_sync_buffer[i] = osc1_phase_[channel] / osc1_phase_inc;
            }
            break;
        case 2:
            for (size_t i = 0; i < num_samples; ++i) {
                osc1_phase_[channel] += osc1_phase_inc;
                osc_buffer[i] = osc_gain * osc1_.PWM_Classic(osc1_phase_[channel], osc1_phase_inc, osc1_pwm);
                sync_buffer[i] = osc1_phase_[channel] > 1.0;
                osc1_phase_[channel] -= std::floor(osc1_phase_[channel]);
                frac_sync_buffer[i] = osc1_phase_[channel] / osc1_phase_inc;
            }
            break;
        default:
            jassertfalse;
    }

    // oscillator 2
    osc_gain = param_osc2_vol.GetModCR(channel);
    float osc_pwm = param_osc2_pwm.GetModCR(channel);
    osc2_[channel].SetPWM(osc_pwm);
    if (param_osc2_sync.Get()) {
        switch (param_osc2_shape.Get()) {
            case 0:
                for (size_t i = 0; i < num_samples; ++i) {
                    osc_buffer[i] += osc_gain * osc2_[channel].Sawtooth(sync_buffer[i], frac_sync_buffer[i]);
                }
                break;
            case 1:
                for (size_t i = 0; i < num_samples; ++i) {
                    osc_buffer[i] += osc_gain * osc2_[channel].Triangle(sync_buffer[i], frac_sync_buffer[i]);
                }
                break;
            case 2:
                for (size_t i = 0; i < num_samples; ++i) {
                    osc_buffer[i] += osc_gain * osc2_[channel].PWM(sync_buffer[i], frac_sync_buffer[i]);
                }
                break;
            case 3:
                for (size_t i = 0; i < num_samples; ++i) {
                    osc_buffer[i] += osc_gain * osc2_[channel].Sine(sync_buffer[i], frac_sync_buffer[i]);
                }
                break;
            default:
                jassertfalse;
        }
    }
    else {
        switch (param_osc2_shape.Get()) {
            case 0:
                for (size_t i = 0; i < num_samples; ++i) {
                    osc_buffer[i] += osc_gain * osc2_[channel].Sawtooth();
                }
                break;
            case 1:
                for (size_t i = 0; i < num_samples; ++i) {
                    osc_buffer[i] += osc_gain * osc2_[channel].Triangle();
                }
                break;
            case 2:
                for (size_t i = 0; i < num_samples; ++i) {
                    osc_buffer[i] += osc_gain * osc2_[channel].PWM();
                }
                break;
            case 3:
                for (size_t i = 0; i < num_samples; ++i) {
                    osc_buffer[i] += osc_gain * osc2_[channel].Sine();
                }
                break;
            default:
                jassertfalse;
        }
    }

    // oscillator 3
    float osc3_detune = param_osc3_detune.GetModCR(channel);
    float osc3_unison_detune = param_osc3_unison_detune.GetModCR(channel);
    int osc3_unison_num = static_cast<int>(param_osc3_unison.GetNoMod());
    std::array<float, kMaxUnison> phase_incs_;
    switch (param_osc3_uniso_type.Get()) {
        case 0: {
            auto& unison_look_table = kPanTable[static_cast<size_t>(osc3_unison_num) - 1];
            for (int i = 0; i < osc3_unison_num; ++i) {
                float pitch = pitch_buffer[0] + osc3_detune + osc3_unison_detune * unison_look_table[size_t(i)];
                phase_incs_[size_t(i)] = qwqdsp::convert::Pitch2Freq(pitch) / fs_;
            }
            break;
        }
        case 1: {
            auto& unison_look_table = osc3_data_[channel].osc3_freq_ratios_;
            for (int i = 0; i < osc3_unison_num; ++i) {
                float pitch = pitch_buffer[0] + osc3_detune + osc3_unison_detune * unison_look_table[size_t(i)];
                phase_incs_[size_t(i)] = qwqdsp::convert::Pitch2Freq(pitch) / fs_;
            }
            break;
        }
        default:
            jassertfalse;
    }
    osc_gain = param_osc3_vol.GetModCR(channel);
    osc_pwm = param_osc3_pwm.GetModCR(channel);
    auto& osc3_phases_ = osc3_data_[channel].osc3_phases_;
    switch (param_osc3_shape.Get()) {
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
        float w0_detune = param_osc4_w0_detune.GetModCR(channel);
        float pitch = w0_detune + pitch_buffer[0];
        float w0_freq = qwqdsp::convert::Pitch2Freq(pitch);
        float w0 = qwqdsp::convert::Freq2W(w0_freq, fs_);
        osc4_[channel].w0 = w0;
        float w_ratio = param_osc4_w_ratio.GetModCR(channel);
        osc4_[channel].w = w0 * w_ratio;
        osc4_[channel].a = param_osc4_slope.GetModCR(channel);
        osc4_[channel].width = param_osc4_width.GetModCR(channel) * std::numbers::pi_v<float> * 2;
        osc4_[channel].n = static_cast<uint32_t>(param_osc4_n.GetModCR(channel));
        osc4_[channel].use_max_n = param_osc4_use_max_n.Get();
        float osc_vol = param_osc4_vol.GetModCR(channel);
        osc4_[channel].Update();
        switch (param_osc4_shape.Get()) {
            case 0:
                for (size_t i = 0; i < num_samples; ++i) {
                    osc_buffer[i] += osc_vol * osc4_[channel].Tick<false>();
                }
                break;
            case 1:
                for (size_t i = 0; i < num_samples; ++i) {
                    osc_buffer[i] += osc_vol * osc4_[channel].Tick<true>();
                }
                break;
            default:
                jassertfalse;
                break;
        }
    }

    // noise
    {
        float vol = param_noise_vol.GetModCR(channel);
        switch (param_noise_type.Get()) {
            case NoiseType_White:
                for (size_t i = 0; i < num_samples; ++i) {
                    osc_buffer[i] += vol * white_noise_[channel].Next();
                }
                break;
            case NoiseType_Pink:
                for (size_t i = 0; i < num_samples; ++i) {
                    osc_buffer[i] += vol * pink_noise_[channel].Next();
                }
                break;
            case NoiseType_Brown:
                for (size_t i = 0; i < num_samples; ++i) {
                    osc_buffer[i] += vol * brown_noise_[channel].Next();
                }
                break;
            default:
                break;
        }
    }

    // -------------------- tick filter --------------------
    if (param_filter_enable.Get()) {
        float const filter_Q = param_Q.GetModCR(channel);
        float const filter_cutoff = param_cutoff_pitch.GetModCR(channel);
        float const direct_mix = param_filter_direct.GetModCR(channel);
        float const lp_mix = param_filter_lp.GetModCR(channel);
        float const hp_mix = param_filter_hp.GetModCR(channel);
        float const bp_mix = param_filter_bp.GetModCR(channel);
        float freq = qwqdsp::convert::Pitch2Freq(filter_cutoff);
        float w = qwqdsp::convert::Freq2W(freq, fs_);
        tpt_svf_[channel].SetCoeffQ(w, filter_Q);
        for (size_t i = 0; i < num_samples; ++i) {
            auto[hp,bp,lp] = tpt_svf_[channel].TickMultiMode(osc_buffer[i]);
            osc_buffer[i] *= direct_mix;
            osc_buffer[i] += hp * hp_mix + bp * bp_mix + lp * lp_mix;
        }
    }

    // -------------------- tick volume envelope --------------------
    auto const& volume_env_buffer = volume_env_.modulator_output;
    for (size_t i = 0; i < num_samples; ++i) {
        left[i] += osc_buffer[i] * volume_env_buffer[channel][i];
    }
}

juce::ValueTree Synth::SaveFxChainState() {
    juce::ValueTree r{"fx_order"};
    for (auto& fx : fx_chain_) {
        r.appendChild(juce::ValueTree{
            "fx", {
                {"name", fx.name}
            }
        }, nullptr);
    }
    return r;
}

void Synth::LoadFxChainState(juce::ValueTree& tree) {
    auto r = tree.getChildWithName("fx_order");
    if (!r.isValid()) {
        InitFxSection();
        fx_order_changed = true;
        return;
    }

    std::vector<FxSection> new_section;
    new_section.reserve(fx_chain_.size());

    for (auto const& fx : r) {
        auto fx_name = fx.getProperty("name").toString();
        auto it = std::find_if(fx_chain_.begin(), fx_chain_.end(), [fx_name](auto const& fx_it) {
            return fx_it.name == fx_name;
        });
        if (it != fx_chain_.end()) {
            new_section.push_back({fx_name, it->doing});
            fx_chain_.erase(it);
        }
    }
    for (auto const& fx : fx_chain_) {
        new_section.push_back(fx);
    }
    fx_chain_.swap(new_section);
    fx_order_changed = true;
}

std::vector<juce::String> Synth::GetCurrentFxChainNames() const {
    std::vector<juce::String> r;
    for (auto const& fx : fx_chain_) {
        r.push_back(fx.name);
    }
    return r;
}

void Synth::MoveFxOrder(juce::StringRef name, int new_index) {
    if (new_index < 0 || new_index >= static_cast<int>(fx_chain_.size())) return;

    auto it = std::find_if(fx_chain_.begin(), fx_chain_.end(), [name](auto const& fx_it) {
        return fx_it.name == name;
    });
    if (it == fx_chain_.end()) return;

    int current_index = static_cast<int>(std::distance(fx_chain_.begin(), it));
    if (current_index == new_index) {
        return;
    }
    
    if (current_index < new_index) {
        auto new_pos = fx_chain_.begin() + new_index + 1;
        std::rotate(it, it + 1, new_pos);
    }
    else {
        auto start_pos = fx_chain_.begin() + new_index;
        std::rotate(start_pos, it, it + 1);
    }
}

void Synth::MoveFxOrder(int old_index, int new_index) {
    if (new_index < 0 || new_index >= static_cast<int>(fx_chain_.size())) return;

    int current_index = old_index;
    if (current_index == new_index) {
        return;
    }
    
    auto it = fx_chain_.begin() + old_index;
    if (current_index < new_index) {
        auto new_pos = fx_chain_.begin() + new_index + 1;
        std::rotate(it, it + 1, new_pos);
    }
    else {
        auto start_pos = fx_chain_.begin() + new_index;
        std::rotate(start_pos, it, it + 1);
    }
}
}