#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <array>
#include <memory>

#include "param_ids.hpp"
#include "channel_mix.hpp"
#include "qwqdsp/convert.hpp"
#include "qwqdsp/filter/rbj.hpp"

static const juce::StringArray kVocoderNames{
    "Burg-LPC",
    "RLS-LPC",
    "STFT-Vocoder",
    "Channel-Vocoder",
};

static const juce::StringArray kChannelVocoderMapNames{
    "linear",
    "mel",
    "log"
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

    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kPreLowpass, 1},
            id::kPreLowpass,
            qwqdsp::convert::Freq2Pitch(1000.0f), 135.1f, 135.0f
        );
        paramListeners_.Add(p, [this](float pitch) {
            juce::ScopedLock lock{getCallbackLock()};
            if (pitch > 135.0f) {
                for (auto& f : pre_lowpass_) {
                    f.Set(1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
                }
            }
            else {
                float const freq = qwqdsp::convert::Pitch2Freq(pitch);
                float const w = qwqdsp::convert::Freq2W(freq, getSampleRate());
                qwqdsp::filter::RBJ design;
                design.Lowpass(w, 0.50979558f);
                pre_lowpass_[0].Set(design.b0, design.b1, design.b2, design.a1, design.a2);
                design.Lowpass(w, 0.60134489f);
                pre_lowpass_[1].Set(design.b0, design.b1, design.b2, design.a1, design.a2);
                design.Lowpass(w, 0.89997622f);
                pre_lowpass_[2].Set(design.b0, design.b1, design.b2, design.a1, design.a2);
                design.Lowpass(w, 2.5629154f);
                pre_lowpass_[3].Set(design.b0, design.b1, design.b2, design.a1, design.a2);
            }
        });
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
            0, 6, 1
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
            (void)b;
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
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kChannelVocoderNBands, 1},
            id::kChannelVocoderNBands,
            juce::NormalisableRange<float>(dsp::ChannelVocoder::kMinOrder, dsp::ChannelVocoder::kMaxOrder, 4),
            16
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
            eChannelVocoderMap_Log
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
            1.0f
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
            dsp::Gain<1>::kMinDb, 40.0f, 0.0f
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
            dsp::Gain<1>::kMinDb, 40.0f, 0.0f
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
            SetLatency();
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
            300.0f, 3000.0f, 500.0f
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
            0.0f, 1.0f, 0.6f
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
            juce::NormalisableRange<float>{0.0f, 1000.0f, 1.0f, 0.4f},
            20.0f
        );
        paramListeners_.Add(p, [this](float bw) {
            juce::ScopedLock lock{getCallbackLock()};
            pitch_glide_.SetSmoothTime(bw, getSampleRate());
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
    rls_lpc_.Init(fs);
    stft_vocoder_.Init(fs);
    channel_vocoder_.Init(fs);

    ensemble_.Init(fs);
    main_gain_.Init(fs, samplesPerBlock);
    side_gain_.Init(fs, samplesPerBlock);
    output_gain_.Init(fs, samplesPerBlock);

    yin_segement_.SetHop(512);
    yin_segement_.SetSize(2048);
    yin_segement_.Reset();
    yin_.Init(sampleRate, 2048);
    pitch_filter_.Reset();
    osc_wpos_ = 0;
    osc_want_write_frac_ = 0;
    pitch_glide_.Reset();
    first_init_ = true;

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
        if (side_ch > 2 && side_ch != 6) {
            side_ch -= 3;
        }
    }

    auto channels = Mix(main_ch, side_ch, buffer);
    auto& main_buffer_ = channels[0];
    auto& side_buffer_ = channels[1];

    auto burg_lp = [this](std::span<float> x) {
        float eb[512]{};
        for (size_t i = 0; i < x.size(); ++i) {
            x[i] = x[i] + noise_.Next() * 1e-5f;
            eb[i] = x[i];
        }
        // poles
        for (int p = 0; p < 2; ++p) {
            float lag{};
            float up{};
            float down{};
            for (size_t i = 0; i < x.size(); ++i) {
                up += x[i] * lag;
                down += x[i] * x[i];
                down += lag * lag;
                lag = eb[i];
            }
            float k = -2.0f * up / down;

            lag = 0;
            for (size_t i = 0; i < x.size(); ++i) {
                float const upgo = x[i] + lag * k;
                float const downgo = lag + x[i] * k;
                lag = eb[i];
                x[i] = upgo;
                eb[i] = downgo;
            }
        }
    };
    if (side_ch == 6) {
        // pitch tracking step
        // 1. yin detect pitch
        // 2. median filter
        // 3. generate waveform

        // burg_lp({temp.data(), num_write});
        yin_segement_.Push(main_buffer_);

        qwqdsp::pitch::FastYin::Result pitch{};
        float const noise_threshold = tracking_noise_->get();
        while (yin_segement_.CanProcess()) {
            yin_.Process(yin_segement_.GetBlock());
            yin_segement_.Advance();

            pitch = yin_.GetPitch();
            float const want_write = yin_segement_.GetHop() + osc_want_write_frac_;
            size_t const iwant = static_cast<size_t>(want_write);
            {
                float t;
                osc_want_write_frac_ = std::modf(want_write, &t);
            }
            size_t const can_write = std::min(osc_buffer_.size() - osc_wpos_, iwant);
            if (pitch.non_period_ratio > noise_threshold) {
                for (size_t i = 0; i < can_write; ++i) {
                    osc_buffer_[osc_wpos_++] = noise_.Next() * pitch.non_period_ratio;
                }
            }
            else {
                pitch = pitch_filter_.Tick(pitch, [](auto const& a, auto const& b) {
                    return a.pitch <=> b.pitch;
                });
                
                [[unlikely]]
                if (first_init_) {
                    pitch_glide_.SetTargetImmediately(pitch.pitch * frequency_mul_);
                }
                else {
                    pitch_glide_.SetTarget(pitch.pitch * frequency_mul_);
                }
                
                if (tracking_waveform_->getIndex() == 0) {
                    for (size_t i = 0; i < can_write; ++i) {
                        tracking_osc_.SetFreq(pitch_glide_.Tick(), getSampleRate());
                        osc_buffer_[osc_wpos_++] = tracking_osc_.Sawtooth();
                    }
                }
                else {
                    for (size_t i = 0; i < can_write; ++i) {
                        tracking_osc_.SetFreq(pitch_glide_.Tick(), getSampleRate());
                        osc_buffer_[osc_wpos_++] = tracking_osc_.PWM();
                    }
                }
            }
        }
        
        size_t const cancopy = std::min(osc_wpos_, side_buffer_.size());
        std::copy_n(osc_buffer_.begin(), cancopy, side_buffer_.begin());
        std::fill(side_buffer_.begin() + cancopy, side_buffer_.end(), float{});
        size_t const drag = osc_wpos_ - cancopy;
        for (size_t i = 0; i < drag; ++i) {
            osc_buffer_[i] = osc_buffer_[i + cancopy];
        }
        osc_wpos_ -= cancopy;
    }

    for (auto& f : main_buffer_) {
        float const out = f - 0.68f * pre_emphasis_;
        pre_emphasis_ = f;
        f = out;
    }
    for (auto& f : pre_lowpass_) {
        for (auto& s : main_buffer_) {
            s = f.Tick(s);
        }
    }

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

    if (latency_.load() != old_latency_) {
        old_latency_ = latency_.load();
        setLatencySamples(old_latency_);
    }

    first_init_ = false;
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
