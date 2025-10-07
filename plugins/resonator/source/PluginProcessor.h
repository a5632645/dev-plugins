#pragma once
#include "../../shared/juce_param_listener.hpp"
#include <complex>

#include "qwqdsp/convert.hpp"
#include "qwqdsp/filter/one_pole.hpp"
#include "ThrianAllpass.hpp"

static constexpr size_t kNumResonators = 8;

// ---------------------------------------- damp one pole low shelfs ----------------------------------------
class ParalleOnepole {
public:
    void Tick(std::array<float, kNumResonators>& x) noexcept {
        for (size_t i = 0; i < kNumResonators; ++i) {
            x[i] = damp_[i].Tick(x[i]);
        }
    }

    void Reset() noexcept {
        for (auto& f : damp_) {
            f.Reset();
        }
    }

    void SetFrequency(const std::array<float, kNumResonators>& omega) noexcept {
        for (size_t i = 0; i < kNumResonators; ++i) {
            if (omega[i] < std::numbers::pi_v<float>) {
                damp_[i].SetLPF(omega[i]);
            }
            else {
                damp_[i].MakePass();
            }
        }
    }
private:
    std::array<qwqdsp::filter::OnePoleFilter, kNumResonators> damp_;
};

// ---------------------------------------- dispersion allpass ----------------------------------------
class ParalleDispersion {
public:
    void Tick(std::array<float, kNumResonators>& x) noexcept {
        for (size_t i = 0; i < kNumResonators; ++i) {
            x[i] = dispersions_[i].Process(x[i]);
        }
    }

    void Reset() noexcept {
        for (auto& f : dispersions_) {
            f.Panic();
        }
    }

    std::array<float, kNumResonators> SetFilter(
        const std::array<float, kNumResonators>& delay,
        const std::array<float, kNumResonators>& omega
    ) noexcept {
        for (size_t i = 0; i < kNumResonators; ++i) {
            dispersions_[i].SetGroupDelay(delay[i]);
        }

        std::array<float, kNumResonators> delays;
        for (size_t i = 0; i < kNumResonators; ++i) {
            delays[i] = dispersions_[i].GetPhaseDelay(omega[i]);
        }
        return delays;
    }

    float SetSingleFilter(
        size_t idx,
        float delay,
        float omega
    ) noexcept {
        dispersions_[idx].SetGroupDelay(delay);
        return dispersions_[idx].GetPhaseDelay(omega);
    }
private:
    std::array<mana::ThrianAllpass, kNumResonators> dispersions_;
};

// ---------------------------------------- delays ----------------------------------------
class ParalleDelay {
public:
    void Init(float fs, float min_pitch) {
        float const min_frequency = qwqdsp::convert::Pitch2Freq(min_pitch);
        float const max_seconds = 1.0f / min_frequency;
        size_t const max_samples = static_cast<size_t>(std::ceil(max_seconds * fs));

        size_t a = 1;
        while (a < max_samples) {
            a *= 2;
        }
        for (auto& v : buffer_) {
            v.resize(a);
        }
        mask_ = a - 1;
    }

    void Reset() noexcept {
        wpos_ = 0;
        for (auto& v : buffer_) {
            std::fill(v.begin(), v.end(), 0);
        }
    }

    void Tick(
        std::array<float, kNumResonators>& x,
        const std::array<float, kNumResonators>& delays
    ) noexcept {
        for (size_t i = 0; i < kNumResonators; ++i) {
            buffer_[i][wpos_] = x[i];

            size_t rpos = wpos_ + mask_ + 1 - delays[i];
            rpos &= mask_;
            x[i] = buffer_[i][rpos];
        }

        ++wpos_;
        wpos_ &= mask_;
    }
private:
    std::array<std::vector<float>, kNumResonators> buffer_;
    size_t wpos_{};
    size_t mask_{};
};

// ---------------------------------------- scatter matrix ----------------------------------------
class ScatterMatrix {
public:
    static constexpr size_t kNumReflections = 7;

    void Tick(
        std::array<float, kNumResonators>& x,
        const std::array<float, kNumReflections>& reflections
    ) noexcept {
        SingleScatter(x[0], x[1], reflections[0]);
        SingleScatter(x[2], x[3], reflections[1]);
        SingleScatter(x[4], x[5], reflections[2]);
        SingleScatter(x[6], x[7], reflections[3]);
        SingleScatter(x[1], x[2], reflections[4]);
        SingleScatter(x[5], x[6], reflections[5]);
        SingleScatter(x[2], x[5], reflections[6]);
    }
private:
    // 只能用正交矩阵?
    static constexpr void SingleScatter(float& a, float& b, float k) {
        float const cosk = std::cos(k);
        float const sink = std::sin(k);
        float const outa = cosk * a - sink * b;
        float const outb = sink * a + cosk * b;
        a = outa;
        b = outb;
        // float const outa = (1 + k) * a - k * b;
        // float const outb = a * k + (1 - k) * b;
        // a = outa;
        // b = outb;
    }
};

struct Voice
{
    int voiceID;   // 复音的唯一 ID (0 到 kNumResonators - 1)
    int midiNote;  // 当前分配到的 MIDI 音高 (0-127)
    bool isActive; // 是否正在被使用 (NoteOn 状态)
    // 其他您需要的参数 (例如：音量、频率、内部滤波器状态等)
};

