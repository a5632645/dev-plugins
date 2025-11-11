#pragma once
#include <vector>
#include <unordered_map>
#include <random>
#include <array>

#include <qwqdsp/osciilor/polyblep_sync.hpp>
#include <qwqdsp/osciilor/polyblep.hpp>
#include <qwqdsp/convert.hpp>
#include <qwqdsp/filter/svf_tpt.hpp>
#include <qwqdsp/algebraic_waveshaper.hpp>
#include <juce_audio_processors/juce_audio_processors.h>
#include <pluginshared/bpm_sync_lfo.hpp>

#include "imodulator.hpp"
#include "lfo.hpp"
#include "envelope.hpp"
#include "delay.hpp"
#include "chorus.hpp"
#include "reverb.hpp"

namespace analogsynth {
static constexpr int kMaxUnison = 16;
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

// -------------------- warpper of juce parameters --------------------
class FloatParam {
public:
    FloatParam(juce::StringRef name, juce::NormalisableRange<float> range, float default_value)
        : name_(name)
        , range_(range)
        , default_value_(default_value) {
    }

    std::unique_ptr<juce::AudioParameterFloat> Build() {
        jassert(ptr_ == nullptr);
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{name_, 1},
            name_,
            range_,
            default_value_
        );
        ptr_ = p.get();
        return p;
    }

    float GetNoMod() noexcept {
        return ptr_->get();
    }

    float GetModCR() noexcept {
        float normal_value = static_cast<juce::RangedAudioParameter*>(ptr_)->getValue();
        normal_value += buffer[0];
        normal_value = std::clamp(normal_value, 0.0f, 1.0f);
        return range_.convertFrom0to1(normal_value);
    }

    float GetModSR() noexcept {
        return GetModCR();
    }

    juce::AudioParameterFloat* ptr_{};
    float buffer[256]{};

    juce::String name_;
    juce::NormalisableRange<float> range_;
    float default_value_;
};

template<bool kNegPos>
class BpmSyncFreqParam {
public:
    using LFOSyncType = typename pluginshared::BpmSyncLFO<kNegPos>::LFOTempoType;

    BpmSyncFreqParam(
        juce::StringRef name,
        juce::NormalisableRange<float> range,
        float default_hz_value,
        LFOSyncType default_lfo_tempo_type,
        juce::StringRef default_lfo_tempo_speed)
        : freq_(name, range, default_hz_value)
        , default_lfo_tempo_type_(default_lfo_tempo_type)
        , default_lfo_tempo_(default_lfo_tempo_speed) {
    }

    std::unique_ptr<juce::AudioParameterFloat> Build1() {
        jassert(freq_.ptr_ == nullptr);
        auto p = freq_.Build();
        state_.param_lfo_hz_ = p.get();
        return p;
    }
    std::unique_ptr<juce::AudioParameterInt> Build2() {
        jassert(state_.param_lfo_tempo_type_ == nullptr);
        auto p = state_.MakeLfoTempoTypeParam(freq_.name_ + "_tt", default_lfo_tempo_type_);
        state_.param_lfo_tempo_type_ = p.get();
        return p;
    }
    std::unique_ptr<juce::AudioParameterChoice> Build3() {
        jassert(state_.param_tempo_speed_ == nullptr);
        auto p = state_.MakeLfoTempoSpeedParam(freq_.name_ + "ts", default_lfo_tempo_);
        state_.param_tempo_speed_ = p.get();
        return p;
    }

    float GetNoMod() noexcept {
        return state_.GetLfoFreq();
    }

    float GetModCR() noexcept {
        if (static_cast<LFOSyncType>(state_.param_lfo_tempo_type_->get()) == LFOSyncType::Free) {
            return freq_.GetModCR();
        }
        else {
            return state_.GetLfoFreq();
        }
    }

    FloatParam freq_;
    pluginshared::BpmSyncLFO<kNegPos> state_;
    LFOSyncType default_lfo_tempo_type_;
    juce::StringRef default_lfo_tempo_;
};

class ChoiceParam {
public:
    ChoiceParam(juce::StringRef name, juce::StringArray choice, juce::StringRef default_name)
        : name_(name)
        , choices_(choice)
        , default_value_(choice.indexOf(default_name)) {
        jassert(default_value_ != -1);
    }

