#pragma once
#include <vector>
#include <unordered_map>
#include <random>
#include <array>

#include <qwqdsp/osciilor/polyblep_sync.hpp>
#include <qwqdsp/osciilor/polyblep.hpp>
#include <qwqdsp/osciilor/noise.hpp>
#include <qwqdsp/convert.hpp>
#include <qwqdsp/simd_element/algebraic_waveshaper.hpp>
#include <qwqdsp/misc/smoother.hpp>
#include <juce_audio_processors/juce_audio_processors.h>

#include "imodulator.hpp"
#include "lfo.hpp"
#include "envelope.hpp"
#include "delay.hpp"
#include "chorus.hpp"
#include "reverb.hpp"
#include "osc4.hpp"
#include "phaser.hpp"
#include "constant.hpp"
#include "abstract_synth.hpp"
#include "wrap_parameters.hpp"
#include "filter.hpp"

namespace analogsynth {
using PanTable = std::array<float, kMaxUnison>;
static constexpr PanTable MakePanTable(int nvocice) {
    PanTable out{};
    if (nvocice == 1) {
        out[0] = 0.0f;
    }
    else {
        float interval = 2.0f / (static_cast<float>(nvocice) - 1);
        float begin = -1.0f;
        for (int i = 0; i < nvocice; ++i) {
            out[size_t(i)] = begin + static_cast<float>(i) * interval;
        }
    }
    return out;
}

static constexpr std::array<PanTable, kMaxUnison> kPanTable{
    MakePanTable(1),
    MakePanTable(2),
    MakePanTable(3),
    MakePanTable(4),
    MakePanTable(5),
    MakePanTable(6),
    MakePanTable(7),
    MakePanTable(8),
    MakePanTable(9),
    MakePanTable(10),
    MakePanTable(11),
    MakePanTable(12),
    MakePanTable(13),
    MakePanTable(14),
    MakePanTable(15),
    MakePanTable(16),
};

// -------------------- modulator --------------------
struct ModulationInfo {
    FloatParam* target{};
    IModulator* source{};

    bool enable{true};
    float amount{0.5f};
    bool bipolar{false};

    // only gui use
    std::function<void()> changed;

    float ConvertFrom01(float mod_val) const noexcept {
        float mod = mod_val;
        if (bipolar) {
            mod = mod * 2.0f - 1.0f;
        }
        return mod * amount;
    }
};

class ModulationMatrix {
public:
    static constexpr size_t kMaxModulations = 64;

    ModulationMatrix() {
        free_modulations_.reserve(kMaxModulations);
        doing_modulations_.reserve(kMaxModulations);
        parameters_.reserve(kMaxModulations);
        parameter_counter_.reserve(kMaxModulations);
        modulator_counter_.reserve(kMaxModulations);
        for (size_t i = 0; i < kMaxModulations; ++i) {
            free_modulations_.push_back(&modulations_[i]);
        }
    }

    void Process(size_t num_samples, size_t channel) noexcept {
        for (auto p : parameters_) {
            p->ClearModBuffer();
        }

        for (auto* m : doing_modulations_) {
            if (!m->enable) continue;

            float mod = m->source->modulator_output[channel][0];
            if (m->bipolar) {
                mod = mod * 2.0f - 1.0f;
            }
            m->target->buffer[channel] += mod * m->amount;
        }

        std::ignore = num_samples;
    }

    std::pair<ModulationInfo*, bool> Add(IModulator* source, FloatParam* target) {
        if (free_modulations_.empty()) {
            return {nullptr, false};
        }
        if (std::find_if(doing_modulations_.begin(), doing_modulations_.end(), [source,target](auto* it) {
            return it->source == source && it->target == target;
        }) != doing_modulations_.end()) {
            return {nullptr,false};
        }

        auto* alloc_modulation = free_modulations_.back();
        free_modulations_.pop_back();
        doing_modulations_.push_back(alloc_modulation);
        alloc_modulation->source = source;
        alloc_modulation->target = target;
        AddModInfoToModulator(alloc_modulation, source);
        AddModInfoToParameter(alloc_modulation, target);
        changed = true;
        return {alloc_modulation, true};
    }

