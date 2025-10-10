#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace simplereverb{
// this copy from https://github.com/mtytel/vital/blob/main/src/synthesis/effects/reverb.cpp
constexpr float kFeedbackDelays[16] = {
    6753.2f, 9278.4f, 7704.5f, 11328.5f,
    9701.12f, 5512.5f, 8480.45f, 5638.65f,
    3120.73f, 3429.5f, 3626.37f, 7713.52f,
    4521.54f, 6518.97f, 5265.56f, 5630.25,
};

static constexpr void SingleScalar(float& a, float& b) noexcept {
    constexpr float g = std::numbers::sqrt2_v<float> / 2;
    float const t = a + b;
    b = (b - a) * g;
    a = t * g;
}
}

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
        param_decay_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            "damp",
            "damp",
            100, 140, 130
        );
        param_damp_pitch_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            "gain",
            "gain",
            -10.0f, 0.0f, -6.01f
        );
        param_damp_gain_ = p.get();
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
    delay_.Init(65536);
    delay_.Reset();
    damp_.Reset();
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

    size_t const num_samples = buffer.getNumSamples();
    float* left_ptr = buffer.getWritePointer(0);
    float* right_ptr = buffer.getWritePointer(1);

    float const fs_ratio = getSampleRate() / 44100.0f;
    float const size_mul = std::exp2(4 * param_size_->get() + -3) * fs_ratio;
    std::array<float, 16> delay_samples;
    for (size_t i = 0; i < 16; ++i) {
        delay_samples[i] = simplereverb::kFeedbackDelays[i] * size_mul;
    }

    std::array<float, 16> decays;
    float const decay_period = 1000.0f / (param_decay_->get() * getSampleRate());
    for (size_t i = 0; i < 16; ++i) {
        decays[i] = std::pow(1e-3f, delay_samples[i] * decay_period);
    }

    damp_.SetFrequency(
        qwqdsp::convert::Pitch2Freq(param_damp_pitch_->get()) * std::numbers::pi_v<float> * 2 / getSampleRate(),
        param_damp_gain_->get()
    );

    for (size_t i = 0; i < num_samples; ++i) {
        float const x = left_ptr[i] * 0.25f;
        auto delay_out = delay_.GetBeforePush(delay_samples);
        for (size_t j = 0; j < 16; ++j) {
            delay_out[j] = x + delay_out[j] * decays[j];
        }
        damp_.Tick(delay_out);

        simplereverb::SingleScalar(delay_out[0], delay_out[1]);
        simplereverb::SingleScalar(delay_out[2], delay_out[3]);
        simplereverb::SingleScalar(delay_out[4], delay_out[5]);
        simplereverb::SingleScalar(delay_out[6], delay_out[7]);
        simplereverb::SingleScalar(delay_out[8], delay_out[9]);
        simplereverb::SingleScalar(delay_out[10], delay_out[11]);
        simplereverb::SingleScalar(delay_out[12], delay_out[13]);
        simplereverb::SingleScalar(delay_out[14], delay_out[15]);
        
        simplereverb::SingleScalar(delay_out[1], delay_out[2]);
        simplereverb::SingleScalar(delay_out[3], delay_out[4]);
        simplereverb::SingleScalar(delay_out[5], delay_out[6]);
        simplereverb::SingleScalar(delay_out[7], delay_out[8]);
        simplereverb::SingleScalar(delay_out[9], delay_out[10]);
        simplereverb::SingleScalar(delay_out[11], delay_out[12]);
        simplereverb::SingleScalar(delay_out[13], delay_out[14]);
        simplereverb::SingleScalar(delay_out[15], delay_out[0]);

        delay_.Push(delay_out);

        left_ptr[i] = std::accumulate(delay_out.begin(), delay_out.end(), 0.0f);
    }

    std::copy_n(left_ptr, num_samples, right_ptr);
}

//==============================================================================
bool SimpleReverbAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SimpleReverbAudioProcessor::createEditor()
{
    // return new SimpleReverbAudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor(*this);
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