    std::unique_ptr<juce::AudioParameterChoice> Build() {
        auto p = std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{name_, 1},
            name_,
            choices_,
            default_value_
        );
        ptr_ = p.get();
        return p;
    }

    std::pair<bool, int> Get() noexcept {
        return {true, ptr_->getIndex()};
    }

    juce::AudioParameterChoice* ptr_{};
private:
    juce::StringRef name_;
    juce::StringArray choices_;
    int default_value_;
};

class BoolParam {
public:
    BoolParam(juce::StringRef name, bool default_value)
        : name_(name)
        , default_value_(default_value) {}

    std::unique_ptr<juce::AudioParameterBool> Build() {
        auto p = std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{name_, 1},
            name_,
            default_value_
        );
        ptr_ = p.get();
        return p;
    }

    bool Get() noexcept {
        return ptr_->get();
    }

    juce::AudioParameterBool* ptr_{};
private:
    juce::StringRef name_;
    bool default_value_;
};

// -------------------- modulator --------------------
struct ModulationInfo {
    FloatParam* target{};
    IModulator* source{};

    bool enable{true};
    float amount{0.5f};
    bool bipolar{false};
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

    void Process(size_t num_samples) noexcept {
        for (auto p : parameters_) {
            p->buffer[0] = 0;
        }

        for (auto* m : doing_modulations_) {
            if (!m->enable) continue;

            float mod = m->source->modulator_output[0];
            if (m->bipolar) {
                mod = mod * 2.0f - 1.0f;
            }
            m->target->buffer[0] += mod * m->amount;
        }

        std::ignore = num_samples;
    }

    std::pair<ModulationInfo*, bool> Add(IModulator* source, FloatParam* target) {
        // auto& param_modulations = parameter_counter_[target];
        // auto exist_it = std::find_if(param_modulations.begin(), param_modulations.end(), [source](auto m) {
            // return m->source == source;
        // });
        // if (exist_it != param_modulations.end()) {
            // return {*exist_it, false};
        // }

        // if (param_modulations.empty()) {
        //     parameters_.push_back(target);
        // }

        // 无可用空间
        if (free_modulations_.empty()) {
            return {nullptr, false};
        }
        // 已经存在
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
        // param_modulations.push_back(alloc_modulation);
        // auto& modulator_modulations = modulator_counter_[source];
        // modulator_modulations.push_back(alloc_modulation);
        changed = true;
        return {alloc_modulation, true};
    }

    void Remove(IModulator* source, FloatParam* target) {
        // auto& param_modulations = parameter_counter_[target];
        // auto exist_it = std::remove_if(param_modulations.begin(), param_modulations.end(), [source](auto m) {
            // return m->source == source;
        // });
        // if (exist_it == param_modulations.end()) {
            // return;
        // }
        // param_modulations.erase(exist_it);

        // auto modulator_modulations = modulator_counter_[source];
        // auto exist_it2 = std::remove_if(modulator_modulations.begin(), modulator_modulations.end(), [source](auto m) {
            // return m->source == source;
        // });
        // modulator_modulations.erase(exist_it2);

        // remove from doing
        auto exist_it3 = std::remove_if(doing_modulations_.begin(), doing_modulations_.end(), [source, target](auto m) {
            return m->source == source && m->target == target;
        });
        if (exist_it3 == doing_modulations_.end()) return;
        auto* pinfo = *exist_it3;
        doing_modulations_.erase(exist_it3);
        free_modulations_.push_back(pinfo);
        RemoveModInfoFromModulator(pinfo, source);
        RemoveModInfoFromParameter(pinfo, target);

        // if (param_modulations.empty()) {
            // auto it4 = std::remove(parameters_.begin(), parameters_.end(), target);
            // target->buffer[0] = 0;
            // parameters_.erase(it4);
        // }
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
        doing_modulations_.erase(exist_it3);
        free_modulations_.push_back(info);
        RemoveModInfoFromModulator(info, info->source);
        RemoveModInfoFromParameter(info, info->target);
        changed = true;
    }

    void RemoveAll() noexcept {
        doing_modulations_.clear();
        for (auto* p : parameters_) {
            p->buffer[0] = 0;
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
        it->second.erase(remove_it);
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
        it->second.erase(remove_it);
        if (it->second.empty()) {
            auto remove_param_it = std::remove(parameters_.begin(), parameters_.end(), target);
            target->buffer[0] = 0;
            parameters_.erase(remove_param_it);
        }
    }

};

// -------------------- simple note queue --------------------
class NoteQueue {
public:
    struct Stroge {
        int note;
        float velocity;
    };

