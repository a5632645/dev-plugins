#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <bit>
#include <mutex>

namespace pluginshared {
class BpmSyncLFO {
public:
    static constexpr std::array kBaseRateTable{
        1.0f / 64.0f, 1.0f / 32.0f, 1.0f / 16.0f, 1.0f / 8.0f, 1.0f / 4.0f, 1.0f / 2.0f, 1.0f, 2.0f, 4.0f, 8.0f,
    };
    static constexpr std::array kBaseRateTextTable{
        "1/64", "1/32", "1/16", "1/8", "1/4", "1/2", "1", "2", "4", "8",
    };
    static constexpr size_t kRateTableSize = kBaseRateTable.size() * 3 * 2 + 1;
    static constexpr std::array kRateMulTable = [] {
        std::array<float, kRateTableSize> output_table{};
        // negative 0 positive
        // 1.5f 1.0f 0.66f
        size_t wpos = 0;
        for (auto it = kBaseRateTable.rbegin(); it != kBaseRateTable.rend(); ++it) {
            auto val = *it;
            output_table[wpos++] = -1.5f * val;
            output_table[wpos++] = -1.0f * val;
            output_table[wpos++] = -2.0f / 3.0f * val;
        }
        output_table[wpos++] = 0.0f;
        for (auto val : kBaseRateTable) {
            output_table[wpos++] = 2.0f / 3.0f * val;
            output_table[wpos++] = 1.0f * val;
            output_table[wpos++] = 1.5f * val;
        }
        return output_table;
    }();
    inline static std::array<juce::String, kRateTableSize> s_rate_name_table;
    inline static std::once_flag s_tempo_table_init_flag;

    static void TryInitTempoTable() {
        std::call_once(s_tempo_table_init_flag, [] {
            size_t wpos = 0;
            for (auto val : kBaseRateTextTable) {
                s_rate_name_table[wpos++] = juce::String{"-"} + val + "T";
                s_rate_name_table[wpos++] = juce::String{"-"} + val;
                s_rate_name_table[wpos++] = juce::String{"-"} + val + "D";
            }
            s_rate_name_table[wpos++] = "freeze";
            for (auto it = kBaseRateTextTable.rbegin(); it != kBaseRateTextTable.rend(); ++it) {
                auto val = *it;
                s_rate_name_table[wpos++] = juce::String{val} + "D";
                s_rate_name_table[wpos++] = juce::String{val};
                s_rate_name_table[wpos++] = juce::String{val} + "T";
            }
        });
    }

    static int GetTempoValueIndex(juce::StringRef rate_name) {
        if (rate_name.text[0] == '0' && rate_name.length() == 1) {
            return GetTempoValueIndex("freeze");
        }

        auto it = std::find(s_rate_name_table.begin(), s_rate_name_table.end(), rate_name);
        jassert(it != s_rate_name_table.end());
        int where = static_cast<int>(it - s_rate_name_table.begin());
        return where;
    }

    BpmSyncLFO() {
        TryInitTempoTable();
    }

    BpmSyncLFO(juce::String name, float min_freq, float max_freq, float interval, float warp, bool warpnp,
               juce::StringRef begin_tempo, juce::StringRef end_tempo, float default_hz, juce::StringRef default_tempo,
               bool default_is_free) {
        TryInitTempoTable();

        name_ = std::move(name);
        std::construct_at(&free_freq_range_, min_freq, max_freq, interval, warp, warpnp);
        tempo_begin_idx_ = GetTempoValueIndex(begin_tempo);
        tempo_end_idx_ = GetTempoValueIndex(end_tempo);
        default_free_hz_ = default_hz;
        default_tempo_idx_ = GetTempoValueIndex(default_tempo);
        default_is_free_ = default_is_free;
    }

