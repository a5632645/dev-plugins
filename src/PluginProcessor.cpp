#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "dsp/channel_vocoder.hpp"
#include "dsp/ensemble.hpp"
#include "dsp/rls_lpc.hpp"
#include "juce_audio_processors/juce_audio_processors.h"
#include "juce_core/juce_core.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include "param_ids.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <memory>
#include "channel_mix.hpp"

static const juce::StringArray kVocoderNames{
    "Burg-LPC",
    "RLS-LPC",
    "STFT-Vocoder",
    "Channel-Vocoder",
};

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
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

    auto filter_callback = [this](float) {
        juce::ScopedLock lock{getCallbackLock()};
        float st = filter_pitch_->get();
        float freq = std::exp2((st - 69.0f) / 12.0f) * 440.0f;
        filter_.MakeHighShelf(filter_gain_->get(), freq, filter_s_->get());
    };
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kEmphasisGain, 1},
            id::kEmphasisGain,
            0.0f, 40.0f, 20.0f
        );
        filter_gain_ = p.get();
        paramListeners_.Add(p, filter_callback);
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kEmphasisPitch, 1},
            id::kEmphasisPitch,
            0.0f, 135.0f, 80.0f
        );
        filter_pitch_ = p.get();
        paramListeners_.Add(p, filter_callback);
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kEmphasisS, 1},
            id::kEmphasisS,
            0.5f, 1.0f, 0.707f
        );
        filter_s_ = p.get();
        paramListeners_.Add(p, filter_callback);
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{id::kMainChannelConfig, 1},
            id::kMainChannelConfig,
            0, 5, 0
        );
        main_channel_config_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{id::kSideChannelConfig, 1},
            id::kSideChannelConfig,
            0, 5, 1
        );
        side_channel_config_ = p.get();
        layout.add(std::move(p));
    }

    // vocoder type
    {
        auto p = std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{id::kVocoderType, 1},
            id::kVocoderType,
            kVocoderNames,
            0
        );
        vocoder_type_param_ = p.get();
        paramListeners_.Add(p, [this](int i) {
            (void)i;
            juce::ScopedLock lock{getCallbackLock()};
            SetLatency(); 
        });
        layout.add(std::move(p));
    }

    // pitch shifter
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kShiftPitch, 1},
            id::kShiftPitch,
            -12.0f, 12.0f, 0.0f
        );
        paramListeners_.Add(p, [this](float l) {
            juce::ScopedLock lock{getCallbackLock()};
            shifter_.SetPitchShift(l);
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{id::kEnableShifter, 1},
            id::kEnableShifter,
            false
        );
        shifter_enabled_ = p.get();
        paramListeners_.Add(p, [this](bool b) {
            juce::ScopedLock lock{getCallbackLock()};
            SetLatency();
        });
        layout.add(std::move(p));
    }

    // channel vocoder
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kChannelVocoderAttack, 1},
            id::kChannelVocoderAttack,
            1.0f, 1000.0f, 1.0f
        );
        paramListeners_.Add(p, [this](float v) {
            juce::ScopedLock _{ getCallbackLock() };
            channel_vocoder_.SetAttack(v);
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kChannelVocoderRelease, 1},
            id::kChannelVocoderRelease,
            10.0f, 1000.0f, 150.0f
        );
        paramListeners_.Add(p, [this](float v) {
            juce::ScopedLock _{ getCallbackLock() };
            channel_vocoder_.SetRelease(v);
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kChannelVocoderFreqBegin, 1},
            id::kChannelVocoderFreqBegin,
            20.0f, 2000.0f, 40.0f
        );
        paramListeners_.Add(p, [this](float v) {
            juce::ScopedLock _{ getCallbackLock() };
            channel_vocoder_.SetFreqBegin(v);
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kChannelVocoderFreqEnd, 1},
            id::kChannelVocoderFreqEnd,
            4000.0f, 18000.0f, 12000.0f
        );
        paramListeners_.Add(p, [this](float v) {
            juce::ScopedLock _{ getCallbackLock() };
            channel_vocoder_.SetFreqEnd(v);
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{id::kChannelVocoderNBands, 1},
            id::kChannelVocoderNBands,
            4, dsp::ChannelVocoder::kMaxOrder, 16
        );
        paramListeners_.Add(p, [this](int v) {
            juce::ScopedLock _{ getCallbackLock() };
            channel_vocoder_.SetNumBands(v);
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kChannelVocoderScale, 1},
            id::kChannelVocoderScale,
            0.25f, 2.0f, 1.0f
        );
        paramListeners_.Add(p, [this](float v) {
            juce::ScopedLock _{ getCallbackLock() };
            channel_vocoder_.SetModulatorScale(v);
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kChannelVocoderCarryScale, 1},
            id::kChannelVocoderCarryScale,
            0.25f, 2.0f, 1.0f
        );
        paramListeners_.Add(p, [this](float v) {
            juce::ScopedLock _{ getCallbackLock() };
            channel_vocoder_.SetCarryScale(v);
        });
        layout.add(std::move(p));
    }
    
    // lpc
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kForgetRate, 1},
            id::kForgetRate,
            juce::NormalisableRange<float>{5.0f, 200.0f, 1.0f, 0.4f},
            20.0f
        );
        paramListeners_.Add(p, [this](float l) {
            juce::ScopedLock lock{getCallbackLock()};
            burg_lpc_.SetForget(l);
            rls_lpc_.SetForgetRate(l);
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kLPCSmooth, 1},
            id::kLPCSmooth,
            juce::NormalisableRange<float>{0.1f, 50.0f, 0.1f, 0.4f},
            0.1f
        );
        paramListeners_.Add(p, [this](float l) {
            juce::ScopedLock lock{getCallbackLock()};
            burg_lpc_.SetSmooth(l);
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kLPCGainAttack, 1},
            id::kLPCGainAttack,
            juce::NormalisableRange<float>{1.0f, 100.0f, 1.0f, 0.4f},
            20.0f
        );
        paramListeners_.Add(p, [this](float l) {
            juce::ScopedLock lock{getCallbackLock()};
            burg_lpc_.SetGainAttack(l);
            rls_lpc_.SetGainAttack(l);
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kLPCGainRelease, 1},
            id::kLPCGainRelease,
            juce::NormalisableRange<float>{5.0f, 200.0f, 1.0f, 0.4f},
            20.0f
        );
        paramListeners_.Add(p, [this](float l) {
            juce::ScopedLock lock{getCallbackLock()};
            burg_lpc_.SetGainRelease(l);
            rls_lpc_.SetGainRelease(l);
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{id::kLPCDicimate, 1},
            id::kLPCDicimate,
            1, 8, 1
        );
        paramListeners_.Add(p, [this](int l) {
            juce::ScopedLock lock{getCallbackLock()};
            burg_lpc_.SetDicimate(l);
            rls_lpc_.SetDicimate(l);
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{id::kLPCOrder, 1},
            id::kLPCOrder,
            2, dsp::BurgLPC::kNumPoles, 35
        );
        paramListeners_.Add(p, [this](int order) {
            juce::ScopedLock lock{getCallbackLock()};
            burg_lpc_.SetLPCOrder(order);
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{id::kRLSLPCOrder, 1},
            id::kRLSLPCOrder,
            0, dsp::RLSLPC::kNumLPC - 1, 2,
            juce::AudioParameterIntAttributes{}.withStringFromValueFunction(
                [](int select_idx, int b) {
                    (void)b;
                    int order = dsp::RLSLPC::kLPCOrders[select_idx];
                    return juce::String{order};
                }
            )
        );
        paramListeners_.Add(p, [this](int order) {
            juce::ScopedLock lock{getCallbackLock()};
            rls_lpc_.SetOrder(dsp::RLSLPC::kLPCOrders[order]);
        });
        layout.add(std::move(p));
    }

    // gain
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kMainGain, 1},
            id::kMainGain,
            dsp::Gain<1>::kMinDb, 20.0f, 0.0f
        );
        paramListeners_.Add(p, [this](float bw) {
            juce::ScopedLock lock{getCallbackLock()};
            main_gain_.SetGain(bw);
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kSideGain, 1},
            id::kSideGain,
            dsp::Gain<1>::kMinDb, 20.0f, 0.0f
        );
        paramListeners_.Add(p, [this](float bw) {
            juce::ScopedLock lock{getCallbackLock()};
            side_gain_.SetGain(bw);
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kOutputgain, 1},
            id::kOutputgain,
            dsp::Gain<1>::kMinDb, 40.0f, 0.0f
        );
        paramListeners_.Add(p, [this](float bw) {
            juce::ScopedLock lock{getCallbackLock()};
            output_gain_.SetGain(bw);
        });
        layout.add(std::move(p));
    }

    // stft
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kStftWindowWidth, 1},
            id::kStftWindowWidth,
            0.0f, 5.0f, 2.0f
        );
        paramListeners_.Add(p, [this](float bw) {
            juce::ScopedLock lock{getCallbackLock()};
            stft_vocoder_.SetBandwidth(bw);
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kStftRelease, 1},
            id::kStftRelease,
            juce::NormalisableRange<float>{1.0f, 1000.0f, 1.0f, 0.4f},
            100.0f
        );
        paramListeners_.Add(p, [this](float bw) {
            juce::ScopedLock lock{getCallbackLock()};
            stft_vocoder_.SetRelease(bw);
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kStftAttack, 1},
            id::kStftAttack,
            juce::NormalisableRange<float>{1.0f, 1000.0f, 1.0f, 0.4f},
            20.0f
        );
        paramListeners_.Add(p, [this](float bw) {
            juce::ScopedLock lock{getCallbackLock()};
            stft_vocoder_.SetAttack(bw);
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kStftBlend, 1},
            id::kStftBlend,
            0.0f, 0.99f, 0.2f
        );
        paramListeners_.Add(p, [this](float omega) {
            juce::ScopedLock lock{getCallbackLock()};
            stft_vocoder_.SetBlend(omega);
        });
        layout.add(std::move(p));
    }

    // ensemble
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kEnsembleDetune, 1},
            id::kEnsembleDetune,
            0.05f, dsp::Ensemble::kMaxSemitone, 0.1f
        );
        paramListeners_.Add(p, [this](float detune) {
            juce::ScopedLock lock{getCallbackLock()};
            ensemble_.SetDetune(detune);
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kEnsembleMix, 1},
            id::kEnsembleMix,
            0.0f, 1.0f, 0.5f
        );
        paramListeners_.Add(p, [this](float mix) {
            juce::ScopedLock lock{getCallbackLock()};
            ensemble_.SetMix(mix);
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{id::kEnsembleNumVoices, 1},
            id::kEnsembleNumVoices,
            2, dsp::Ensemble::kMaxVoices, 8
        );
        paramListeners_.Add(p, [this](int nvocice) {
            juce::ScopedLock lock{getCallbackLock()};
            ensemble_.SetNumVoices(nvocice);
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kEnsembleSpread, 1},
            id::kEnsembleSpread,
            0.0f, 1.0f, 1.0f
        );
        paramListeners_.Add(p, [this](float spread) {
            juce::ScopedLock lock{getCallbackLock()};
            ensemble_.SetSperead(spread);
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kEnsembleRate, 1},
            id::kEnsembleRate,
            juce::NormalisableRange<float>(dsp::Ensemble::kMinFrequency, 1.0f, 0.01f, 0.4f),
            0.2f
        );
        paramListeners_.Add(p, [this](float rate) {
            juce::ScopedLock lock{getCallbackLock()};
            ensemble_.SetRate(rate);
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{id::kEnsembleMode, 1},
            id::kEnsembleMode,
            juce::StringArray{
                "sine",
                "noise"
            },
            1
        );
        paramListeners_.Add(p, [this](int mode) {
            juce::ScopedLock _{ getCallbackLock() };
            ensemble_.SetMode(static_cast<dsp::Ensemble::Mode>(mode));
        });
        layout.add(std::move(p));
    }

    value_tree_ = std::make_unique<juce::AudioProcessorValueTreeState>(*this, nullptr, "PARAMETERS", std::move(layout));
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
    value_tree_ = nullptr;
}

