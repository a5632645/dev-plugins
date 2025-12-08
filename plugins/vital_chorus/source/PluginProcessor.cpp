#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "qwqdsp/convert.hpp"

static juce::StringArray const kTempoStrings {
    "freeze", "32/1", "16/1", "8/1", "4/1", "2/1", "1/1", "1/2", "1/4", "1/8", "1/16"
};
static constexpr std::array const kTempoMuls {
    0.0f, 1.0f/128.0f, 1.0f/64.0f, 1.0f/32.0f, 1.0f/16.0f, 1.0f/8.0f, 1.0f/4.0f, 1.0f/2.0f, 1.0f, 2.0f, 4.0f
};

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
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            "freq",
            "freq",
            juce::NormalisableRange<float>{1.0f / 64.0f, 8.0f},
            0.125f
        );
        param_freq_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            "depth",
            "depth",
            0.0f, 1.0f,
            0.5f
        );
        param_depth_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            "delay1",
            "delay1",
            0.976f, 19.99f,
            1.953f
        );
        param_delay1_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            "delay2",
            "delay2",
            0.976f, 19.99f,
            7.812f
        );
        param_delay2_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            "feedback",
            "feedback",
            -0.95f, 0.95f,
            0.4f
        );
        param_feedback_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            "mix",
            "mix",
            0.0f, 1.0f,
            0.5f
        );
        param_mix_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            "cutoff",
            "cutoff",
            8.0f, 136.0f,
            60.0f
        );
        param_cutoff_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            "spread",
            "spread",
            0.0f, 1.0f,
            1.0f
        );
        param_spread_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            "num_voices",
            "num_voices",
            juce::NormalisableRange<float>{4.0f, VitalChorus::kMaxNumChorus, 4.0f},
            16.0f
        );
        param_num_voices_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterChoice>(
            "tempo",
            "tempo",
            kTempoStrings,
            4
        );
        param_tempo_idx_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterInt>(
            "sync_type",
            "sync_type",
            0, static_cast<int>(LFOTempoType::NumTypes) - 1,
            static_cast<int>(LFOTempoType::Sync)
        );
        param_sync_type_ = p.get();
        layout.add(std::move(p));
    }

    value_tree_ = std::make_unique<juce::AudioProcessorValueTreeState>(*this, nullptr, "PARAMETERS", std::move(layout));
    preset_manager_ = std::make_unique<pluginshared::PresetManager>(*value_tree_, *this);
}

VitalChorusAudioProcessor::~VitalChorusAudioProcessor()
{
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
    dsp_.Init(static_cast<float>(sampleRate));
    dsp_.Reset();
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

void VitalChorusAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    float fbpm = 120.0f;
    float fppq = 0.0f;
    bool sync_lfo = false;
    if (auto* head = getPlayHead(); head != nullptr) {
        if (auto pos = head->getPosition(); pos) {
            if (auto bpm = pos->getBpm(); bpm) {
                fbpm = static_cast<float>(*bpm);
            }
            if (auto ppq = pos->getPpqPosition(); ppq) {
                fppq = static_cast<float>(*ppq);
                sync_lfo = true;
            }
            if (!pos->getIsPlaying()) {
                sync_lfo = false;
            }
        }
    }

    LFOTempoType tempo_type = static_cast<LFOTempoType>(param_sync_type_->get());
    if (tempo_type == LFOTempoType::Free) {
        dsp_.SetRate(param_freq_->get());
    }
    else {
        float sync_rate = kTempoMuls[static_cast<size_t>(param_tempo_idx_->getIndex())];
        if (tempo_type == LFOTempoType::SyncDot) {
            sync_rate *= 2.0f / 3.0f;
        }
        else if (tempo_type == LFOTempoType::SyncTri) {
            sync_rate *= 3.0f / 2.0f;
        }
        
        if (sync_lfo) {
            float sync_phase = sync_rate * fppq;
            sync_phase -= std::floor(sync_phase);
            dsp_.SyncLFOPhase(sync_phase);
        }

        float const lfo_freq = sync_rate * fbpm / 60.0f;
        dsp_.SetRate(lfo_freq);
    }

    size_t const num_samples = static_cast<size_t>(buffer.getNumSamples());
    float* left_ptr = buffer.getWritePointer(0);
    float* right_ptr = buffer.getWritePointer(1);

    dsp_.delay1 = param_delay1_->get();
    dsp_.delay2 = param_delay2_->get();
    dsp_.depth = param_depth_->get();
    dsp_.feedback = param_feedback_->get();
    dsp_.mix = param_mix_->get();
    dsp_.SetNumVoices(static_cast<size_t>(param_num_voices_->get()));

    float filter_radius = param_spread_->get() * 8 * 12;
    float low_freq = qwqdsp::convert::Pitch2Freq(param_cutoff_->get() + filter_radius);
    float high_freq = qwqdsp::convert::Pitch2Freq(param_cutoff_->get() - filter_radius);
    low_freq = low_freq * std::numbers::pi_v<float> * 2 / static_cast<float>(getSampleRate());
    high_freq = high_freq * std::numbers::pi_v<float> * 2 / static_cast<float>(getSampleRate());
    dsp_.SetFilter(low_freq, high_freq);

    dsp_.Process({left_ptr, num_samples}, {right_ptr, num_samples});
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
    suspendProcessing(false);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VitalChorusAudioProcessor();
}