    struct LfoInfo2 {
        float lfo_freq;
        float lfo_phase;
        bool sync_lfo;
    };
    [[nodiscard]] LfoInfo2 SyncBpm2(juce::AudioPlayHead* head) {
        float fbpm = 120.0f;
        float fppq = 0.0f;
        float lfo_phase = 0.0f;
        bool sync_lfo = false;
        if (head != nullptr) {
            auto pos = head->getPosition();
            if (auto bpm = pos->getBpm(); bpm) {
                fbpm = static_cast<float>(*bpm);
            }
            if (auto ppq = pos->getPpqPosition(); ppq) {
                fppq = static_cast<float>(*ppq);
                sync_lfo = true;
            }
            if (!pos->getIsPlaying()) {
                sync_lfo = false;
            }
        }

        float lfo_freq = 0.0f;
        FreqAttrubute freq_attr = GetFreqAttribute();
        if (!freq_attr.tempo_sync) {
            lfo_freq = free_freq_range_.convertFrom0to1(param_freq->get());
            sync_lfo = false;
        }
        else {
            if (!freq_attr.ppq_sync) {
                sync_lfo = false;
            }

            float findex =
                std::lerp(static_cast<float>(tempo_begin_idx_), static_cast<float>(tempo_end_idx_), param_freq->get());
            int index = static_cast<int>(findex);
            index = std::clamp(index, tempo_begin_idx_, tempo_end_idx_);

            float sync_rate = kRateMulTable[static_cast<size_t>(index)];
            if (!freq_attr.tempo_snap) {
                int nindex = std::min(index + 1, tempo_end_idx_);
                float next_rate = kRateMulTable[static_cast<size_t>(nindex)];
                sync_rate = std::lerp(sync_rate, next_rate, findex - static_cast<float>(index));
            }

            float sync_phase = sync_rate * fppq;
            sync_phase -= std::floor(sync_phase);
            lfo_phase = sync_phase;

            lfo_freq = sync_rate * fbpm / 60.0f;
        }

        if (should_reset_phase_.exchange(false, std::memory_order_acquire)) {
            lfo_phase = reseted_phase_;
            sync_lfo = true;
        }

        return LfoInfo2{lfo_freq, lfo_phase, sync_lfo};
    }

    [[nodiscard]]
    std::pair<std::unique_ptr<juce::AudioParameterInt>, std::unique_ptr<juce::AudioParameterFloat>> Build() {
        FreqAttrubute attr_init{};
        if (!default_is_free_) {
            attr_init.tempo_sync = -1;
            attr_init.tempo_snap = -1;
        }

        auto ptype = std::make_unique<juce::AudioParameterInt>(juce::ParameterID{name_ + "_type", 1}, name_ + "_type",
                                                               0, std::numeric_limits<int32_t>::max(),
                                                               std::bit_cast<int>(attr_init));
        param_type = ptype.get();

        auto attr =
            juce::AudioParameterFloatAttributes{}
                .withStringFromValueFunction([this, float_numeric = GetFloatNumericText(free_freq_range_.interval)](
                                                 auto x, auto) -> juce::String {
                    FreqAttrubute freq_attr = GetFreqAttribute();
                    if (freq_attr.tempo_sync) {
                        int index = static_cast<int>(
                            std::lerp(static_cast<float>(tempo_begin_idx_), static_cast<float>(tempo_end_idx_), x));
                        index = std::clamp(index, tempo_begin_idx_, tempo_end_idx_);
                        return s_rate_name_table[static_cast<size_t>(index)];
                    }
                    else {
                        return juce::String{free_freq_range_.convertFrom0to1(x), float_numeric};
                    }
                })
                .withValueFromStringFunction([this](juce::String x) -> float {
                    FreqAttrubute freq_attr = GetFreqAttribute();
                    if (freq_attr.tempo_sync) {
                        auto it = std::find(s_rate_name_table.begin(), s_rate_name_table.end(), x);
                        if (it == s_rate_name_table.end()) return 0.0f;

                        int where = static_cast<int>(it - s_rate_name_table.begin());
                        where = std::clamp(where, tempo_begin_idx_, tempo_end_idx_);
                        float val01 = static_cast<float>(where - tempo_begin_idx_)
                                    / static_cast<float>(tempo_end_idx_ - tempo_begin_idx_);
                        return val01;
                    }
                    else {
                        return free_freq_range_.convertTo0to1(x.getFloatValue());
                    }
                });
        auto pfreq = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{name_ + "_freq", 1}, name_ + "_freq", juce::NormalisableRange<float>{0, 1},
            default_is_free_ ? GetDefaultHzVal01() : GetDefaultTempoVal01(), attr);
        param_freq = pfreq.get();