//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AudioPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String AudioPluginAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    float fs = static_cast<float>(sampleRate);
    burg_lpc_.Init(fs);
    filter_.Init(fs);
    rls_lpc_.Init(fs);
    stft_vocoder_.Init(fs);
    channel_vocoder_.Init(fs);

    ensemble_.Init(fs);
    main_gain_.Init(fs, samplesPerBlock);
    side_gain_.Init(fs, samplesPerBlock);
    output_gain_.Init(fs, samplesPerBlock);

    paramListeners_.CallAll();
}

void AudioPluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    int main_ch = main_channel_config_->get();
    int side_ch = side_channel_config_->get();
    if (buffer.getNumChannels() < 4) {
        // 无侧链，侧链声道映射为主声道
        if (main_ch > 2) {
            main_ch -= 3;
        }
        if (side_ch > 2) {
            side_ch -= 3;
        }
    }

    auto channels = Mix(main_ch, side_ch, buffer);
    auto& main_buffer_ = channels[0];
    auto& side_buffer_ = channels[1];

    filter_.Process(main_buffer_);
    if (shifter_enabled_->get()) {
        shifter_.Process(main_buffer_);
    }
    main_gain_.Process(main_buffer_);
    side_gain_.Process(side_buffer_);

    switch (vocoder_type_param_->getIndex()) {
    case eVocoderType_BurgLPC:
        burg_lpc_.Process(main_buffer_, side_buffer_);
        break;
    case eVocoderType_RLSLPC:
        rls_lpc_.Process(main_buffer_, side_buffer_);
        break;
    case eVocoderType_STFTVocoder:
        stft_vocoder_.Process(main_buffer_, side_buffer_);
        break;
    case eVocoderType_ChannelVocoder:
        channel_vocoder_.ProcessBlock(main_buffer_, side_buffer_);
        break;
    default:
        jassertfalse;
        break;
    }

    ensemble_.Process(main_buffer_, side_buffer_);
    output_gain_.Process(channels);
}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor (*this);
}

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    (void)destData;
}

void AudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    (void)data;
    (void)sizeInBytes;
}

void AudioPluginAudioProcessor::Panic() {
    const juce::ScopedLock lock{ getCallbackLock() };
}

void AudioPluginAudioProcessor::SetLatency() {
    int latency = 0;
    switch (vocoder_type_param_->getIndex()) {
    case eVocoderType_STFTVocoder:
        latency += stft_vocoder_.kFFTSize;
        break;
    default:
        break;
    }

    if (shifter_enabled_->get()) {
        latency += shifter_.kNumDelay / 2;
    }
    setLatencySamples(latency);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}
