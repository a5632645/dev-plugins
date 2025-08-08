#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "dsp/lattice_apf.hpp"
#include "juce_audio_processors/juce_audio_processors.h"
#include <array>
#include <cstddef>
#include <span>
#include "juce_core/juce_core.h"
#include "param_ids.hpp"

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kK, 1},
            id::kK,
            -0.9f, 0.9f, 0.2f
        );
        paramListeners_.Add(p, [this](float f){
            juce::ScopedLock lock{getCallbackLock()};
            dsp_.SetReflection(f);
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kBegin, 1},
            id::kBegin,
            0.1f, 80.0f, 5.0f
        );
        paramListeners_.Add(p, [this](float f){
            juce::ScopedLock lock{getCallbackLock()};
            dsp_.SetDelayBegin(f);
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kEnd, 1},
            id::kEnd,
            0.1f, 80.0f, 20.0f
        );
        paramListeners_.Add(p, [this](float f){
            juce::ScopedLock lock{getCallbackLock()};
            dsp_.SetDelayEnd(f);
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kFreq, 1},
            id::kFreq,
            0.1f, 5.0f, 0.5f
        );
        paramListeners_.Add(p, [this](float f){
            juce::ScopedLock lock{getCallbackLock()};
            dsp_.SetFrequency(f);
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kMix, 1},
            id::kMix,
            0.0f, 1.0f, 0.5f
        );
        paramListeners_.Add(p, [this](float f){
            juce::ScopedLock lock{getCallbackLock()};
            dsp_.SetMix(f);
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{id::kNum, 1},
            id::kNum,
            1, dsp::LatticeAPF::kNumBlock, 4
        );
        paramListeners_.Add(p, [this](int f){
            juce::ScopedLock lock{getCallbackLock()};
            dsp_.SetNumBlock(f);
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{id::kMono, 1},
            id::kMono,
            false
        );
        paramListeners_.Add(p, [this](bool p){
            juce::ScopedLock lock{getCallbackLock()};
            dsp_.SetMono(p);
        });
        layout.add(std::move(p));
    }

    value_tree_ = std::make_unique<juce::AudioProcessorValueTreeState>(*this, nullptr, "PARAMETERS", std::move(layout));
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
    value_tree_ = nullptr;
}

//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AudioPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String AudioPluginAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    dsp_.Init(100.0f, sampleRate);
    paramListeners_.CallAll();
}

void AudioPluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    std::span left{buffer.getWritePointer(0), static_cast<size_t>(buffer.getNumSamples())};
    std::span right{buffer.getWritePointer(1), static_cast<size_t>(buffer.getNumSamples())};
    dsp_.Process(left, right);
}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor (*this);
}

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = value_tree_->copyState().createXml()) {
        copyXmlToBinary(*state, destData);
    }
}

void AudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto state = juce::ValueTree::fromXml(*getXmlFromBinary(data, sizeInBytes));
    if (state.isValid()) {
        value_tree_->replaceState(state);
    }
}

void AudioPluginAudioProcessor::Panic() {
    const juce::ScopedLock lock{ getCallbackLock() };
}

void AudioPluginAudioProcessor::SetLatency() {
    
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}
