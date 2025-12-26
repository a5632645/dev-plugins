#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
EwarpAudioProcessor::EwarpAudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
    , warp_bands_("warp", {1.0f, 1000.0f, 0.1f, 0.4f}, 100.0f)
    , ratio_("ratio", {0.1f, 12.0f, 0.1f}, 1.0f)
    , am2rm_("am2rm", {0.0f, 1.0f, 0.01f}, 0.5)
    , decay_("decay", {0.1f, 0.99f, 0.001f}, 0.6f)
    , reverse_("reverse", {0.0f, 1.0f, 0.01f}, 0.5f)
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(warp_bands_.Build());
    layout.add(ratio_.Build());
    layout.add(am2rm_.Build());
    layout.add(decay_.Build());
    layout.add(reverse_.Build());

    value_tree_ = std::make_unique<juce::AudioProcessorValueTreeState>(*this, nullptr, kParameterValueTreeIdentify, std::move(layout));
    preset_manager_ = std::make_unique<pluginshared::PresetManager>(*value_tree_, *this);
}

EwarpAudioProcessor::~EwarpAudioProcessor()
{
    value_tree_ = nullptr;
}

//==============================================================================
const juce::String EwarpAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool EwarpAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool EwarpAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool EwarpAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double EwarpAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int EwarpAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int EwarpAudioProcessor::getCurrentProgram()
{
    return 0;
}

void EwarpAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String EwarpAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void EwarpAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void EwarpAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    ewarp_.fs = static_cast<float>(sampleRate);
    param_listener_.CallAll();
}

void EwarpAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool EwarpAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void EwarpAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    size_t const num_samples = buffer.getNumSamples();
    float* left_ptr = buffer.getWritePointer(0);
    float* right_ptr = buffer.getWritePointer(1);

    ewarp_.am2rm = am2rm_.Get();
    ewarp_.ratio = ratio_.Get();
    ewarp_.decay = decay_.Get();
    ewarp_.warp_bands = warp_bands_.Get();
    ewarp_.reverse_mix_ = reverse_.Get();
    ewarp_.Update();
    ewarp_.Process(left_ptr, right_ptr, num_samples);
}

//==============================================================================
bool EwarpAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* EwarpAudioProcessor::createEditor()
{
    return new EwarpAudioProcessorEditor (*this);
    // return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void EwarpAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    suspendProcessing(true);
    
    juce::ValueTree plugin_state{"PLUGIN_STATE"};
    plugin_state.appendChild(value_tree_->copyState(), nullptr);
    
    if (auto xml = plugin_state.createXml(); xml != nullptr) {
        copyXmlToBinary(*xml, destData);
    }

    suspendProcessing(false);
}

void EwarpAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    suspendProcessing(true);

    auto xml = *getXmlFromBinary(data, sizeInBytes);
    auto plugin_state = juce::ValueTree::fromXml(xml);
    if (plugin_state.isValid()) {
        auto parameter = plugin_state.getChildWithName(kParameterValueTreeIdentify);
        value_tree_->replaceState(parameter);
    }

    suspendProcessing(false);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EwarpAudioProcessor();
}
