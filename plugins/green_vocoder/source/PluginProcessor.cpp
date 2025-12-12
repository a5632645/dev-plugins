#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <array>
#include <memory>

#include "param_ids.hpp"

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

    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kPreTilt, 1},
            id::kPreTilt,
            0.0f, 20.0f, 10.0f
        );
        paramListeners_.Add(p, [this](float db) {
            juce::ScopedLock lock{getCallbackLock()};
            pre_tilt_filter_.SetTilt(static_cast<float>(getSampleRate()), db);
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{id::kMainChannelConfig, 1},
            id::kMainChannelConfig,
            juce::StringArray{
                "Main Left", "Main Right", "Side Left", "Side Right",
            }, 0
        );
        main_channel_config_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{id::kSideChannelConfig, 1},
            id::kSideChannelConfig,
            juce::StringArray{
                "Main", "Side", "Pitch"
            }, 1
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
            channel_vocoder_.SetFormantShift(l);
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
            (void)b;
            juce::ScopedLock lock{getCallbackLock()};
            SetLatency();
        });
        layout.add(std::move(p));
    }

    // channel vocoder
    {
        auto p = std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{id::kChannelVocoderFilterBankMode, 1},
            id::kChannelVocoderFilterBankMode,
            juce::StringArray{
                "stack butterworth 12",
                "stack butterworth 24",
                "flat butterworth 12",
                "flat butterworth 24",
                "chebyshev 12",
                "chebyshev 24"
            }, 1
        );
        paramListeners_.Add(p, [this](int mode) {
            juce::ScopedLock _{ getCallbackLock() };
            channel_vocoder_.SetFilterBankMode(static_cast<green_vocoder::dsp::ChannelVocoder::FilterBankMode>(mode));
        });
        layout.add(std::move(p));
    }
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
            juce::ParameterID{id::kChannelVocoderGate, 1},
            id::kChannelVocoderGate,
            -100.0f, 20.0f, -100.0f
        );
        paramListeners_.Add(p, [this](float v) {
            juce::ScopedLock _{ getCallbackLock() };
            channel_vocoder_.SetGate(v);
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
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kChannelVocoderNBands, 1},
            id::kChannelVocoderNBands,
            juce::NormalisableRange<float>(green_vocoder::dsp::ChannelVocoder::kMinOrder, green_vocoder::dsp::ChannelVocoder::kMaxOrder, 4),
            36
        );
        paramListeners_.Add(p, [this](float v) {
            juce::ScopedLock _{ getCallbackLock() };
            channel_vocoder_.SetNumBands(static_cast<int>(std::round(v)));
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kChannelVocoderScale, 1},
            id::kChannelVocoderScale,
            0.1f, 2.0f, 1.0f
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
            0.1f, 2.0f, 1.0f
        );
        paramListeners_.Add(p, [this](float v) {
            juce::ScopedLock _{ getCallbackLock() };
            channel_vocoder_.SetCarryScale(v);
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{id::kChannelVocoderMap, 1},
            id::kChannelVocoderMap,
            kChannelVocoderMapNames,
            eChannelVocoderMap_Mel
        );
        paramListeners_.Add(p, [this](int i) {
            juce::ScopedLock lock{getCallbackLock()};
            channel_vocoder_.SetMap(static_cast<eChannelVocoderMap>(i));
        });
        layout.add(std::move(p));
    }
    
    // lpc
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kForgetRate, 1},
            id::kForgetRate,
            juce::NormalisableRange<float>{5.0f, 200.0f, 1.0f, 0.4f},
            10.0f
        );
        paramListeners_.Add(p, [this](float l) {
            juce::ScopedLock lock{getCallbackLock()};
            burg_lpc_.SetForget(l);
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kLPCSmooth, 1},
            id::kLPCSmooth,
            juce::NormalisableRange<float>{1.0f, 50.0f, 0.1f, 0.4f},
            1.0f
        );
        paramListeners_.Add(p, [this](float l) {
            juce::ScopedLock lock{getCallbackLock()};
            burg_lpc_.SetSmooth(l);
            block_burg_lpc_.SetSmear(l);
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kLPCGainAttack, 1},
            id::kLPCGainAttack,
            juce::NormalisableRange<float>{1.0f, 100.0f, 1.0f, 0.4f},
            10.0f
        );
        paramListeners_.Add(p, [this](float l) {
            juce::ScopedLock lock{getCallbackLock()};
            burg_lpc_.SetGainAttack(l);
            block_burg_lpc_.SetAttack(l);
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
            block_burg_lpc_.SetRelease(l);
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{id::kLPCDicimate, 1},
            id::kLPCDicimate,
            juce::StringArray{
                "Modern", "Legacy", "Telephone"
            }, 0
        );
        paramListeners_.Add(p, [this](int l) {
            juce::ScopedLock lock{getCallbackLock()};
            burg_lpc_.SetQuality(static_cast<green_vocoder::dsp::LeakyBurgLPC::Quality>(l));
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kLPCOrder, 1},
            id::kLPCOrder,
            juce::NormalisableRange<float>{
                4.0f, green_vocoder::dsp::LeakyBurgLPC::kNumPoles, 4.0f
            }, 36.0f
        );
        paramListeners_.Add(p, [this](float order) {
            juce::ScopedLock lock{getCallbackLock()};
            burg_lpc_.SetLPCOrder(static_cast<int>(order));
            block_burg_lpc_.SetPoles(static_cast<size_t>(order));
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
            juce::ParameterID{id::kMfccNumBands, 1},
            id::kMfccNumBands,
            juce::NormalisableRange<float>{
                green_vocoder::dsp::MFCCVocoder::kMinNumMfcc,
                green_vocoder::dsp::MFCCVocoder::kMaxNumMfcc,
                4.0f
            }, 20.0f
        );
        paramListeners_.Add(p, [this](float bw) {
            juce::ScopedLock lock{getCallbackLock()};
            mfcc_vocoder_.SetNumMfcc(static_cast<size_t>(bw));
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
            mfcc_vocoder_.SetRelease(bw);
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kStftAttack, 1},
            id::kStftAttack,
            juce::NormalisableRange<float>{1.0f, 1000.0f, 1.0f, 0.4f},
            1.0f
        );
        paramListeners_.Add(p, [this](float bw) {
            juce::ScopedLock lock{getCallbackLock()};
            stft_vocoder_.SetAttack(bw);
            mfcc_vocoder_.SetAttack(bw);
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
    {
        auto p = std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{id::kStftSize, 1},
            id::kStftSize,
            juce::StringArray{
                "256",
                "512",
                "1024",
                "2048",
                "4096"
            },
            2
        );
        paramListeners_.Add(p, [this](int idx) {
            juce::ScopedLock lock{getCallbackLock()};
            static constexpr std::array kArray{
                256, 512, 1024, 2048, 4096
            };
            stft_vocoder_.SetFFTSize(kArray[idx]);
            mfcc_vocoder_.SetFFTSize(kArray[idx]);
            block_burg_lpc_.SetBlockSize(kArray[idx]);
            SetLatency();
        });
        layout.add(std::move(p));
    }

    // ensemble
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kEnsembleDetune, 1},
            id::kEnsembleDetune,
            0.01f, green_vocoder::dsp::Ensemble::kMaxSemitone, 0.05f
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
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kEnsembleNumVoices, 1},
            id::kEnsembleNumVoices,
            juce::NormalisableRange<float>{4, green_vocoder::dsp::Ensemble::kMaxVoices, 4.0f},
            8
        );
        paramListeners_.Add(p, [this](float nvocice) {
            juce::ScopedLock lock{getCallbackLock()};
            ensemble_.SetNumVoices(static_cast<int>(nvocice));
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
            juce::NormalisableRange<float>(green_vocoder::dsp::Ensemble::kMinFrequency, 1.0f, 0.01f),
            0.1f
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
            0
        );
        paramListeners_.Add(p, [this](int mode) {
            juce::ScopedLock _{ getCallbackLock() };
            ensemble_.SetMode(static_cast<green_vocoder::dsp::Ensemble::Mode>(mode));
        });
        layout.add(std::move(p));
    }

    // pitch tracking
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kTrackingLow, 1},
            id::kTrackingLow,
            20.0f, 300.0f, 80.0f
        );
        paramListeners_.Add(p, [this](float low) {
            juce::ScopedLock lock{getCallbackLock()};
            yin_.SetMinPitch(low);
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kTrackingHigh, 1},
            id::kTrackingHigh,
            300.0f, 800.0f, 500.0f
        );
        paramListeners_.Add(p, [this](float max) {
            juce::ScopedLock lock{getCallbackLock()};
            yin_.SetMaxPitch(max);
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kTrackingPitch, 1},
            id::kTrackingPitch,
            -36.0f, 36.0f, 0.0f
        );
        paramListeners_.Add(p, [this](float pitch) {
            juce::ScopedLock lock{getCallbackLock()};
            frequency_mul_ = std::exp2(pitch / 12.0f);
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kTrackingPwm, 1},
            id::kTrackingPwm,
            0.01f, 0.99f, 0.5f
        );
        paramListeners_.Add(p, [this](float pwm) {
            juce::ScopedLock lock{getCallbackLock()};
            tracking_osc_.SetPWM(pwm);
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kTrackingNoise, 1},
            id::kTrackingNoise,
            0.0f, 1.0f, 0.5f
        );
        tracking_noise_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{id::kTrackingWaveform, 1},
            id::kTrackingWaveform,
            juce::StringArray{
                "saw",
                "pwm"
            },
            0
        );
        tracking_waveform_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kTrackingGlide, 1},
            id::kTrackingGlide,
            juce::NormalisableRange<float>{1.0f, 1000.0f, 1.0f, 0.4f},
            1.0f
        );
        paramListeners_.Add(p, [this](float bw) {
            juce::ScopedLock lock{getCallbackLock()};
            pitch_glide_.MakeFilter(bw * getSampleRate() / 1000.0f);
        });
        layout.add(std::move(p));
    }

    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"output_drive", 1},
            "output_drive",
            -40.0f, 40.0f, 0.0f
        );
        output_drive_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{"output_saturation", 1},
            "output_saturation",
            false
        );
        output_saturation_ = p.get();
        layout.add(std::move(p));
    }

    value_tree_ = std::make_unique<juce::AudioProcessorValueTreeState>(*this, nullptr, "PARAMETERS", std::move(layout));
    preset_manager_ = std::make_unique<pluginshared::PresetManager>(*value_tree_, *this);
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
    size_t block_size = static_cast<size_t>(samplesPerBlock);

    crossing_main_buffer_.resize(block_size);
    crossing_side_buffer_.resize(block_size);
    burg_lpc_.Init(fs, block_size);
    stft_vocoder_.Init(fs);
    mfcc_vocoder_.Init(fs);
    channel_vocoder_.Init(fs, block_size);
    ensemble_.Init(fs);
    block_burg_lpc_.Init(fs);

    yin_segement_.SetHop(1024);
    yin_segement_.SetSize(2048);
    yin_segement_.Reset();
    yin_.Init(static_cast<float>(sampleRate), 2048);
    osc_wpos_ = 0;
    pitch_glide_.Reset();
    first_init_ = true;

    pre_tilt_filter_.Reset();
    output_driver_.Reset();

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
    std::ignore = midiMessages;
    juce::ScopedNoDenormals noDenormals;

    int main_ch = main_channel_config_->getIndex();
    int side_ch = side_channel_config_->getIndex();
    if (buffer.getNumChannels() < 4) {
        // 无侧链，侧链声道映射为主声道
        if (main_ch >= 2) {
            main_ch -= 2;
        }
        if (side_ch == 1) {
            side_ch = 0;
        }
    }
    float* pitch_buffer = buffer.getWritePointer(main_ch);
    size_t const num_samples = static_cast<size_t>(buffer.getNumSamples());

    if (side_ch == 2) {
        yin_segement_.Push({pitch_buffer, num_samples});

        float const noise_gain = tracking_noise_->get();
        while (yin_segement_.CanProcess()) {
            yin_.Process(yin_segement_.GetBlock());
            yin_segement_.Advance();

            // get pitch
            auto pitch = yin_.GetPitch();
            size_t const iwant = static_cast<size_t>(yin_segement_.GetHop());
            size_t const can_write = std::min(osc_buffer_.size() - osc_wpos_, iwant);

            float target_pitch = pitch.pitch_hz * frequency_mul_;
            target_pitch = std::max(target_pitch, 0.1f);

            // fill trival wave
            float curr_trival_wave_gain = last_osc_mix_;
            float const delta_trival_wave_gain = (1.0f - pitch.non_period_ratio - curr_trival_wave_gain) / static_cast<float>(can_write);
            size_t osc_wpos = osc_wpos_;
            if (tracking_waveform_->getIndex() == 0) {
                for (size_t i = 0; i < can_write; ++i) {
                    curr_trival_wave_gain += delta_trival_wave_gain;
                    tracking_osc_.SetFreq(pitch_glide_.Tick(target_pitch), getSampleRate());
                    osc_buffer_[osc_wpos++] = tracking_osc_.Sawtooth() * curr_trival_wave_gain;
                }
            }
            else {
                for (size_t i = 0; i < can_write; ++i) {
                    curr_trival_wave_gain += delta_trival_wave_gain;
                    tracking_osc_.SetFreq(pitch_glide_.Tick(target_pitch), getSampleRate());
                    osc_buffer_[osc_wpos++] = tracking_osc_.PWM_NoDC() * curr_trival_wave_gain;
                }
            }
            last_osc_mix_ = 1.0f - pitch.non_period_ratio;

            // add noise
            float curr_noise_gain = last_noise_mix_;
            float target_noise_gain = pitch.non_period_ratio * noise_gain;
            float delta_noise_gain = (target_noise_gain - curr_noise_gain) / static_cast<float>(can_write);
            for (size_t i = 0; i < can_write; ++i) {
                curr_noise_gain += delta_noise_gain;
                osc_buffer_[osc_wpos_++] += noise_.Next() * curr_noise_gain;
            }
            last_noise_mix_ = target_noise_gain;
        }
        
        size_t const cancopy = std::min(osc_wpos_, num_samples);
        for (size_t i = 0; i < cancopy; ++i) {
            crossing_side_buffer_[i] = {osc_buffer_[i], osc_buffer_[i]};
        }
        for (size_t i = cancopy; i < num_samples; ++i) {
            crossing_side_buffer_[i].Broadcast(0);
        }

        size_t const drag = osc_wpos_ - cancopy;
        for (size_t i = 0; i < drag; ++i) {
            osc_buffer_[i] = osc_buffer_[i + cancopy];
        }
        osc_wpos_ -= cancopy;
    }