    NoteQueue() {
        notes_.reserve(128);
    }

    Stroge NoteOn(int note, float velocity) noexcept {
        notes_.push_back({note, velocity});
        return {note, velocity};
    }

    Stroge NoteOff(int note) noexcept {
        auto it = std::remove_if(notes_.begin(), notes_.end(), [note](auto const& t) {
            return t.note == note;
        });
        notes_.erase(it, notes_.end());
        
        if (notes_.empty()) {
            return {-1, 0.0f};
        }
        return notes_.back();
    }
private:
    std::vector<Stroge> notes_;
};

class Synth {
public:
    Synth() {
        AddModulateModulator(lfo1_);
        AddModulateModulator(lfo2_);
        AddModulateModulator(lfo3_);
        AddModulateModulator(mod_env_);
        AddModulateModulator(volume_env_);
        
        AddModulateParam(param_osc1_detune);
        AddModulateParam(param_osc1_vol);
        AddModulateParam(param_osc1_pwm);
        AddModulateParam(param_osc3_detune);
        AddModulateParam(param_osc3_vol);
        AddModulateParam(param_osc3_pwm);
        AddModulateParam(param_osc2_detune);
        AddModulateParam(param_osc2_vol);
        AddModulateParam(param_osc2_pwm);
        AddModulateParam(param_cutoff_pitch);
        AddModulateParam(param_Q);
        AddModulateParam(param_filter_direct);
        AddModulateParam(param_filter_lp);
        AddModulateParam(param_filter_hp);
        AddModulateParam(param_filter_bp);
        AddModulateParam(param_lfo1_freq.freq_);
        AddModulateParam(param_lfo2_freq.freq_);
        AddModulateParam(param_lfo3_freq.freq_);

        modulation_matrix.Add(&lfo1_, &param_cutoff_pitch);
    }

    void Init(float fs) noexcept;

    void Reset() noexcept;

    void NoteOn(int note, float velocity) {
        auto n = note_queue_.NoteOn(note, velocity);
        StartNewNote(n.note, n.velocity);
    }

    void NoteOff(int note) {
        auto n = note_queue_.NoteOff(note);
        if (n.note != -1) {
            StartNewNote(n.note, n.velocity);
        }
        else {
            StopNote();
        }
    }

    void SyncBpm(juce::AudioProcessor& p) {
        if (auto* head = p.getPlayHead()) {
            param_lfo1_freq.state_.SyncBpm(head->getPosition());
            param_lfo2_freq.state_.SyncBpm(head->getPosition());
            param_lfo3_freq.state_.SyncBpm(head->getPosition());
            param_chorus_rate.state_.SyncBpm(head->getPosition());
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
    }

    void Process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi_buffer) {
        size_t const num_samples = static_cast<size_t>(buffer.getNumSamples());
        float* left_ptr = buffer.getWritePointer(0);
        float* right_ptr = buffer.getWritePointer(1);

        int buffer_pos = 0;
        for (auto midi : midi_buffer) {
            auto message = midi.getMessage();
            if (message.isNoteOnOrOff()) {
                ProcessRaw(left_ptr + buffer_pos, right_ptr + buffer_pos, static_cast<size_t>(midi.samplePosition - buffer_pos));
                buffer_pos = midi.samplePosition;

                if (message.isNoteOn()) {
                    NoteOn(message.getNoteNumber(), message.getFloatVelocity());
                }
                else {
                    NoteOff(message.getNoteNumber());
                }
            }
        }

        ProcessRaw(left_ptr + buffer_pos, right_ptr + buffer_pos, num_samples - static_cast<size_t>(buffer_pos));
    }

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
    // oscillator 3
    FloatParam param_osc3_detune{"osc3_detune",
         juce::NormalisableRange<float>{-24.0f, 24.0f, 0.1f},
         0.0f
    };
    FloatParam param_osc3_vol{"osc3_vol",
         juce::NormalisableRange<float>{0.0f, 1.0f, 0.01f},
         0.5f
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
    // filter
    FloatParam param_cutoff_pitch{"filter_cutoff_pitch",
        juce::NormalisableRange<float>{qwqdsp::convert::Freq2Pitch(20.0f), qwqdsp::convert::Freq2Pitch(20000.0f), 0.1f},
        qwqdsp::convert::Freq2Pitch(1000.0f)
    };
    FloatParam param_Q{"filter_Q",
        juce::NormalisableRange<float>{0.1f, 10.0f, 0.01f},
        1.0f / std::numbers::sqrt2_v<float>
    };
    FloatParam param_filter_direct{"filter_direct",
        juce::NormalisableRange<float>{-1.0f,1.0f,0.01f},
        0.0f
    };
    FloatParam param_filter_lp{"filter_lp",
        juce::NormalisableRange<float>{-1.0f,1.0f,0.01f},
        1.0f
    };
    FloatParam param_filter_bp{"filter_bp",
        juce::NormalisableRange<float>{-1.0f,1.0f,0.01f},
        0.0f
    };
    FloatParam param_filter_hp{"filter_hp",
        juce::NormalisableRange<float>{-1.0f,1.0f,0.01f},
        0.0f
    };
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
        juce::NormalisableRange<float>{0.0f,2.0f,0.01f},
        1.3f
    };
    FloatParam param_reverb_damp{"reverb.damp",
        juce::NormalisableRange<float>{20.0f, 20000.0f, 1.0f, 0.4f},
        10000.0f
    };
private:
    void AddModulateParam(FloatParam& p) {
        modulation_matrix.AddModulateParam(p);
    }