    void Remove(IModulator* source, FloatParam* target) {
        auto exist_it3 = std::remove_if(doing_modulations_.begin(), doing_modulations_.end(), [source, target](auto m) {
            return m->source == source && m->target == target;
        });
        if (exist_it3 == doing_modulations_.end()) return;
        auto* pinfo = *exist_it3;
        doing_modulations_.erase(exist_it3, doing_modulations_.end());
        free_modulations_.push_back(pinfo);
        RemoveModInfoFromModulator(pinfo, source);
        RemoveModInfoFromParameter(pinfo, target);
        changed = true;
    }

    void ChangeSource(ModulationInfo* info, IModulator* new_source) {
        if (info->source == new_source || new_source == nullptr) return;
        RemoveModInfoFromModulator(info, info->source);
        info->source = new_source;
        AddModInfoToModulator(info, new_source);
        changed = true;
    }

    void ChangeTarget(ModulationInfo* info, FloatParam* new_target) {
        if (info->target == new_target || new_target == nullptr) return;
        RemoveModInfoFromParameter(info, info->target);
        info->target = new_target;
        AddModInfoToParameter(info, new_target);
        changed = true;
    }

    IModulator* FindSource(juce::StringRef name) {
        auto it = modulator_hash_table_.find(name);
        if (it == modulator_hash_table_.end()) {
            return nullptr;
        }
        return it->second;
    }

    FloatParam* FindTarget(juce::StringRef name) {
        auto it = parameter_hash_table_.find(name);
        if (it == parameter_hash_table_.end()) {
            return nullptr;
        }
        return it->second;
    }

    void Remove(ModulationInfo* info) {
        // Remove(info->source, info->target);
        auto exist_it3 = std::remove(doing_modulations_.begin(), doing_modulations_.end(), info);
        if (exist_it3 == doing_modulations_.end()) return;
        doing_modulations_.erase(exist_it3, doing_modulations_.end());
        free_modulations_.push_back(info);
        RemoveModInfoFromModulator(info, info->source);
        RemoveModInfoFromParameter(info, info->target);
        changed = true;
    }

    void RemoveAll() noexcept {
        doing_modulations_.clear();
        for (auto* p : parameters_) {
            p->ClearModBuffer();
        }
        parameters_.clear();
        parameter_counter_.clear();
        modulator_counter_.clear();
        free_modulations_.clear();
        for (size_t i = 0; i < kMaxModulations; ++i) {
            free_modulations_.push_back(&modulations_[i]);
        }
        changed = true;
    }

    juce::ValueTree SaveState() {
        juce::ValueTree r{"modulations"};
        for (auto* m : doing_modulations_) {
            r.appendChild(juce::ValueTree{
                "mod", {
                    {"source", m->source->name_},
                    {"target", m->target->name_},
                    {"amount", m->amount},
                    {"enable", m->enable},
                    {"bipolar", m->bipolar}
                }
            }, nullptr);
        }
        return r;
    }

    void LoadState(juce::ValueTree& tree) {
        auto r = tree.getChildWithName("modulations");
        RemoveAll();
        for (auto mod : r) {
            auto modulator_name = mod.getProperty("source").toString();
            auto parameter_name = mod.getProperty("target").toString();
            auto[ptr_mod, added] = AddModulationByName(modulator_name, parameter_name);
            if (ptr_mod != nullptr) {
                ptr_mod->amount = mod.getProperty("amount");
                ptr_mod->enable = mod.getProperty("enable");
                ptr_mod->bipolar = mod.getProperty("bipolar");
            }
        }
        changed = true;
    }

    std::pair<ModulationInfo*, bool> AddModulationByName(juce::StringRef source, juce::StringRef target) {
        auto modulator_pair = modulator_hash_table_.find(source);
        auto parameter_pair = parameter_hash_table_.find(target);
        if (modulator_pair == modulator_hash_table_.end()
            || parameter_pair == parameter_hash_table_.end()) {
            return {nullptr, false};
        }
        return Add(modulator_pair->second, parameter_pair->second);
    }

    void AddModulateParam(FloatParam& p) {
        parameter_hash_table_[p.name_] = &p;
    }

    void AddModulateModulator(IModulator& m) {
        modulator_hash_table_[m.name_] = &m;
    }

    std::vector<ModulationInfo*> const& GetDoingModulations() const {
        return doing_modulations_;
    }

