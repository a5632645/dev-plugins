#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ResonatorAudioProcessor::ResonatorAudioProcessor()
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

    for (size_t i = 0; i < kNumResonators; ++i) {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::String{"pitch"} + juce::String{i},
            juce::String{"pitch"} + juce::String{i},
            juce::NormalisableRange<float>{0.0f, 127.0f, 1.0f},
            60.0f
        );
        pitches_[i] = p.get();
        layout.add(std::move(p));
    }
    for (size_t i = 0; i < kNumResonators; ++i) {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::String{"fine"} + juce::String{i},
            juce::String{"fine"} + juce::String{i},
            juce::NormalisableRange<float>{-100.0f, 100.0f, 1.0f},
            0.0f
        );
        fine_tune_[i] = p.get();
        layout.add(std::move(p));
    }
    for (size_t i = 0; i < kNumResonators; ++i) {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::String{"damp"} + juce::String{i},
            juce::String{"damp"} + juce::String{i},
            100, 140, 130
        );
        damp_pitch_[i] = p.get();
        layout.add(std::move(p));
    }
    for (size_t i = 0; i < kNumResonators; ++i) {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::String{"dispersion"} + juce::String{i},
            juce::String{"dispersion"} + juce::String{i},
            0, 1.0f, 0
        );
        dispersion_pole_radius_[i] = p.get();
        layout.add(std::move(p));
    }
    for (size_t i = 0; i < kNumResonators; ++i) {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::String{"decay"} + juce::String{i},
            juce::String{"decay"} + juce::String{i},
            0.0f, 32000.0f, i == 0 ? 500.0f : 0.0f
        );
        decays_[i] = p.get();
        layout.add(std::move(p));
    }
    for (size_t i = 0; i < ScatterMatrix::kNumReflections; ++i) {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::String{"reflection"} + juce::String{i},
            juce::String{"reflection"} + juce::String{i},
            -std::numbers::pi_v<float>, std::numbers::pi_v<float>, 0.0f
        );
        matrix_reflections_[i] = p.get();
        layout.add(std::move(p));
    }
    for (size_t i = 0; i < kNumResonators; ++i) {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::String{"mix"} + juce::String{i},
            juce::String{"mix"} + juce::String{i},
            -61.0f, 0.0f, i == 0 ? 0.0f : -61.0f
        );
        mix_volume_[i] = p.get();
        layout.add(std::move(p));
    }
    for (size_t i = 0; i < kNumResonators; ++i) {
        auto p = std::make_unique<juce::AudioParameterBool>(
            juce::String{"polarity"} + juce::String{i},
            juce::String{"polarity"} + juce::String{i},
            false
        );
        polarity_[i] = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterBool>(
            "midi_drive",
            "midi_drive",
            false
        );
        midi_drive_ = p.get();
        layout.add(std::move(p));
    }
    {
        
    }
    {

    }

    value_tree_ = std::make_unique<juce::AudioProcessorValueTreeState>(*this, nullptr, "PARAMETERS", std::move(layout));
}

ResonatorAudioProcessor::~ResonatorAudioProcessor()
{
    value_tree_ = nullptr;
}

//==============================================================================
const juce::String ResonatorAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ResonatorAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ResonatorAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ResonatorAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ResonatorAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ResonatorAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int ResonatorAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ResonatorAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String ResonatorAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void ResonatorAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void ResonatorAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    damp_.Reset();
    delay_.Init(sampleRate, 0.0f);
    delay_.Reset();
    dispersion_.Reset();
}

void ResonatorAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool ResonatorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void ResonatorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    size_t const num_samples = buffer.getNumSamples();
    float* left_ptr = buffer.getWritePointer(0);
    float* right_ptr = buffer.getWritePointer(1);

    std::array<float, kNumResonators> pole_radius;
    for (size_t i = 0; i < kNumResonators; ++i) {
        pole_radius[i] = dispersion_pole_radius_[i]->get();
    }

    // update delaylines
    std::array<float, kNumResonators> delay_samples;
    std::array<float, kNumResonators> omegas;
    std::array<float, kNumResonators> allpass_set_delay;
    for (size_t i = 0; i < kNumResonators; ++i) {
        float pitch = pitches_[i]->get() + fine_tune_[i]->get() / 100.0f;
        if (polarity_[i]->get()) {
            pitch += 12;
        }
        float const freq = qwqdsp::convert::Pitch2Freq(pitch);
        omegas[i] = freq * std::numbers::pi_v<float> / getSampleRate();
        delay_samples[i] = getSampleRate() / freq;
        allpass_set_delay[i] = delay_samples[i] * pole_radius[i] / (mana::ThrianAllpass::kMaxNumAPF + 0.1f);
    }

    // update allpass filters
    auto allpass_delay = dispersion_.SetFilter(allpass_set_delay, omegas);

    // update decays
    std::array<float, kNumResonators> feedback;
    for (size_t i = 0; i < kNumResonators; ++i) {
        float const decay_ms = decays_[i]->get();
        if (decay_ms > 0.5f) {
            float const mul = -3.0f * delay_samples[i] / (getSampleRate() * decay_ms / 1000.0f);
            feedback[i] = std::pow(10.0f, mul);
            feedback[i] = std::min(feedback[i], 1.0f);
        }
        else {
            feedback[i] = 0;
        }

        if (polarity_[i]->get()) {
            feedback[i] = -feedback[i];
        }
    }

    // remove allpass delays
    for (size_t i = 0; i < kNumResonators; ++i) {
        delay_samples[i] -= allpass_delay[i];
        delay_samples[i] = std::max(delay_samples[i], 0.0f);
    }

    // update scatter matrix
    std::array<float, ScatterMatrix::kNumReflections> reflections;
    for (size_t i = 0; i < ScatterMatrix::kNumReflections; ++i) {
        reflections[i] = matrix_reflections_[i]->get();
    }

    // update damp filter
    std::array<float, kNumResonators> damp_w;
    for (size_t i = 0; i < kNumResonators; ++i) {
        float const freq = qwqdsp::convert::Pitch2Freq(damp_pitch_[i]->get());
        damp_w[i] = freq * std::numbers::pi_v<float> * 2 / getSampleRate();
    }
    damp_.SetFrequency(damp_w);

    // output mix volumes
    std::array<float, kNumResonators> mix;
    for (size_t i = 0; i < kNumResonators; ++i) {
        float const db = mix_volume_[i]->get();
        if (db < -60.0f) {
            mix[i] = 0;
        }
        else {
            mix[i] = qwqdsp::convert::Db2Gain(db);
        }
    }

    // processing
    for (size_t i = 0; i < num_samples; ++i) {
        float out = left_ptr[i];
        for (size_t j = 0; j < kNumResonators; ++j) {
            out += fb_values_[j] * mix[j];
        }

        for (size_t j = 0; j < kNumResonators; ++j) {
            fb_values_[j] += left_ptr[i];
        }

        delay_.Tick(fb_values_, delay_samples);
        dispersion_.Tick(fb_values_);
        damp_.Tick(fb_values_);
        matrix_.Tick(fb_values_, reflections);
        for (size_t j = 0; j < kNumResonators; ++j) {
            fb_values_[j] *= feedback[j];
        }

        left_ptr[i] = out;
    }

    std::copy_n(left_ptr, num_samples, right_ptr);
}

//==============================================================================
bool ResonatorAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* ResonatorAudioProcessor::createEditor()
{
    return new ResonatorAudioProcessorEditor (*this);
    // return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void ResonatorAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    suspendProcessing(true);
    if (auto state = value_tree_->copyState().createXml(); state != nullptr) {
        copyXmlToBinary(*state, destData);
    }
    suspendProcessing(false);
}

void ResonatorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
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
    return new ResonatorAudioProcessor();
}
