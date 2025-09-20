#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SteepFlangerAudioProcessor::SteepFlangerAudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       .withInput("Sidechain", juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"notches", 1},
            "notches",
            juce::NormalisableRange<float>{1.0f, static_cast<float>(kMaxNumNotchs), 0.1f},
            1.0f
        );
        param_listener_.Add(p, [this](float num_notches) {
            juce::ScopedLock _{getCallbackLock()};
            notch_smoother_.SetTarget(num_notches);
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"cutoff", 1},
            "cutoff",
            juce::NormalisableRange<float>{0.01f, std::numbers::pi_v<float> - 0.01f, 0.01f},
            std::numbers::pi_v<float> / 2
        );
        param_listener_.Add(p, [this](float cutoff) {
            juce::ScopedLock _{getCallbackLock()};
            cutoff_w_ = cutoff;
            UpdateCoeff();
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"coeff_len", 1},
            "coeff_len",
            juce::NormalisableRange<float>{2.0f, static_cast<float>(kMaxCoeffLen), 1.0f},
            1.0f
        );
        param_listener_.Add(p, [this](float coeff_len) {
            juce::ScopedLock _{getCallbackLock()};
            coeff_len_ = coeff_len;
            UpdateCoeff();
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"side_lobe", 1},
            "side_lobe",
            juce::NormalisableRange<float>{20.0f, 100.0f, 0.1f},
            40.0f
        );
        param_listener_.Add(p, [this](float side_lobe) {
            juce::ScopedLock _{getCallbackLock()};
            side_lobe_ = side_lobe;
            UpdateCoeff();
        });
        layout.add(std::move(p));
    }

    value_tree_ = std::make_unique<juce::AudioProcessorValueTreeState>(*this, nullptr, "PARAMETERS", std::move(layout));
}

SteepFlangerAudioProcessor::~SteepFlangerAudioProcessor()
{
    value_tree_ = nullptr;
}

//==============================================================================
const juce::String SteepFlangerAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SteepFlangerAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SteepFlangerAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SteepFlangerAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SteepFlangerAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SteepFlangerAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SteepFlangerAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SteepFlangerAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String SteepFlangerAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void SteepFlangerAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void SteepFlangerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    delay_left_.Init(kMaxNumNotchs * kMaxCoeffLen + 10);
    delay_right_.Init(kMaxCoeffLen * kMaxNumNotchs + 10);
    notch_smoother_.Reset();
    notch_smoother_.SetSmoothTime(50.0f, sampleRate);
    param_listener_.CallAll();
}

void SteepFlangerAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool SteepFlangerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void SteepFlangerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    std::ignore = midiMessages;

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    size_t const len = buffer.getNumSamples();
    std::span<float> left{buffer.getWritePointer(0), len};
    std::span<float> right{buffer.getWritePointer(1), len};

    for (size_t i = 0; i < len; ++i) {
        delay_left_.Push(left[i]);
        delay_right_.Push(right[i]);

        float sum_left = 0;
        float sum_right = 0;
        float const num_notch = notch_smoother_.Tick();
        for (size_t i = 0; i < coeff_len_; ++i) {
            sum_left += coeffs_[i] * delay_left_.GetAfterPush(i * num_notch);
            sum_right += coeffs_[i] * delay_right_.GetAfterPush(i * num_notch);
        }
        left[i] = sum_left;
        right[i] = sum_right;
    }
}

//==============================================================================
bool SteepFlangerAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SteepFlangerAudioProcessor::createEditor()
{
    // return new SteepFlangerAudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void SteepFlangerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = value_tree_->copyState().createXml()) {
        copyXmlToBinary(*state, destData);
    }
}

void SteepFlangerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto state = juce::ValueTree::fromXml(*getXmlFromBinary(data, sizeInBytes));
    if (state.isValid()) {
        value_tree_->replaceState(state);
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SteepFlangerAudioProcessor();
}


#include "qwqdsp/filter/window_fir.hpp"
#include "qwqdsp/window/kaiser.hpp"

void SteepFlangerAudioProcessor::UpdateCoeff() {
    if (coeff_len_ == 2) {
        coeffs_[0] = 0.5f;
        coeffs_[1] = 0.5f;
        return;
    }
    std::span<float> kernel{coeffs_.data(), coeff_len_};
    qwqdsp::filter::WindowFIR::Lowpass(kernel, cutoff_w_);
    float const beta = qwqdsp::window::Kaiser::Beta(side_lobe_);
    qwqdsp::window::Kaiser::ApplyWindow(kernel, beta, false);
    
    float energy = 0;
    for (auto x : kernel) {
        energy += x * x;
    }
    float g = 1.0f / std::sqrt(energy);
    for (auto& x : kernel) {
        x *= g;
    }
}