    // const value won't change after synth constructor
    std::unordered_map<juce::String, FloatParam*> parameter_hash_table_;
    std::unordered_map<juce::String, IModulator*> modulator_hash_table_;
    // things
    std::atomic_bool changed{false};
private:
    std::unordered_map<FloatParam*, std::vector<ModulationInfo*>> parameter_counter_;
    std::unordered_map<IModulator*, std::vector<ModulationInfo*>> modulator_counter_;
    std::array<ModulationInfo, kMaxModulations> modulations_;
    std::vector<ModulationInfo*> free_modulations_;
    std::vector<ModulationInfo*> doing_modulations_;
    std::vector<FloatParam*> parameters_;

    void AddModInfoToModulator(ModulationInfo* info, IModulator* source) {
        modulator_counter_[source].push_back(info);
    }
    void RemoveModInfoFromModulator(ModulationInfo* info, IModulator* source) {
        auto it = modulator_counter_.find(source);
        if (it == modulator_counter_.end()) return;
        auto remove_it = std::remove(it->second.begin(), it->second.end(), info);
        it->second.erase(remove_it, it->second.end());
    }
    void AddModInfoToParameter(ModulationInfo* info, FloatParam* target) {
        auto it = parameter_counter_.find(target);
        if (it == parameter_counter_.end() || it->second.empty()) {
            parameters_.push_back(target);
        }
        parameter_counter_[target].push_back(info);
    }
    void RemoveModInfoFromParameter(ModulationInfo* info, FloatParam* target) {
        auto it = parameter_counter_.find(target);
        if (it == parameter_counter_.end()) return;
        auto remove_it = std::remove(it->second.begin(), it->second.end(), info);
        it->second.erase(remove_it, it->second.end());
        if (it->second.empty()) {
            auto remove_param_it = std::remove(parameters_.begin(), parameters_.end(), target);
            target->ClearModBuffer();
            parameters_.erase(remove_param_it, parameters_.end());
        }
    }

};

class Synth : public AbstractSynth<Synth> {
public:
    using SimdType = qwqdsp::psimd::Float32x4;

    Synth();

    void Init(float fs) noexcept;

    void Reset() noexcept;

    void SyncBpm(juce::AudioProcessor& p);

    void Process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi_buffer) {
        is_legato_ = param_legato.Get();
        gliding_factor_ = qwqdsp::misc::ExpSmoother::ComputeSmoothFactor(param_glide_time.GetNoMod(), fs_, 2.0f);
        size_t new_num_voices = static_cast<size_t>(param_num_voices.GetNoMod());
        if (new_num_voices != was_num_voices_) {
            was_num_voices_ = new_num_voices;
            SetNumVoices(new_num_voices);
            last_trigger_channel_ = 0;
        }

        size_t const num_samples = static_cast<size_t>(buffer.getNumSamples());
        float* left_ptr = buffer.getWritePointer(0);
        float* right_ptr = buffer.getWritePointer(1);

        int buffer_pos = 0;
        for (auto midi : midi_buffer) {
            ProcessSection(left_ptr + buffer_pos, right_ptr + buffer_pos, static_cast<size_t>(midi.samplePosition - buffer_pos));
            buffer_pos = midi.samplePosition;

            auto message = midi.getMessage();
            if (message.isNoteOn()) {
                NoteOn(message.getNoteNumber(), message.getFloatVelocity());
            }
            else if (message.isNoteOff(true)) {
                NoteOff(message.getNoteNumber());
            }
            else if (message.isAllNotesOff()) {
                AllNoteOff();
                last_trigger_channel_ = 0;
            }
        }

        ProcessSection(left_ptr + buffer_pos, right_ptr + buffer_pos, num_samples - static_cast<size_t>(buffer_pos));
    }

