#include "synth.hpp"

namespace analogsynth {
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
    osc4_.Reset(0);
    phaser_.Reset();
}

void Synth::InitFxSection() {
    fx_chain_.clear();
    fx_chain_.push_back({"delay", [](Synth& s, std::span<SimdType> block) {
        if (s.param_delay_enable.Get()) {
            s.delay_.fs = s.fs_;
            s.delay_.delay_ms = s.param_delay_ms.GetModCR();
            s.delay_.feedback = s.param_delay_feedback.GetModCR();
            s.delay_.mix = s.param_delay_mix.GetModCR();
            s.delay_.pingpong = s.param_delay_pingpong.Get();
            s.delay_.lowcut_f = s.param_delay_lp.GetModCR();
            s.delay_.highcut_f = s.param_delay_hp.GetModCR();
            s.delay_.Process(block);
        }
    }});
    fx_chain_.push_back({"chorus", [](Synth& s, std::span<SimdType> block) {
        if (s.param_chorus_enable.Get()) {
            s.chorus_.fs = s.fs_;
            s.chorus_.delay = s.param_chorus_delay.GetModCR();
            s.chorus_.depth = s.param_chorus_depth.GetModCR();
            s.chorus_.feedback = s.param_chorus_feedback.GetModCR();
            s.chorus_.mix = s.param_chorus_mix.GetModCR();
            s.chorus_.rate = s.param_chorus_rate.GetModCR();
            s.chorus_.Process(block);
        }
    }});
    fx_chain_.push_back({"phaser", [](Synth& s, std::span<SimdType> block) {
        if (s.param_phaser_enable.Get()) {
            float center_pitch = s.param_phaser_center.GetModCR();
            float depth_pitch = s.param_phaser_depth.GetModCR();
            float begin = center_pitch - depth_pitch;
            float end = center_pitch + depth_pitch;
            begin = std::clamp(begin, kPitch20, kPitch20000);
            end = std::clamp(end, kPitch20, kPitch20000);
            begin = qwqdsp::convert::Pitch2Freq(begin);
            end = qwqdsp::convert::Pitch2Freq(end);
            s.phaser_.begin_w = qwqdsp::convert::Freq2W(begin, s.fs_);
            s.phaser_.end_w = qwqdsp::convert::Freq2W(end, s.fs_);
            s.phaser_.fs = s.fs_;
            s.phaser_.feedback = s.param_phase_feedback.GetModCR();
            s.phaser_.mix = s.param_phaser_mix.GetModCR();
            s.phaser_.Q = s.param_phaser_Q.GetModCR();
            s.phaser_.rate = s.param_phaser_rate.GetModCR();
            s.phaser_.stereo = s.param_phaser_stereo.GetModCR();
            s.phaser_.Process(block);
        }
    }});
    fx_chain_.push_back({"distortion", [](Synth& s, std::span<SimdType> block) {
        if (s.param_distortion_enable.Get()) {
            float db = s.param_distortion_drive.GetModCR();
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
        lfo1_.Reset(param_lfo1_freq.state_.GetSyncPhase());
    }
    if (param_lfo2_freq.state_.ShouldSync()) {
        lfo1_.Reset(param_lfo2_freq.state_.GetSyncPhase());
    }
    if (param_lfo3_freq.state_.ShouldSync()) {
        lfo1_.Reset(param_lfo3_freq.state_.GetSyncPhase());
    }
    if (param_chorus_rate.state_.ShouldSync()) {
        chorus_.SyncBpm(param_chorus_rate.state_.GetSyncPhase());
    }
    if (param_phaser_rate.state_.ShouldSync()) {
        phaser_.SyncBpm(param_phaser_rate.state_.GetSyncPhase());
    }
}

void Synth::ProcessBlock(float* left, float* right, size_t num_samples) noexcept {
    // -------------------- tick modulators --------------------
    // tick lfos
    Lfo::Parameter lfo_param;
    lfo_param.phase_inc = param_lfo1_freq.GetModCR() / fs_;
    lfo_param.shape = static_cast<Lfo::Shape>(param_lfo1_shape.Get());
    lfo1_.Process(lfo_param, num_samples);
    lfo_param.phase_inc = param_lfo2_freq.GetModCR() / fs_;
    lfo_param.shape = static_cast<Lfo::Shape>(param_lfo2_shape.Get());
    lfo2_.Process(lfo_param, num_samples);
    lfo_param.phase_inc = param_lfo3_freq.GetModCR() / fs_;
    lfo_param.shape = static_cast<Lfo::Shape>(param_lfo3_shape.Get());
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
    // tick marcos
    marco1_.Update();
    marco2_.Update();
    marco3_.Update();
    marco4_.Update();

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
    switch (param_osc1_shape.Get()) {
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
        switch (param_osc2_shape.Get()) {
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
        switch (param_osc2_shape.Get()) {
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
    switch (param_osc3_uniso_type.Get()) {
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
        switch (param_osc4_shape.Get()) {
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

    // noise
    {
        float vol = param_noise_vol.GetModCR();
        switch (param_noise_type.Get()) {
            case NoiseType_White:
                for (size_t i = 0; i < num_samples; ++i) {
                    osc_buffer[i] += vol * white_noise_.Next();
                }
                break;
            case NoiseType_Pink:
                for (size_t i = 0; i < num_samples; ++i) {
                    osc_buffer[i] += vol * pink_noise_.Next();
                }
                break;
            case NoiseType_Brown:
                for (size_t i = 0; i < num_samples; ++i) {
                    osc_buffer[i] += vol * brown_noise_.Next();
                }
                break;
            default:
                break;
        }
    }

    // -------------------- tick filter --------------------
    if (param_filter_enable.Get()) {
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
    for (auto& fx : fx_chain_) {
        fx.doing(*this, fx_block);
    }

    // -------------------- output --------------------
    for (size_t i = 0; i < num_samples; ++i) {
        left[i] = fx_block[i].x[0];
        right[i] = fx_block[i].x[1];
    }
    // // tick reverb
    // if (param_reverb_enable.Get()) {
    //     reverb_.damping = param_reverb_damp.GetNoMod();
    //     reverb_.decay = param_reverb_decay.GetNoMod();
    //     reverb_.lowpass = param_reverb_lowpass.GetNoMod();
    //     reverb_.mix = param_reverb_mix.GetNoMod();
    //     reverb_.predelay = param_reverb_predelay.GetNoMod();
    //     reverb_.size = param_reverb_size.GetNoMod();
    //     reverb_.Process(left, right, num_samples);
    // }
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