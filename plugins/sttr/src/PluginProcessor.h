
#pragma once

#include <pluginshared/juce_param_listener.hpp>
#include <pluginshared/preset_manager.hpp>
#include <pluginshared/wrap_parameters.hpp>

#include "dsp/SttrProcessor.hpp"

class SttrAudioProcessor final : public juce::AudioProcessor {
public:
    static constexpr auto kParameterValueTreeIdentify = "PARAMETERS";

    SttrAudioProcessor();
    ~SttrAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void reset() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // --- parameters ---
    pluginshared::FloatParam mixParam{
        "Mix", {0.0f, 1.0f, 0.01f},
         0.5f
    };
    pluginshared::FloatParam hopMsParam{
        "HopMs", {0.0f, 500.0f, 0.01f, 0.3f},
         4.0f
    };
    pluginshared::FloatParam dryDelayParam{
        "DryDelay", {0.0f, 1.0f, 0.01f},
         0.5f
    };
    pluginshared::ChoiceParam windowTypeParam{
        "WindowType",
        juce::StringArray{"Hann", "Hamming", "Blackman", "Blackman-Harris", "Nuttall", "Blackman-Nuttall"},
        "Hann"
    };
    pluginshared::FloatParam stretchParam{
        "Stretch", {0.7f, 1.4f, 0.01f},
         1.0f
    };

    // --- DSP engine ---
    SttrProcessor dsp_;

    JuceParamListener param_listener_;
    std::unique_ptr<juce::AudioProcessorValueTreeState> value_tree_;
    std::unique_ptr<pluginshared::PresetManager> preset_manager_;
private:
    void pullParameters();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SttrAudioProcessor)
};