#if 0
    // crossing left and right buffer
    // if main_ch == 0 or 1, modulator is 0 and 1
    // if main_ch == 2 or 3, modulator is 2 and 3
    float const* modu_left = buffer.getReadPointer(0);
    float const* modu_right = buffer.getReadPointer(1);
    if (main_ch == 2 || main_ch == 3) {
        modu_left = buffer.getReadPointer(2);
        modu_right = buffer.getReadPointer(3);
    }
    for (size_t i = 0; i < num_samples; ++i) {
        crossing_main_buffer_[i] = {modu_left[i], modu_right[i]};
    }

    // if side_ch == 0, carry is 0 and 1
    // if side_ch == 1, carry is 2 and 3
    // if side_ch == 2, cross_carry_buffer has been filled by pitch tracking oscillator
    if (side_ch == 1) {
        float const* side_left = buffer.getReadPointer(2);
        float const* side_right = buffer.getReadPointer(3);
        for (size_t i = 0; i < num_samples; ++i) {
            crossing_side_buffer_[i] = {side_left[i], side_right[i]};
        }
    }
    else if (side_ch == 0) {
        // copy
        std::copy_n(crossing_main_buffer_.begin(), num_samples, crossing_side_buffer_.begin());
    }
#else
    // NOTE: Debug test code
    float const* modu = buffer.getReadPointer(0);
    float const* carry = buffer.getReadPointer(1);
    if (side_ch != 2) {
        for (size_t i = 0; i < num_samples; ++i) {
            crossing_main_buffer_[i].Broadcast(modu[i]);
            crossing_side_buffer_[i].Broadcast(carry[i]);
        }
    }
    else {
        for (size_t i = 0; i < num_samples; ++i) {
            crossing_main_buffer_[i].Broadcast(modu[i]);
        }
    }
