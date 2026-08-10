#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <memory>

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
    : AudioProcessor(BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withInput("Sidechain", juce::AudioChannelSet::stereo(), true)
#endif
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
      ) {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    params_.BuildLayout(layout);

    value_tree_ = std::make_unique<juce::AudioProcessorValueTreeState>(*this, nullptr, kParameterValueTreeIdentify,
                                                                       std::move(layout));
    params_.BeginListening();
    preset_manager_ = std::make_unique<pluginshared::PresetManager>(*value_tree_, *this);
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor() {
    params_.EndListening();
    value_tree_ = nullptr;
}

//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const {
    return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool AudioPluginAudioProcessor::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const {
    return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms() {
    return 1;
}

int AudioPluginAudioProcessor::getCurrentProgram() {
    return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram(int index) {
    juce::ignoreUnused(index);
}

const juce::String AudioPluginAudioProcessor::getProgramName(int index) {
    juce::ignoreUnused(index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName(int index, const juce::String& newName) {
    juce::ignoreUnused(index, newName);
}

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    engine_.Init(sampleRate, static_cast<size_t>(samplesPerBlock));

    // 首次进入时将所有模块标记为需要更新
    params_.should_update_tilt_.store(true, std::memory_order_release);
    params_.should_update_leaky_lpc_.store(true, std::memory_order_release);
    params_.should_update_block_lpc_.store(true, std::memory_order_release);
    params_.should_update_stft_.store(true, std::memory_order_release);
    params_.should_update_channel_vocoder_.store(true, std::memory_order_release);
    params_.should_update_pitch_osc_.store(true, std::memory_order_release);
}

void AudioPluginAudioProcessor::reset() {}

void AudioPluginAudioProcessor::releaseResources() {
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) return false;
#if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet()) return false;
#endif
    return true;
#endif
}

void AudioPluginAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    std::ignore = midiMessages;
    juce::ScopedNoDenormals noDenormals;

    // --- sync dirty params → Engine（内部按 atomic 标志更新各模块） ---
    engine_.Update(params_);

    // --- resolve channel routing ---
    bool const has_sidechain = buffer.getNumChannels() >= 4;

    bool const swap = params_.channel_swap.Get();
    int pitch_ch_idx = params_.pitch_channel.Get();             // 0=Off, 1=ch0, 2=ch1, 3=ch2, 4=ch3
    if (!has_sidechain && pitch_ch_idx >= 3) pitch_ch_idx -= 2; // ch2/3 → ch0/1
    bool const use_pitch = pitch_ch_idx > 0;
    int const pitch_ch = pitch_ch_idx - 1; // 0-based channel index when active

    int mod_ch = 0;
    int carry_ch = has_sidechain ? 2 : 0;
    if (swap) std::swap(mod_ch, carry_ch);

    green_vocoder::eVocoderType const vocoder_type = static_cast<green_vocoder::eVocoderType>(params_.vocoder_type.Get());

    // --- delegate to engine ---
    engine_.Process(buffer, mod_ch, carry_ch, pitch_ch, use_pitch, vocoder_type);

    // --- latency check ---
    int new_latency = engine_.GetLatency();
    if (new_latency != old_latency_) {
        old_latency_ = new_latency;
        setLatencySamples(new_latency);
    }
}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const {
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor() {
    return new AudioPluginAudioProcessorEditor(*this);
}

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    suspendProcessing(true);

    juce::ValueTree plugin_state{"PLUGIN_STATE"};
    plugin_state.appendChild(value_tree_->copyState(), nullptr);

    if (auto xml = plugin_state.createXml(); xml != nullptr) {
        copyXmlToBinary(*xml, destData);
    }

    suspendProcessing(false);
}

void AudioPluginAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    suspendProcessing(true);

    auto xml = *getXmlFromBinary(data, sizeInBytes);
    auto plugin_state = juce::ValueTree::fromXml(xml);
    if (plugin_state.isValid()) {
        auto parameter = plugin_state.getChildWithName(kParameterValueTreeIdentify);
        if (parameter.isValid()) {
            value_tree_->replaceState(parameter);
        }
    }

    reset();
    suspendProcessing(false);
}

void AudioPluginAudioProcessor::Panic() {
    const juce::ScopedLock lock{getCallbackLock()};
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new AudioPluginAudioProcessor();
}