        return {std::move(ptype), std::move(pfreq)};
    }

    [[nodiscard]]
    std::pair<std::unique_ptr<juce::AudioParameterInt>, std::unique_ptr<juce::AudioParameterFloat>> Build(
        juce::String name, float min_freq, float max_freq, float interval, float warp, bool warpnp,
        juce::StringRef begin_tempo, juce::StringRef end_tempo, float default_hz, juce::StringRef default_tempo,
        bool default_is_free) {
        name_ = std::move(name);
        std::construct_at(&free_freq_range_, min_freq, max_freq, interval, warp, warpnp);
        tempo_begin_idx_ = GetTempoValueIndex(begin_tempo);
        tempo_end_idx_ = GetTempoValueIndex(end_tempo);
        default_free_hz_ = default_hz;
        default_tempo_idx_ = GetTempoValueIndex(default_tempo);
        default_is_free_ = default_is_free;
        return Build();
    }

    struct FreqAttrubute {
        int tempo_sync : 1;
        int ppq_sync : 1;
        int tempo_snap : 1;
    };
    FreqAttrubute GetFreqAttribute() const {
        return std::bit_cast<FreqAttrubute>(param_type->get());
    }

    void SetFreqAttribute(FreqAttrubute attr) {
        param_type->setValueNotifyingHost(param_type->convertTo0to1(static_cast<float>(std::bit_cast<int>(attr))));
    }

    void ResetToDefaultHz() {
        param_freq->setValueNotifyingHost(free_freq_range_.convertTo0to1(default_free_hz_));
        FreqAttrubute attr = GetFreqAttribute();
        attr.tempo_sync = 0;
        SetFreqAttribute(attr);
    }

    void ResetToDefaultTempo() {
        float val01 = static_cast<float>(default_tempo_idx_ - tempo_begin_idx_)
                    / static_cast<float>(tempo_end_idx_ - tempo_begin_idx_);
        param_freq->setValueNotifyingHost(val01);
        FreqAttrubute attr = GetFreqAttribute();
        attr.tempo_sync = -1;
        SetFreqAttribute(attr);
    }

    float GetDefaultHzVal01() {
        return free_freq_range_.convertTo0to1(default_free_hz_);
    }

    float GetDefaultTempoVal01() {
        float val01 = static_cast<float>(default_tempo_idx_ - tempo_begin_idx_)
                    / static_cast<float>(tempo_end_idx_ - tempo_begin_idx_);
        return val01;
    }

    void TryResetPhase(float phase) {
        reseted_phase_ = phase;
        should_reset_phase_.store(true, std::memory_order_release);
    }

    juce::AudioParameterFloat* param_freq{};
    juce::AudioParameterInt* param_type{};
private:
    static int GetFloatNumericText(float interval) {
        float v = 1.0f;
        int num = 0;
        while (v > interval) {
            v /= 10.0f;
            ++num;
        }
        return num;
    }

    juce::String name_;
    juce::NormalisableRange<float> free_freq_range_;
    int tempo_begin_idx_;
    int tempo_end_idx_;
    float default_free_hz_;
    int default_tempo_idx_;
    bool default_is_free_;

    float reseted_phase_{};
    std::atomic<bool> should_reset_phase_{};
};
} // namespace pluginshared