class PolyphonyManager {
public:
    PolyphonyManager() {
        initializeVoices();
    }

    /**
     * @brief 处理 NoteOn 事件，分配或替换复音。
     * @param note 要分配的 MIDI 音高。
     * @return 分配到的复音的 ID，如果找不到则返回 -1 (理论上不会发生)。
     */
    int noteOn(int note, bool round_robin) {
        if (round_robin) {
            // 测试循环位
            if (!voices[round_robin_].isActive) {
                int id = activateVoice(round_robin_, note);
                ++round_robin_;
                round_robin_ &= (kNumResonators - 1);
                return id;
            }
        }

        // 尝试找到一个空闲的复音
        for (size_t id = 0; id < kNumResonators; ++id) {
            if (!voices[id].isActive) {
                // 找到空闲复音：直接分配
                return activateVoice(id, note);
            }
        }

        // --- 所有复音都被占用：FIFO 替换最旧的 ---
        
        // 获取最旧的复音 ID (deque 的头部)
        int oldestVoiceID = activationOrder.front();
        activationOrder.pop_front();

        // 激活新音高并返回 ID
        return activateVoice(oldestVoiceID, note);
    }

    /**
     * @brief 处理 NoteOff 事件，释放对应的复音。
     * @param note 要释放的 MIDI 音高。
     * @return 返回被关闭的id，如果没有则是kNumResonators
     */
    int noteOff(int note) {
        // 找到所有分配给该音高的复音并释放
        for (size_t id = 0; id < kNumResonators; ++id) {
            if (voices[id].isActive && voices[id].midiNote == note) {
                deactivateVoice(id);
                // 注意：如果有多个复音分配到同一个音高，这里只会释放找到的第一个。
                // 如果需要支持单音高多复音，则可以继续循环或使用更复杂的映射。
                return id;
            }
        }
        return kNumResonators;
    }
    
    // -----------------------------------------------------------
    // 调试/查询方法
    // -----------------------------------------------------------
    const Voice& getVoice(int id) const {
        return voices[id];
    }

    void initializeVoices() {
        for (size_t i = 0; i < kNumResonators; ++i) {
            voices[i].voiceID = i;
            voices[i].midiNote = -1;
            voices[i].isActive = false;
        }
        round_robin_ = 0;
    }

private:
    std::array<Voice, kNumResonators> voices;           // 存储所有复音实例
    std::deque<int> activationOrder;     // 存储正在使用复音的 ID，用于 FIFO 替换
    size_t round_robin_{};

    int activateVoice(int id, int note) {
        voices[id].midiNote = note;
        voices[id].isActive = true;
        
        // 将此复音 ID 添加到激活队列的尾部，表示它现在是“最新”的
        activationOrder.push_back(id);
        return id;
    }

    void deactivateVoice(int id) {
        voices[id].isActive = false;
        voices[id].midiNote = -1;

        // 从激活队列中移除此 ID，因为它不再是“激活”状态
        // 使用 std::remove_if + erase idiom
        activationOrder.erase(
            std::remove(activationOrder.begin(), activationOrder.end(), id),
            activationOrder.end()
        );
    }
};

// ---------------------------------------- juce processor ----------------------------------------
class ResonatorAudioProcessor final : public juce::AudioProcessor
{
public:
    //==============================================================================
    ResonatorAudioProcessor();
    ~ResonatorAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    JuceParamListener param_listener_;
    std::unique_ptr<juce::AudioProcessorValueTreeState> value_tree_;

    ParalleOnepole damp_;
    ParalleDelay delay_;
    ParalleDispersion dispersion_;
    ScatterMatrix matrix_;
    std::array<float, kNumResonators> fb_values_{};
    std::array<juce::AudioParameterFloat*, kNumResonators> pitches_{};
    std::array<juce::AudioParameterFloat*, kNumResonators> fine_tune_{};
    std::array<juce::AudioParameterFloat*, kNumResonators> damp_pitch_{};
    std::array<juce::AudioParameterFloat*, kNumResonators> dispersion_pole_radius_{};
    std::array<juce::AudioParameterFloat*, kNumResonators> decays_{};
    std::array<juce::AudioParameterBool*, kNumResonators> polarity_{};
    std::array<juce::AudioParameterFloat*, kNumResonators> mix_volume_{};
    std::array<juce::AudioParameterFloat*, ScatterMatrix::kNumReflections> matrix_reflections_{};

    juce::AudioParameterBool* midi_drive_{};
    juce::AudioParameterBool* allow_round_robin_{};
    juce::AudioParameterFloat* global_pitch_{};
    juce::AudioParameterFloat* global_damp_{};
    juce::AudioParameterFloat* dry_mix_{};

    bool was_midi_drive_{false};
    PolyphonyManager note_manager_;

    float dry_volume_{};
    std::array<float, kNumResonators> mix_{};
    std::array<float, kNumResonators + 1> input_volume_{};
    std::array<float, kNumResonators> delay_samples_{};
    std::array<float, ScatterMatrix::kNumReflections> reflections_{};
    std::array<float, kNumResonators> feedbacks_{};
private:
    void ProcessCommon(juce::AudioBuffer<float>&, juce::MidiBuffer&);
    void ProcessMidi(juce::AudioBuffer<float>&, juce::MidiBuffer&);
    void ProcessDSP(float* ptr, size_t len);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResonatorAudioProcessor)
};