    void AddModulateModulator(IModulator& m) {
        modulation_matrix.AddModulateModulator(m);
    }

    static constexpr size_t kBlockSize = 256;
    void StartNewNote(int note, float velocity) noexcept {
        current_note_ = static_cast<float>(note);
        current_velocity_ = velocity;

        // envelopes
        volume_env_.envelope_.NoteOn(false);
        mod_env_.envelope_.NoteOn(false);

        // lfos
        if (param_lfo1_retrigger.Get()) {
            lfo1_.Reset(param_lfo1_phase.GetNoMod());
        }
        if (param_lfo2_retrigger.Get()) {
            lfo2_.Reset(param_lfo2_phase.GetNoMod());
        }
        if (param_lfo3_retrigger.Get()) {
            lfo3_.Reset(param_lfo3_phase.GetNoMod());
        }

        // unison frequency and phase
        for (float& r : osc3_freq_ratios_) {
            r = unison_distribution_(random_generator_);
        }
        if (param_osc3_retrigger.Get()) {
            float begin_phase = param_osc3_phase.GetNoMod();
            float random_amount = param_osc3_phase_random.GetNoMod();
            for (float& r : osc3_phases_) {
                float p = begin_phase + random_amount * unison_distribution_(random_generator_);
                r = p - std::floor(p);
            }
        }
    }

    void StopNote() noexcept {
        volume_env_.envelope_.Noteoff(false);
        mod_env_.envelope_.Noteoff(false);
    }

    void ProcessRaw(float* left, float* right, size_t num_samples) noexcept {
        while (num_samples != 0) {
            size_t cando = std::min<size_t>(num_samples, kBlockSize);
            ProcessBlock(left, right, cando);
            num_samples -= cando;
            left += cando;
            right += cando;
        }
    }

    void ProcessBlock(float* left, float* right, size_t num_samples) noexcept;

    using BlepCoeff = qwqdsp::oscillor::blep_coeff::BlackmanNutallApprox;
    float osc1_phase_inc_{1e-7f};
    float osc1_phase_{};
    float osc1_pwm_{};
    qwqdsp::oscillor::PolyBlep<BlepCoeff> osc1_;
    qwqdsp::oscillor::PolyBlepSync<BlepCoeff> osc2_;
    
    std::array<float, kMaxUnison> osc3_phases_{};
    std::array<float, kMaxUnison> osc3_freq_ratios_{};
    qwqdsp::oscillor::PolyBlep<BlepCoeff> osc3_;
    std::uniform_real_distribution<float> unison_distribution_{-1.0f, 1.0f};
    std::default_random_engine random_generator_;

    qwqdsp::filter::SvfTPT tpt_svf_;

    Delay delay_;
    Chorus chorus_;
    qwqdsp::AlgebraicWaveshaperSimd<qwqdsp::psimd::Float32x4> saturator_;
    Reverb reverb_;

    Lfo lfo1_{"lfo1"};
    Lfo lfo2_{"lfo2"};
    Lfo lfo3_{"lfo3"};
    ModEnvelope volume_env_{"vol env"};
    ModEnvelope mod_env_{"mod env"};
    
    float fs_{};
    float current_note_{};
    float current_velocity_{};
    NoteQueue note_queue_;
};
}