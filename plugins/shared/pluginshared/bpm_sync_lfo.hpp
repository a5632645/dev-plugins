#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace pluginshared {
template<bool NegPos>
class BpmSyncLFO {
public:
    enum class LFOTempoType {
        Free = 0,
        Sync,
        SyncDot,
        SyncTri,
        NumTypes
    };

    inline static juce::StringArray const kTempoStrings{
        "freeze", "32",  "16",  "8",    "4",    "2",   "1",
        "1/2",    "1/4", "1/8", "1/16", "1/32", "1/64"};
    static constexpr std::array const kTempoMuls{
        0.0f,        1.0f / 128.0f, 1.0f / 64.0f, 1.0f / 32.0f, 1.0f / 16.0f,
        1.0f / 8.0f, 1.0f / 4.0f,   1.0f / 2.0f,  1.0f,         2.0f,
        4.0f,        8.0f,          16.0f};

    inline static juce::StringArray const kNegPosTempoStrings{
        "-1/64", "-1/32", "-1/16", "-1/8",   "-1/4", "-1/2", "-1",  "-2", "-4",
        "-8",    "-16",   "-32",   "freeze", "32",   "16",   "8",   "4",  "2",
        "1",     "1/2",   "1/4",   "1/8",    "1/16", "1/32", "1/64"};
    static constexpr std::array const kNegPosTempoMuls{
        -16.0f,        -8.0f,         -4.0f,         -2.0f,
        -1.0f,         -1.0f / 2.0f,  -1.0f / 4.0f,  -1.0f / 8.0f,
        -1.0f / 16.0f, -1.0f / 32.0f, -1.0f / 64.0f, -1.0f / 128.0f,
        0.0f,          1.0f / 128.0f, 1.0f / 64.0f,  1.0f / 32.0f,
        1.0f / 16.0f,  1.0f / 8.0f,   1.0f / 4.0f,   1.0f / 2.0f,
        1.0f,          2.0f,          4.0f,          8.0f,
        16.0f};

    inline static const juce::String kTempoSpeedPrefix{"ts"};
    inline static const juce::String kTempoTypePrefix{"tt"};
    inline static const juce::String kHzPrefix{"hz"};

    static juce::String GetTempoSpeedParamId(juce::StringRef name) {
        return kTempoSpeedPrefix + name;
    }
    static juce::String GetTempoTypeParamId(juce::StringRef name) {
        return kTempoTypePrefix + name;
    }
    static juce::String GetHzParamId(juce::StringRef name) {
        return kHzPrefix + name;
    }

    static std::unique_ptr<juce::AudioParameterChoice> MakeLfoTempoSpeedParam(
        juce::StringRef name, juce::StringRef default_speed
    ) {
        if constexpr (NegPos) {
            int const default_idx = kNegPosTempoStrings.indexOf(default_speed);
            jassert(default_idx != -1);
            return std::make_unique<juce::AudioParameterChoice>(
                juce::ParameterID{name, 1},
                name,
                kNegPosTempoStrings,
                default_idx
            );
        }
        else {
            int const default_idx = kTempoStrings.indexOf(default_speed);
            jassert(default_idx != -1);
            return std::make_unique<juce::AudioParameterChoice>(
                juce::ParameterID{name, 1},
                name,
                kTempoStrings,
                default_idx
            );
        }
    }

    static std::unique_ptr<juce::AudioParameterInt> MakeLfoTempoTypeParam(
        juce::StringRef name, LFOTempoType default_idx
    ) {
        return std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{name, 1},
            name,
            0, static_cast<int>(LFOTempoType::NumTypes) - 1,
            static_cast<int>(default_idx)
        );
    }

    static std::unique_ptr<juce::AudioParameterFloat> MakeLfoHzParam(
        juce::StringRef name,
        float min, float max, float step,
        bool symitri, float default_value
    ) {
        return std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{name, 1},
            name,
            juce::NormalisableRange<float>{min, max, step, 0.4f, symitri},
            default_value
        );
    }

    void SyncBpm(juce::Optional<juce::AudioPlayHead::PositionInfo> pos) {
        float fbpm = default_bpm_;
        float fppq = 0.0f;
        bool sync_lfo = false;
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
        should_sync_ = sync_lfo;

        LFOTempoType tempo_type = static_cast<LFOTempoType>(param_lfo_tempo_type_->get());
        if (tempo_type == LFOTempoType::Free) {
            lfo_freq_ = param_lfo_hz_->get();
            should_sync_ = false;
        }
        else {
            float sync_rate{};
            if constexpr (NegPos) {
                sync_rate = kNegPosTempoMuls[static_cast<size_t>(param_tempo_speed_->getIndex())];
            }
            else {
                sync_rate = kTempoMuls[static_cast<size_t>(param_tempo_speed_->getIndex())];
            }
            if (tempo_type == LFOTempoType::SyncDot) {
                sync_rate *= 2.0f / 3.0f;
            }
            else if (tempo_type == LFOTempoType::SyncTri) {
                sync_rate *= 3.0f / 2.0f;
            }
            
            if (sync_lfo) {
                float sync_phase = sync_rate * fppq;
                sync_phase -= std::floor(sync_phase);
                sync_phase_ = sync_phase;
            }

            float const lfo_freq = sync_rate * fbpm / 60.0f;
            lfo_freq_ = lfo_freq;
        }
    }

    /**
     * @return [0,1]
     */
    float GetSyncPhase() const noexcept {
        return sync_phase_;
    }

    bool ShouldSync() const noexcept {
        return should_sync_;
    }

    /**
     * @return hz
     */
    float GetLfoFreq() const noexcept {
        return lfo_freq_;
    }

    void SetTempoTypeToFree() {
        param_lfo_tempo_type_->setValueNotifyingHost(param_lfo_tempo_type_->convertTo0to1(static_cast<float>(LFOTempoType::Free)));
    }
    
    float default_bpm_{120.0f};
    juce::AudioParameterChoice* param_tempo_speed_{};
    juce::AudioParameterInt* param_lfo_tempo_type_{};
    juce::AudioParameterFloat* param_lfo_hz_{};
private:
    bool should_sync_{};
    float sync_phase_{};
    float lfo_freq_{};
};
}
