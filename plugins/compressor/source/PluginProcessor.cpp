#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
EmptyAudioProcessor::EmptyAudioProcessor()
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
            "attack",
            "attack",
            0, 1000,
            0
        );
        param_listener_.Add(p, [this](float v) {
            juce::ScopedLock _{getCallbackLock()};
            dsp_.SetAttack(v, getSampleRate());
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            "release",
            "release",
            0, 1000,
            0
        );
        param_listener_.Add(p, [this](float v) {
            juce::ScopedLock _{getCallbackLock()};
            dsp_.SetRelease(v, getSampleRate());
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            "threshold",
            "threshold",
            -60, 6,
            0
        );
        param_listener_.Add(p, [this](float v) {
            juce::ScopedLock _{getCallbackLock()};
            dsp_.SetThreshould(v);
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            "knee",
            "knee",
            0, 18,
            6
        );
        param_listener_.Add(p, [this](float v) {
            juce::ScopedLock _{getCallbackLock()};
            dsp_.knee_db_ = v;
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            "ratio",
            "ratio",
            0, 1,
            0
        );
        param_listener_.Add(p, [this](float v) {
            juce::ScopedLock _{getCallbackLock()};
            dsp_.ratio_ = v;
        });
        layout.add(std::move(p));
    }

    value_tree_ = std::make_unique<juce::AudioProcessorValueTreeState>(*this, nullptr, "PARAMETERS", std::move(layout));
}

EmptyAudioProcessor::~EmptyAudioProcessor()
{
    value_tree_ = nullptr;
}

//==============================================================================
const juce::String EmptyAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool EmptyAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool EmptyAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool EmptyAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double EmptyAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int EmptyAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int EmptyAudioProcessor::getCurrentProgram()
{
    return 0;
}

void EmptyAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String EmptyAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void EmptyAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void EmptyAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    dsp_.Init(sampleRate);
    param_listener_.CallAll();
}

void EmptyAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool EmptyAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void EmptyAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    size_t const len = buffer.getNumSamples();
    float* left_ptr = buffer.getWritePointer(0);
    float* right_ptr = buffer.getWritePointer(1);
    std::span<float> left_block{left_ptr, len};

    dsp_.ProcessRMS({left_ptr, len});
    for (size_t i = 0; i < len; ++i) {
        jassert(!(std::isnan(left_ptr[i]) || std::isinf(left_ptr[i])));
        right_ptr[i] = left_ptr[i];
    }
}

//==============================================================================
bool EmptyAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* EmptyAudioProcessor::createEditor()
{
    // return new SteepFlangerAudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void EmptyAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    suspendProcessing(true);
    if (auto state = value_tree_->copyState().createXml(); state != nullptr) {
        copyXmlToBinary(*state, destData);
    }
    suspendProcessing(false);
}

void EmptyAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    suspendProcessing(true);
    auto xml = *getXmlFromBinary(data, sizeInBytes);
    auto state = juce::ValueTree::fromXml(xml);
    if (state.isValid()) {
        value_tree_->replaceState(state);
    }
    suspendProcessing(false);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EmptyAudioProcessor();
}