    // -------------------- effect chain --------------------
    std::atomic<bool> fx_order_changed{false};
    juce::ValueTree SaveFxChainState();
    void LoadFxChainState(juce::ValueTree& tree);
    std::vector<juce::String> GetCurrentFxChainNames() const;
    void MoveFxOrder(juce::StringRef name, int new_index);
    void MoveFxOrder(int old_index, int new_index);
    // -------------------- for modulations --------------------
    ModulationMatrix modulation_matrix;
    // -------------------- parameters --------------------
    // oscillator 1
    FloatParam param_osc1_detune{"osc1_detune",
         juce::NormalisableRange<float>{-24.0f, 24.0f, 0.1f},
         0.0f
    };
    FloatParam param_osc1_vol{"osc1_vol",
         juce::NormalisableRange<float>{0.0f, 1.0f, 0.01f},
         0.5f
    };
    ChoiceParam param_osc1_shape{"osc1_shape",
        juce::StringArray{"saw","tri","pwm"},
        "saw"
    };
    FloatParam param_osc1_pwm{"osc1_pwm",
         juce::NormalisableRange<float>{0.01f, 0.99f, 0.01f},
         0.5f
    };
    // oscillator 2
    BoolParam param_osc2_sync{"osc2_sync", false};
    FloatParam param_osc2_detune{"osc2_detune",
         juce::NormalisableRange<float>{-24.0f, 24.0f, 0.1f},
         0.0f
    };
    FloatParam param_osc2_vol{"osc2_vol",
         juce::NormalisableRange<float>{0.0f, 1.0f, 0.01f},
         0.0f
    };
    ChoiceParam param_osc2_shape{"osc2_shape",
        juce::StringArray{"saw","tri","pwm","sin"},
        "saw"
    };
    FloatParam param_osc2_pwm{"osc2_pwm",
         juce::NormalisableRange<float>{0.01f, 0.99f, 0.01f},
         0.5f
    };
    // oscillator 3
    FloatParam param_osc3_detune{"osc3_detune",
         juce::NormalisableRange<float>{-24.0f, 24.0f, 0.1f},
         0.0f
    };
    FloatParam param_osc3_vol{"osc3_vol",
         juce::NormalisableRange<float>{0.0f, 1.0f, 0.01f},
         0.0f
    };
    ChoiceParam param_osc3_shape{"osc3_shape",
        juce::StringArray{"saw","tri","pwm"},
        "saw"
    };
    FloatParam param_osc3_pwm{"osc3_pwm",
         juce::NormalisableRange<float>{0.01f, 0.99f, 0.01f},
         0.5f
    };
    FloatParam param_osc3_unison{"osc3.unison",
        juce::NormalisableRange<float>{1.0f, kMaxUnison, 1.0f},
        1.0f
    };
    FloatParam param_osc3_unison_detune{"osc3.unison_detune",
        juce::NormalisableRange<float>{0.0f, 2.0f, 0.01f, 0.4f},
        0.1f
    };
    ChoiceParam param_osc3_uniso_type{"osc3.unison_type",
        juce::StringArray{"uniform","random"},
        "uniform"
    };
    BoolParam param_osc3_retrigger{"osc3.retrigger", false};
    FloatParam param_osc3_phase{"osc3.phase",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.01f},
        0.0f
    };
    FloatParam param_osc3_phase_random{"osc3.phase_random",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.01f},
        0.0f
    };
    // oscillator 4
    FloatParam param_osc4_slope{"osc4.slope",
        juce::NormalisableRange<float>{0.1f,0.995f,0.001f},
        0.9f
    };
    FloatParam param_osc4_width{"osc4.width",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f},
        0.25f
    };
    FloatParam param_osc4_n{"osc4.n",
        juce::NormalisableRange<float>{1.0f, 4096.0f, 1.0f, 0.2f},
        5.0f
    };
    FloatParam param_osc4_w0_detune{"osc4.w0_detune",
        juce::NormalisableRange<float>{-24.0f,24.0f,0.1f},
        0.0f
    };
    FloatParam param_osc4_w_ratio{"osc4.w_ratio",
        juce::NormalisableRange<float>{0.1f, 12.0f, 0.01f},
        1.0f
    };
    BoolParam param_osc4_use_max_n{"osc4.use_max_n",false};
    ChoiceParam param_osc4_shape{"osc4.shape",
        juce::StringArray{"up","down"},
        "up"
    };
    FloatParam param_osc4_vol{"osc4.vol",
        juce::NormalisableRange<float>{0.0f,1.0f,0.01f},
        0.0f
    };
    // noise
    FloatParam param_noise_vol{"noise.vol",
        juce::NormalisableRange<float>{0.0f,1.0f,0.01f},
        0.0f
    };
    ChoiceParam param_noise_type{"noise.type",
        juce::StringArray{"white","pink","brown"},
        "pink"
    };
    enum {
        NoiseType_White = 0,
        NoiseType_Pink,
        NoiseType_Brown
    };
    // volume envelope
    FloatParam param_env_volume_attack{"vol_env_attack",
         juce::NormalisableRange<float>{0.0f, 10000.0f, 0.1f, 0.4f},
         1.0f
    };
    FloatParam param_env_volume_decay{"vol_env_decay",
         juce::NormalisableRange<float>{0.0f, 10000.0f, 1.0f, 0.4f},
         1000.0f
    };
    FloatParam param_env_volume_sustain{"vol_env_sustain",
         juce::NormalisableRange<float>{0.0f, 1.0f, 0.01f},
         1.0f
    };
    FloatParam param_env_volume_release{"vol_env_release",
         juce::NormalisableRange<float>{0.0f, 10000.0f, 1.0f, 0.4f},
         100.0f
    };
    BoolParam param_env_volume_exp{"vol_env.exp", true};
    // filter envelope
    FloatParam param_env_mod_attack{"mod_env_attack",
         juce::NormalisableRange<float>{0.0f, 10000.0f, 0.1f, 0.4f},
         1.0f
    };
    FloatParam param_env_mod_decay{",mod_env_decay",
         juce::NormalisableRange<float>{0.0f, 10000.0f, 1.0f, 0.4f},
         1000.0f
    };
    FloatParam param_env_mod_sustain{"mod_env_sustain",
         juce::NormalisableRange<float>{0.0f, 1.0f, 0.01f},
         1.0f
    };
    FloatParam param_env_mod_release{"mod_env_release",
         juce::NormalisableRange<float>{0.0f, 10000.0f, 1.0f, 0.4f},
         100.0f
    };
    BoolParam param_env_mod_exp{"mod_env.exp", false};
    // lfo
    BpmSyncFreqParam<false> param_lfo1_freq{"lfo1_freq",
        juce::NormalisableRange<float>{0.0f, 10.0f},
        0.0f,
        BpmSyncFreqParam<false>::LFOSyncType::Sync,
        "1/4"
    };
    ChoiceParam param_lfo1_shape{"lfo1_shape",
        juce::StringArray{
            "sin", "tri", "sawup", "sawdown", "noise", "smooth", "s&h"
        },
        "sin"
    };
    BoolParam param_lfo1_retrigger{"lfo1.retrriger", true};
    FloatParam param_lfo1_phase{"lfo1.phase",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.01f},
        0.0f
    };
    BpmSyncFreqParam<false> param_lfo2_freq{"lfo2_freq",
        juce::NormalisableRange<float>{0.0f, 10.0f},
        0.0f,
        BpmSyncFreqParam<false>::LFOSyncType::Sync,
        "1/4"
    };
    ChoiceParam param_lfo2_shape{"lfo2_shape",
        juce::StringArray{
            "sin", "tri", "sawup", "sawdown", "noise", "smooth", "s&h"
        },
        "sin"
    };
    BoolParam param_lfo2_retrigger{"lfo2.retrriger", true};
    FloatParam param_lfo2_phase{"lfo2.phase",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.01f},
        0.0f
    };
    BpmSyncFreqParam<false> param_lfo3_freq{"lfo3_freq",
        juce::NormalisableRange<float>{0.0f, 10.0f},
        0.0f,
        BpmSyncFreqParam<false>::LFOSyncType::Sync,
        "1/4"
    };
    ChoiceParam param_lfo3_shape{"lfo3_shape",
        juce::StringArray{
            "sin", "tri", "sawup", "sawdown", "noise", "smooth", "s&h"
        },
        "sin"
    };
    BoolParam param_lfo3_retrigger{"lfo3.retrriger", true};
    FloatParam param_lfo3_phase{"lfo3.phase",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.01f},
        0.0f
    };
    // fx-delay
    BoolParam param_delay_enable{"delay.enable", false};
    FloatParam param_delay_ms{"delay.delay_ms",
        juce::NormalisableRange<float>{0.0f, Delay::kMaxDelayMs, 0.1f, 0.4f},
        351.2f
    };
    BoolParam param_delay_pingpong{"delay.pingpong", false};
    FloatParam param_delay_feedback{"delay.feedback",
        juce::NormalisableRange<float>{-1.0f, 1.0f, 0.01f},
        0.6f
    };
    FloatParam param_delay_lp{"delay.lp",
        juce::NormalisableRange<float>{19.0f, 20001.0f, 1.0f, 0.4f},
        20001.0f
    };
    FloatParam param_delay_hp{"delay.hp",
        juce::NormalisableRange<float>{19.0f, 20001.0f, 1.0f, 0.4f},
        19.0f
    };
    FloatParam param_delay_mix{"delay.mix",
        juce::NormalisableRange<float>{0.0f,1.0f,0.01f},
        0.4f
    };
    // fx-chorus
    BoolParam param_chorus_enable{"chorus.enable", false};
    FloatParam param_chorus_delay{"delay.delay",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.01f},
        1.0f
    };
    FloatParam param_chorus_feedback{"chorus.feedback",
        juce::NormalisableRange<float>{-0.99f, 0.99f, 0.01f},
        0.4f
    };
    FloatParam param_chorus_mix{"chorus.mix",
        juce::NormalisableRange<float>{0.0f,1.0f,0.01f},
        0.6f
    };
    FloatParam param_chorus_depth{"chorus.depth",
        juce::NormalisableRange<float>{0.0f,1.0f,0.01f},
        0.3f
    };
    BpmSyncFreqParam<false> param_chorus_rate{"chorus.rate",
        juce::NormalisableRange<float>{0.0f, 5.0f, 0.01f, 0.4f},
        0.0f,
        BpmSyncFreqParam<false>::LFOSyncType::Sync,
        "4"
    };
    // fx-distortion
    BoolParam param_distortion_enable{"distortion.enable", false};
    FloatParam param_distortion_drive{"distortion.drive",
        juce::NormalisableRange<float>{-20.0f, 20.0f, 0.01f},
        0.0f
    };
    // fx-reverb
    BoolParam param_reverb_enable{"reverb.enable",false};
    FloatParam param_reverb_mix{"reverb.mix",
        juce::NormalisableRange<float>{0.0f,1.0f,0.01f},
        0.1f
    };
    FloatParam param_reverb_predelay{"reverb.predelay",
        juce::NormalisableRange<float>{0.0f,0.1f,0.0001f},
        0.0f
    };
    FloatParam param_reverb_lowpass{"reverb.lowpass",
        juce::NormalisableRange<float>{20.0f, 20000.0f, 1.0f, 0.4f},
        10000.0f
    };
    FloatParam param_reverb_decay{"reverb.decay",
        juce::NormalisableRange<float>{0.0f,1.0f,0.01f},
        0.6f
    };
    FloatParam param_reverb_size{"reverb.size",
        juce::NormalisableRange<float>{0.5f,2.0f,0.01f},
        1.3f
    };
    FloatParam param_reverb_damp{"reverb.damp",
        juce::NormalisableRange<float>{20.0f, 20000.0f, 1.0f, 0.4f},
        10000.0f
    };
    // fx-phaser
    BoolParam param_phaser_enable{"phaser.enable",false};
    FloatParam param_phaser_mix{"phaser.mix",
        juce::NormalisableRange<float>{0.0f,1.0f,0.01f},
        0.5f
    };
    FloatParam param_phaser_center{"phaser.center",
        juce::NormalisableRange<float>{kPitch20,kPitch20000,0.1f},
        85.0f
    };
    FloatParam param_phaser_depth{"phaser.depth",
        juce::NormalisableRange<float>{0.0f,kPitch20000-kPitch20,0.1f},
        10.0f
    };
    BpmSyncFreqParam<false> param_phaser_rate{"phaser.rate",
        juce::NormalisableRange<float>{0.0f,10.0f,0.1f},
        0.2f,
        BpmSyncFreqParam<false>::LFOSyncType::Sync,
        "1"
    };
    FloatParam param_phase_feedback{"phaser.feedback",
        juce::NormalisableRange<float>{-0.99f,0.99f,0.01f},
        0.4f
    };
    FloatParam param_phaser_Q{"phaser.Q",
        juce::NormalisableRange<float>{0.1f,10.0f,0.01f},
        1.0f / std::numbers::sqrt2_v<float>
    };
    FloatParam param_phaser_stereo{"phaser.stereo",
        juce::NormalisableRange<float>{0.0f,1.0f,0.01f},
        0.25f
    };
    // marcos
    FloatParam param_marco1{"marco1",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.0001f},
        0.0f
    };
    FloatParam param_marco2{"marco2",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.0001f},
        0.0f
    };
    FloatParam param_marco3{"marco3",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.0001f},
        0.0f
    };
    FloatParam param_marco4{"marco4",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.0001f},
        0.0f
    };
    // voices
    BoolParam param_legato{"legato", false};
    FloatParam param_glide_time{"glide_time",
        juce::NormalisableRange<float>{0.0f, 4000.0f, 0.1f, 0.4f},
        0.0f
    };
    FloatParam param_num_voices{"num_voices",
        juce::NormalisableRange<float>{1.0f, kMaxPoly, 1.0f},
        kMaxPoly
    };
    // filter section
    Filter filter_;

    // -------------------- implement for CVoice --------------------
    void StopChannel(uint32_t channel) noexcept {
        volume_env_.envelope_[channel].Noteoff(false);
        mod_env_.envelope_[channel].Noteoff(false);
    }

    uint32_t FindVoiceToSteal() noexcept {
        // find the min vol voice
        uint32_t min_vol_channel = 0;
        uint32_t min_vol_and_release_channel = kMaxPoly;
        float min_vol = std::numeric_limits<float>::infinity();
        float min_vol_and_release = std::numeric_limits<float>::infinity();
        for (auto channel : active_channels_) {
            float vol_output = volume_env_.envelope_[channel].GetLastOutput();
            if (vol_output < min_vol) {
                min_vol = vol_output;
                min_vol_channel = channel;
            }

            if (volume_env_.envelope_[channel].GetState() == qwqdsp::AdsrEnvelope::State::Release
                || volume_env_.envelope_[channel].GetState() == qwqdsp::AdsrEnvelope::State::Init) {
                if (vol_output < min_vol_and_release) {
                    min_vol_and_release = vol_output;
                    min_vol_and_release_channel = channel;
                }
            }
        }

        if (min_vol_and_release_channel == kMaxPoly) {
            min_vol_and_release_channel = min_vol_channel;
        }

        return min_vol_and_release_channel;
    }

    void StartNewChannel(uint32_t channel, int note, float velocity, bool retrigger, float glide_begin_pitch, float target_pitch) noexcept {
        juce::ignoreUnused(note, velocity);

        gliding_pitch_[channel] = glide_begin_pitch;
        target_pitch_[channel] = target_pitch;
        
        if (!retrigger) return;
        last_trigger_channel_ = channel;

        // envelopes
        volume_env_.envelope_[channel].NoteOn(false);
        mod_env_.envelope_[channel].NoteOn(false);

        // lfos
        if (param_lfo1_retrigger.Get()) {
            lfo1_.Reset(channel, param_lfo1_phase.GetNoMod());
        }
        if (param_lfo2_retrigger.Get()) {
            lfo2_.Reset(channel, param_lfo2_phase.GetNoMod());
        }
        if (param_lfo3_retrigger.Get()) {
            lfo3_.Reset(channel, param_lfo3_phase.GetNoMod());
        }

        // unison frequency and phase
        for (float& r : osc3_data_[channel].osc3_freq_ratios_) {
            r = unison_distribution_(random_generator_);
        }
        if (param_osc3_retrigger.Get()) {
            float begin_phase = param_osc3_phase.GetNoMod();
            float random_amount = param_osc3_phase_random.GetNoMod();
            for (float& r : osc3_data_[channel].osc3_phases_) {
                float p = begin_phase + random_amount * unison_distribution_(random_generator_);
                r = p - std::floor(p);
            }
        }
    }

    float GetCurrentGlidingPitch(uint32_t channel) noexcept {
        return gliding_pitch_[channel];
    }

    bool VoiceShouldRemove(uint32_t channel) noexcept {
        return volume_env_.envelope_[channel].GetState() == qwqdsp::AdsrEnvelope::State::Init;
    }
