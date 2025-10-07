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
    juce::AudioParameterBool* relative_pitch_{};
    juce::AudioParameterFloat* global_pitch_{};
private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResonatorAudioProcessor)
};
