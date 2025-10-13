#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimpleReverbAudioProcessor::SimpleReverbAudioProcessor()
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
            "chorus amount",
            "chorus amount",
            0.0f, 1.0f, 0.05f
        );
        param_chorus_amount_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            "chorus freq",
            "chorus freq",
            juce::NormalisableRange<float>{0.003f, 8.0f, 0.001f, 0.4f},
            0.25f
        );
        param_chorus_freq_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            "mix",
            "mix",
            0.0f, 1.0f, 0.25f
        );
        param_wet_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            "pre lowpass",
            "pre lowpass",
            0.0f, 130.0f, 0.0f
        );
        param_pre_lowpass_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            "pre highpass",
            "pre highpass",
            0.0f, 130.0f, 110.0f
        );
        param_pre_highpass_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            "low damp",
            "low damp",
            0.0f, 130.0f, 0.0f
        );
        param_low_damp_pitch_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            "high damp",
            "high damp",
            0.0f, 130.0f, 90.0f
        );
        param_high_damp_pitch_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            "low gain",
            "low gain",
            -6.0f, 0.0f, 0.0f
        );
        param_low_damp_db_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            "high gain",
            "high gain",
            -6.0f, 0.0f, -1.0f
        );
        param_high_damp_db_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            "size",
            "size",
            0.0f, 1.0f, 0.5f
        );
        param_size_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            "decay",
            "decay",
            juce::NormalisableRange<float>{15.0f, 64000.0f, 1.0f, 0.4f},
            1000.0f
        );
        param_decay_ms_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            "predelay",
            "predelay",
            juce::NormalisableRange<float>{0.0f, 300.0f, 1.0f, 0.4f},
            0.0f
        );
        param_predelay_ = p.get();
        layout.add(std::move(p));
    }

    value_tree_ = std::make_unique<juce::AudioProcessorValueTreeState>(*this, nullptr, "PARAMETERS", std::move(layout));
}

SimpleReverbAudioProcessor::~SimpleReverbAudioProcessor()
{
    value_tree_ = nullptr;
}

//==============================================================================
const juce::String SimpleReverbAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SimpleReverbAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SimpleReverbAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SimpleReverbAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SimpleReverbAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SimpleReverbAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SimpleReverbAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SimpleReverbAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String SimpleReverbAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void SimpleReverbAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void SimpleReverbAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    dsp_.Init(static_cast<float>(sampleRate));
    dsp_.Reset();
    param_listener_.CallAll();
}

void SimpleReverbAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool SimpleReverbAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void SimpleReverbAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    size_t const num_samples = static_cast<size_t>(buffer.getNumSamples());
    float* left_ptr = buffer.getWritePointer(0);
    float* right_ptr = buffer.getWritePointer(1);

    dsp_.chorus_amount = param_chorus_amount_->get();
    dsp_.chorus_freq = param_chorus_freq_->get();
    dsp_.wet = param_wet_->get();
    dsp_.pre_lowpass = param_pre_lowpass_->get();
    dsp_.pre_highpass = param_pre_highpass_->get();
    dsp_.low_damp_pitch = param_low_damp_pitch_->get();
    dsp_.high_damp_pitch = param_high_damp_pitch_->get();
    dsp_.low_damp_db = param_low_damp_db_->get();
    dsp_.high_damp_db = param_high_damp_db_->get();
    dsp_.size = param_size_->get();
    dsp_.decay_ms = param_decay_ms_->get();
    dsp_.pre_delay = param_predelay_->get();

    std::array<SimdType, 512> temp_in;
    std::array<SimdType, 512> temp_out;

    size_t offset = 0;
    while (offset != num_samples) {
        size_t const cando = std::min(512ull, num_samples - offset);

        // shuffle
        for (size_t j = 0; j < cando; ++j) {
            temp_in[j].x[0] = *left_ptr;
            temp_in[j].x[1] = *right_ptr;
            temp_in[j].x[2] = *left_ptr;
            temp_in[j].x[3] = *right_ptr;
            temp_in[j] *= SimdType::FromSingle(0.5f);
            ++left_ptr;
            ++right_ptr;
        }

        dsp_.Process({temp_in.data(), cando}, {temp_out.data(), cando});

        // shuffle back
        left_ptr -= cando;
        right_ptr -= cando;
        for (size_t j = 0; j < cando; ++j) {
            SimdType t = temp_out[j];
            *left_ptr = t.x[0] + t.x[2];
            *right_ptr = t.x[1] + t.x[3];
            jassert(!std::isnan(*left_ptr));
            jassert(!std::isnan(*right_ptr));
            ++left_ptr;
            ++right_ptr;
        }

        offset += cando;
    }
}

//==============================================================================
bool SimpleReverbAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SimpleReverbAudioProcessor::createEditor()
{
    return new SimpleReverbAudioProcessorEditor (*this);
    // return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void SimpleReverbAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    suspendProcessing(true);
    if (auto state = value_tree_->copyState().createXml(); state != nullptr) {
        copyXmlToBinary(*state, destData);
    }
    suspendProcessing(false);
}

void SimpleReverbAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
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
    return new SimpleReverbAudioProcessor();
}