private:
    inline static const float kPitch20 = qwqdsp::convert::Freq2Pitch(20.0f);
    inline static const float kPitch20000 = qwqdsp::convert::Freq2Pitch(20000.0f);

    void AddModulateParam(FloatParam& p) {
        modulation_matrix.AddModulateParam(p);
    }

    void AddModulateModulator(IModulator& m) {
        modulation_matrix.AddModulateModulator(m);
    }

    // -------------------- processing blocks --------------------
    void ProcessAndAddBlock(size_t channel, float* left, float* right, size_t num_samples) noexcept;
    void ProcessSection(float* left, float* right, size_t num_samples) noexcept {
        while (num_samples != 0) {
            size_t cando = std::min<size_t>(num_samples, kBlockSize);
            std::fill_n(left, cando, 0.0f);
            std::fill_n(right, cando, 0.0f);

            // -------------------- tick oscillator and filter --------------------
            for (auto channel : active_channels_) {
                ProcessAndAddBlock(channel, left, right, cando);
            }

            // -------------------- tick effects --------------------
            std::array<qwqdsp::psimd::Float32x4, kBlockSize> fx_temp;
            for (size_t i = 0; i < cando; ++i) {
                fx_temp[i].x[0] = left[i];
                fx_temp[i].x[1] = left[i];
            }
            std::span fx_block{fx_temp.data(), cando};
            for (auto& fx : fx_chain_) {
                fx.doing(*this, fx_block);
            }
            for (size_t i = 0; i < cando; ++i) {
                left[i] = fx_block[i].x[0];
                right[i] = fx_block[i].x[1];
            }

            num_samples -= cando;
            left += cando;
            right += cando;
        }

        RemoveDeadChannels();
    }

    // polynomial management
    size_t last_trigger_channel_{};
    size_t was_num_voices_{kMaxPoly};
    float gliding_pitch_[kMaxPoly]{};
    float target_pitch_[kMaxPoly]{};
    float gliding_factor_{};

    // oscillator section
    using BlepCoeff = qwqdsp::oscillor::blep_coeff::BlackmanNutallApprox;
    float osc1_phase_[kMaxPoly]{};
    qwqdsp::oscillor::PolyBlep<BlepCoeff> osc1_;
    qwqdsp::oscillor::PolyBlepSync<BlepCoeff> osc2_[kMaxPoly];
    Osc4 osc4_[kMaxPoly];
    qwqdsp::oscillor::WhiteNoise white_noise_[kMaxPoly];
    qwqdsp::oscillor::PinkNoise pink_noise_[kMaxPoly];
    qwqdsp::oscillor::BrownNoise brown_noise_[kMaxPoly];
    
    struct Osc3Data {
        std::array<float, kMaxUnison> osc3_phases_{};
        std::array<float, kMaxUnison> osc3_freq_ratios_{};
    };
    Osc3Data osc3_data_[kMaxPoly];
    qwqdsp::oscillor::PolyBlep<BlepCoeff> osc3_;
    std::uniform_real_distribution<float> unison_distribution_{-1.0f, 1.0f};
    std::default_random_engine random_generator_;

    // effect section
    struct FxSection {
        juce::String name;
        void(*doing)(Synth& synth, std::span<SimdType> block);
    };
    std::vector<FxSection> fx_chain_;
    void InitFxSection();
    Delay delay_;
    Chorus chorus_;
    qwqdsp::simd_element::AlgebraicWaveshaperSimd<SimdType> saturator_;
    Reverb reverb_;
    Phaser phaser_;

    // modulator section
    Lfo lfo1_{"lfo1"};
    Lfo lfo2_{"lfo2"};
    Lfo lfo3_{"lfo3"};
    ModEnvelope volume_env_{"vol env"};
    ModEnvelope mod_env_{"mod env"};

    class MarcoModulator : public IModulator {
    public:
        MarcoModulator(juce::StringRef name, FloatParam& param) 
            : IModulator(name)
            , param_(param) {}
        
        void Update(size_t channel) noexcept {
            this->modulator_output[channel][0] = param_.GetNoMod();
        }
    private:
        FloatParam& param_;
    };
    MarcoModulator marco1_{"marco1", param_marco1};
    MarcoModulator marco2_{"marco2", param_marco2};
    MarcoModulator marco3_{"marco3", param_marco3};
    MarcoModulator marco4_{"marco4", param_marco4};
    
    float fs_{};
};
}