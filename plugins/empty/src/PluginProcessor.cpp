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

    // Rate (Hz)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        param_ids::rate, "Rate",
        juce::NormalisableRange<float>(param_ranges::rateMin, param_ranges::rateMax, 0.01f, 0.3f),
        param_ranges::rateDefault));

    // Depth (0-1)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        param_ids::depth, "Depth",
        juce::NormalisableRange<float>(param_ranges::depthMin, param_ranges::depthMax, 0.01f),
        param_ranges::depthDefault));

    // Feedback (0-0.95)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        param_ids::feedback, "Feedback",
        juce::NormalisableRange<float>(param_ranges::feedbackMin, param_ranges::feedbackMax, 0.01f),
        param_ranges::feedbackDefault));

    // Mix (0-1)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        param_ids::mix, "Mix",
        juce::NormalisableRange<float>(param_ranges::mixMin, param_ranges::mixMax, 0.01f),
        param_ranges::mixDefault));

    // Stages
    juce::StringArray stageChoices;
    for (int s = param_ranges::stagesMin; s <= param_ranges::stagesMax; s += param_ranges::stagesStep)
        stageChoices.add(juce::String(s));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        param_ids::stages, "Stages", stageChoices, 
        (param_ranges::stagesDefault - param_ranges::stagesMin) / param_ranges::stagesStep));

    value_tree_ = std::make_unique<juce::AudioProcessorValueTreeState>(*this, nullptr, kParameterValueTreeIdentify, std::move(layout));
    preset_manager_ = std::make_unique<pluginshared::PresetManager>(*value_tree_, *this);

    // Sync initial parameters to phaser
    phaser_.setRate(*value_tree_->getRawParameterValue(param_ids::rate));
    phaser_.setDepth(*value_tree_->getRawParameterValue(param_ids::depth));
    phaser_.setFeedback(*value_tree_->getRawParameterValue(param_ids::feedback));
    phaser_.setMix(*value_tree_->getRawParameterValue(param_ids::mix));
    phaser_.setStages(param_ranges::stagesMin + 
        static_cast<int>(*value_tree_->getRawParameterValue(param_ids::stages)) * param_ranges::stagesStep);
}

EmptyAudioProcessor::~EmptyAudioProcessor()
{
    preset_manager_ = nullptr;
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
    phaser_.prepare(sampleRate, samplesPerBlock);

    // Sync parameters to phaser
    phaser_.setRate(*value_tree_->getRawParameterValue(param_ids::rate));
    phaser_.setDepth(*value_tree_->getRawParameterValue(param_ids::depth));
    phaser_.setFeedback(*value_tree_->getRawParameterValue(param_ids::feedback));
    phaser_.setMix(*value_tree_->getRawParameterValue(param_ids::mix));
    const int stageIdx = static_cast<int>(*value_tree_->getRawParameterValue(param_ids::stages));
    phaser_.setStages(param_ranges::stagesMin + stageIdx * param_ranges::stagesStep);
}

void EmptyAudioProcessor::releaseResources()
{
    phaser_.reset();
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
    juce::ignoreUnused (midiMessages);

    const int numSamples = buffer.getNumSamples();

    // Sync parameters from APVTS → atomic phaser params (audio thread safe)
    phaser_.setRate(*value_tree_->getRawParameterValue(param_ids::rate));
    phaser_.setDepth(*value_tree_->getRawParameterValue(param_ids::depth));
    phaser_.setFeedback(*value_tree_->getRawParameterValue(param_ids::feedback));
    phaser_.setMix(*value_tree_->getRawParameterValue(param_ids::mix));
    const int stageIdx = static_cast<int>(*value_tree_->getRawParameterValue(param_ids::stages));
    phaser_.setStages(param_ranges::stagesMin + stageIdx * param_ranges::stagesStep);

    // Process audio
    float* left  = buffer.getWritePointer(0);
    float* right = buffer.getWritePointer(1);
    phaser_.process(left, right, numSamples);
}

//==============================================================================
bool EmptyAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* EmptyAudioProcessor::createEditor()
{
    return new EmptyAudioProcessorEditor (*this);
    // return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void EmptyAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    suspendProcessing(true);
    
    juce::ValueTree plugin_state{"PLUGIN_STATE"};
    plugin_state.appendChild(value_tree_->copyState(), nullptr);
    
    if (auto xml = plugin_state.createXml(); xml != nullptr) {
        copyXmlToBinary(*xml, destData);
    }

    suspendProcessing(false);
}

void EmptyAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
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
    return new EmptyAudioProcessor();
}
