#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
VitalChorusAudioProcessor::VitalChorusAudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
{
    dsp_ = vital_chorus::CreateDsp();
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    params_.BuildLayout(layout);
    value_tree_ = std::make_unique<juce::AudioProcessorValueTreeState>(*this, nullptr, "PARAMETERS", std::move(layout));
    params_.BeginListening();
    preset_manager_ = std::make_unique<pluginshared::PresetManager>(*value_tree_, *this);
}

VitalChorusAudioProcessor::~VitalChorusAudioProcessor()
{
    params_.EndListening();
    value_tree_ = nullptr;
}

//==============================================================================
const juce::String VitalChorusAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool VitalChorusAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool VitalChorusAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool VitalChorusAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double VitalChorusAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int VitalChorusAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int VitalChorusAudioProcessor::getCurrentProgram()
{
    return 0;
}

void VitalChorusAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String VitalChorusAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void VitalChorusAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void VitalChorusAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    dsp_->Init(static_cast<float>(sampleRate));
    dsp_->Reset();

    params_.MarkChanged();
}

void VitalChorusAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool VitalChorusAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void VitalChorusAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    if (params_.IsParamChanged()) {
        dsp_->Update(params_.ToDspParam(static_cast<float>(getSampleRate()), getPlayHead()));
    }

    dsp_->SyncPhase(params_, getPlayHead());

    int const num_samples = buffer.getNumSamples();
    float* left_ptr = buffer.getWritePointer(0);
    float* right_ptr = buffer.getWritePointer(1);

    dsp_->Process(left_ptr, right_ptr, num_samples);
}

//==============================================================================
bool VitalChorusAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* VitalChorusAudioProcessor::createEditor()
{
    return new VitalChorusAudioProcessorEditor (*this);
    // return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void VitalChorusAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    suspendProcessing(true);
    if (auto state = value_tree_->copyState().createXml(); state != nullptr) {
        copyXmlToBinary(*state, destData);
    }
    suspendProcessing(false);
}

void VitalChorusAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    suspendProcessing(true);
    auto xml = *getXmlFromBinary(data, sizeInBytes);
    auto state = juce::ValueTree::fromXml(xml);
    if (state.isValid()) {
        value_tree_->replaceState(state);
    }
    reset();
    suspendProcessing(false);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VitalChorusAudioProcessor();
}
