#pragma once
#include "../../shared/juce_param_listener.hpp"
#include <numbers>

#include "qwqdsp/filter/one_pole.hpp"
#include "qwqdsp/convert.hpp"

// -------------------- paralle delaylines --------------------
namespace simplereverb {
class ParalleDelay {
public:
    void Init(size_t max_samples) {
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

    void Push(std::array<float, 16> const& x) noexcept {
        for (size_t i = 0; i < 16; ++i) {
            buffer_[i][wpos_] = x[i];
        }
        ++wpos_;
        wpos_ &= mask_;
    }

    std::array<float, 16> GetBeforePush(const std::array<float, 16>& delays) noexcept {
        std::array<float, 16> y;
        for (size_t i = 0; i < 16; ++i) {
            size_t rpos = wpos_ + mask_ + 1 - delays[i];
            rpos &= mask_;
            y[i] = buffer_[i][rpos];
        }
        return y;
    }

    void Tick(
        std::array<float, 16>& x,
        const std::array<float, 16>& delays
    ) noexcept {
        for (size_t i = 0; i < 16; ++i) {
            buffer_[i][wpos_] = x[i];

            size_t rpos = wpos_ + mask_ + 1 - delays[i];
            rpos &= mask_;
            x[i] = buffer_[i][rpos];
        }

        ++wpos_;
        wpos_ &= mask_;
    }
private:
    std::array<std::vector<float>, 16> buffer_;
    size_t wpos_{};
    size_t mask_{};
};

class ParalleOnepole {
public:
    void Tick(std::array<float, 16>& x) noexcept {
        for (size_t i = 0; i < 16; ++i) {
            x[i] = damp_[i].Tick(x[i]);
        }
    }

    void Reset() noexcept {
        for (auto& f : damp_) {
            f.Reset();
        }
    }

    void SetFrequency(float omega, float db) noexcept {
        if (omega < std::numbers::pi_v<float> - 1e-5f) {
            damp_[0].MakeHighShelf(omega, qwqdsp::convert::Db2Gain(db));
        }
        else {
            damp_[0].MakePass();
        }
        for (size_t i = 1; i < 16; ++i) {
            damp_[i].CopyFrom(damp_[0]);
        }
    }
private:
    std::array<qwqdsp::filter::OnePoleFilter, 16> damp_;
};
}

// ---------------------------------------- juce processor ----------------------------------------
class SimpleReverbAudioProcessor final : public juce::AudioProcessor
{
public:
    //==============================================================================
    SimpleReverbAudioProcessor();
    ~SimpleReverbAudioProcessor() override;

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

    juce::AudioParameterFloat* param_size_{};
    juce::AudioParameterFloat* param_decay_{};
    juce::AudioParameterFloat* param_damp_pitch_{};
    juce::AudioParameterFloat* param_damp_gain_{};

    simplereverb::ParalleDelay delay_;
    simplereverb::ParalleOnepole damp_;
private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleReverbAudioProcessor)
};