#endif
    
    // tilt->shifter sounds harsh, shifter->tilt is ok
    if (shifter_enabled_->get()) {
        // shifter_.Process(crossing_main_buffer_);
    }
    for (auto& f : crossing_main_buffer_) {
        f = pre_tilt_filter_.Tick(f);
    }

    switch (vocoder_type_param_->getIndex()) {
    case eVocoderType_LeakyBurgLPC:
        burg_lpc_.Process(crossing_main_buffer_, crossing_side_buffer_);
        break;
    case eVocoderType_STFTVocoder:
        stft_vocoder_.Process(crossing_main_buffer_.data(), crossing_side_buffer_.data(), num_samples);
        break;
    case eVocoderType_MFCCVocoder:
        mfcc_vocoder_.Process(crossing_main_buffer_.data(), crossing_side_buffer_.data(), num_samples);
        break;
    case eVocoderType_ChannelVocoder:
        channel_vocoder_.ProcessBlock(crossing_main_buffer_.data(), crossing_side_buffer_.data(), num_samples);
        break;
    case eVocoderType_BlockBurgLPC:
        block_burg_lpc_.Process(crossing_main_buffer_.data(), crossing_side_buffer_.data(), num_samples);
        break;
    default:
        jassertfalse;
        break;
    }

    float* main_left = buffer.getWritePointer(0);
    float* main_right = buffer.getWritePointer(1);
    float const output_gain = qwqdsp::convert::Db2Gain(output_drive_->get());
    if (!output_saturation_->get()) {
        ensemble_.Process(crossing_main_buffer_.data(), num_samples);
        for (size_t i = 0; i < num_samples; ++i) {
            auto x = crossing_main_buffer_[i] * output_gain;
            main_left[i] = x[0];
            main_right[i] = x[1];
        }
    }
    else {
        for (size_t i = 0; i < num_samples; ++i) {
            crossing_main_buffer_[i] = output_driver_.ADAA(crossing_main_buffer_[i] * output_gain);
        }
        ensemble_.Process(crossing_main_buffer_.data(), num_samples);
        for (size_t i = 0; i < num_samples; ++i) {
            auto const& x = crossing_main_buffer_[i];
            main_left[i] = x[0];
            main_right[i] = x[1];
        }
    }

    if (latency_.load() != old_latency_) {
        old_latency_ = latency_.load();
        setLatencySamples(old_latency_);
    }

    // first_init_ = false;
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
    if (auto state = value_tree_->copyState().createXml()) {
        copyXmlToBinary(*state, destData);
    }
}

void AudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto state = juce::ValueTree::fromXml(*getXmlFromBinary(data, sizeInBytes));
    if (state.isValid()) {
        value_tree_->replaceState(state);
    }
}

void AudioPluginAudioProcessor::Panic() {
    const juce::ScopedLock lock{ getCallbackLock() };
}

void AudioPluginAudioProcessor::SetLatency() {
    int latency = 0;
    switch (vocoder_type_param_->getIndex()) {
    case eVocoderType_STFTVocoder:
    case eVocoderType_MFCCVocoder:
    case eVocoderType_BlockBurgLPC:
        if (stft_vocoder_.GetFFTSize() > getBlockSize()) {
            latency += stft_vocoder_.GetFFTSize();
        }
        break;
    default:
        break;
    }

    if (shifter_enabled_->get()) {
        latency += shifter_.kNumDelay / 2;
    }
    // setLatencySamples(latency);
    latency_.store(latency);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}
