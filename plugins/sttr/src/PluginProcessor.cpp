#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SttrAudioProcessor::SttrAudioProcessor()
    : AudioProcessor(BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
      ) {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout += mixParam;
    layout += hopMsParam;
    layout += dryDelayParam;
    layout += stretchParam;
    layout += windowTypeParam;

    value_tree_ = std::make_unique<juce::AudioProcessorValueTreeState>(*this, nullptr, kParameterValueTreeIdentify,
                                                                       std::move(layout));
    preset_manager_ = std::make_unique<pluginshared::PresetManager>(*value_tree_, *this);
}

SttrAudioProcessor::~SttrAudioProcessor() {
    preset_manager_ = nullptr;
    value_tree_ = nullptr;
}

//==============================================================================
const juce::String SttrAudioProcessor::getName() const {
    return JucePlugin_Name;
}

bool SttrAudioProcessor::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool SttrAudioProcessor::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool SttrAudioProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double SttrAudioProcessor::getTailLengthSeconds() const {
    return 0.0;
}

int SttrAudioProcessor::getNumPrograms() {
    return 1;
}

int SttrAudioProcessor::getCurrentProgram() {
    return 0;
}

void SttrAudioProcessor::setCurrentProgram(int index) {
    juce::ignoreUnused(index);
}

const juce::String SttrAudioProcessor::getProgramName(int index) {
    juce::ignoreUnused(index);
    return {};
}

void SttrAudioProcessor::changeProgramName(int index, const juce::String& newName) {
    juce::ignoreUnused(index, newName);
}

bool SttrAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    // stereo only
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo();
}

//==============================================================================
void SttrAudioProcessor::pullParameters() {
    SttrProcessor::Parameters p;
    p.mix = mixParam.Get();
    p.hopMs = hopMsParam.Get();
    p.dryDelay = dryDelayParam.Get();
    p.stretch = stretchParam.Get();
    p.windowType = static_cast<Window::Type>(windowTypeParam.Get());
    dsp_.setParameters(p);
}

void SttrAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    juce::ignoreUnused(samplesPerBlock);
    pullParameters();
    dsp_.prepare(static_cast<float>(sampleRate));
}

void SttrAudioProcessor::releaseResources() {
    dsp_.reset();
}

void SttrAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    pullParameters();

    dsp_.processBlock(buffer.getWritePointer(0), buffer.getWritePointer(1), buffer.getNumSamples());
}

//==============================================================================
bool SttrAudioProcessor::hasEditor() const {
    return true;
}

juce::AudioProcessorEditor* SttrAudioProcessor::createEditor() {
    return new SttrAudioProcessorEditor(*this);
}

//==============================================================================
void SttrAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    suspendProcessing(true);

    juce::ValueTree plugin_state{"PLUGIN_STATE"};
    plugin_state.appendChild(value_tree_->copyState(), nullptr);

    if (auto xml = plugin_state.createXml(); xml != nullptr) {
        copyXmlToBinary(*xml, destData);
    }

    suspendProcessing(false);
}

void SttrAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
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
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new SttrAudioProcessor();
}
