#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <array>
#include <memory>

#include "global.hpp"
#include "param_ids.hpp"

#if BUILD_IN_CI
#define I_AM_USING_LOOPBACK_DEBUG 0
#else
#define I_AM_USING_LOOPBACK_DEBUG 1
#endif

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

    {
        auto p = std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id::kPreTilt, 1}, id::kPreTilt, 0.0f,
                                                             20.0f, 10.0f);
        paramListeners_.Add(p, [this](float db) { pre_tilt_filter_.SetTilt(static_cast<float>(getSampleRate()), db); });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterBool>(juce::ParameterID{id::kChannelSwap, 1},
                                                            id::kChannelSwap,
                                                            false);
        channel_swap_ = p.get();
        paramListeners_.Add(p, [](bool) {});
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{id::kPitchChannel, 1},
                                                              id::kPitchChannel,
                                                              juce::StringArray{"Off", "Main L", "Main R", "Side L", "Side R"}, 0);
        pitch_channel_ = p.get();
        layout.add(std::move(p));
    }

    // vocoder type
    {
        auto p = std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{id::kVocoderType, 1}, id::kVocoderType,
                                                              kVocoderNames, 2);
        vocoder_type_param_ = p.get();
        paramListeners_.Add(p, [this](int i) {
            (void)i;

            SetLatency();
        });
        layout.add(std::move(p));
    }

    // pitch shifter
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id::kShiftPitch, 1}, id::kShiftPitch,
                                                             -24.0f, 24.0f, 0.0f);
        paramListeners_.Add(p, [this](float l) {
            channel_vocoder_.SetFormantShift(l);
            stft_vocoder_.SetFormantShift(l);
            // scale the formant ratio to almost what other vocoders sound like for LPC vocoders
            l *= (16.0f / 24.0f);
            block_burg_lpc_.SetFormantShift(l / 24.0f);
            burg_lpc_.SetFormantShift(l / 24.0f);
        });
        layout.add(std::move(p));
    }

    // channel vocoder
    {
        auto p = std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{id::kChannelVocoderFilterBankMode, 1}, id::kChannelVocoderFilterBankMode,
            juce::StringArray{"bandpass 12", "stack butterworth 24", "stack butterworth 36", "flat butterworth 24",
                              "flat butterworth 36", "chebyshev 24", "chebyshev 36", "Elliptic 24", "Elliptic 36"},
            1);
        paramListeners_.Add(p, [this](int mode) {
            channel_vocoder_.SetFilterBankMode(static_cast<green_vocoder::dsp::ChannelVocoder::FilterBankMode>(mode));
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id::kChannelVocoderAttack, 1},
                                                             id::kChannelVocoderAttack, 1.0f, 1000.0f, 10.0f);
        paramListeners_.Add(p, [this](float v) { channel_vocoder_.SetAttack(v); });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id::kChannelVocoderGate, 1},
                                                             id::kChannelVocoderGate, -100.0f, 20.0f, -100.0f);
        paramListeners_.Add(p, [this](float v) { channel_vocoder_.SetGate(v); });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kChannelVocoderRelease, 1}, id::kChannelVocoderRelease,
            juce::NormalisableRange<float>{10.0f, 32000.0f, 1.0f, 0.4f}, 150.0f);
        paramListeners_.Add(p, [this](float v) { channel_vocoder_.SetRelease(v); });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id::kChannelVocoderFreqBegin, 1},
                                                             id::kChannelVocoderFreqBegin, 20.0f, 2000.0f, 40.0f);
        paramListeners_.Add(p, [this](float v) { channel_vocoder_.SetFreqBegin(v); });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id::kChannelVocoderFreqEnd, 1},
                                                             id::kChannelVocoderFreqEnd, 4000.0f, 18000.0f, 12000.0f);
        paramListeners_.Add(p, [this](float v) { channel_vocoder_.SetFreqEnd(v); });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kChannelVocoderNBands, 1}, id::kChannelVocoderNBands,
            juce::NormalisableRange<float>(green_vocoder::dsp::ChannelVocoder::kMinOrder,
                                           green_vocoder::dsp::ChannelVocoder::kMaxOrder, 4),
            20);
        paramListeners_.Add(p, [this](float v) { channel_vocoder_.SetNumBands(static_cast<int>(std::round(v))); });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id::kChannelVocoderScale, 1},
                                                             id::kChannelVocoderScale, 0.1f, 2.0f, 1.0f);
        paramListeners_.Add(p, [this](float v) { channel_vocoder_.SetModulatorScale(v); });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id::kChannelVocoderRipple, 1},
                                                             id::kChannelVocoderRipple, 0.1f, 10.0f, 1.0f);
        paramListeners_.Add(p, [this](float v) { channel_vocoder_.SetFilterRipple(v); });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id::kChannelVocoderCarryScale, 1},
                                                             id::kChannelVocoderCarryScale, 0.1f, 2.0f, 1.0f);
        paramListeners_.Add(p, [this](float v) { channel_vocoder_.SetCarryScale(v); });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{id::kChannelVocoderMap, 1},
                                                              id::kChannelVocoderMap, kChannelVocoderMapNames,
                                                              eChannelVocoderMap_Mel);
        paramListeners_.Add(p, [this](int i) { channel_vocoder_.SetMap(static_cast<eChannelVocoderMap>(i)); });
        layout.add(std::move(p));
    }

    // lpc
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id::kForgetRate, 1}, id::kForgetRate,
                                                             juce::NormalisableRange<float>{5.0f, 200.0f, 1.0f, 0.4f},
                                                             10.0f);
        paramListeners_.Add(p, [this](float l) { burg_lpc_.SetForget(l); });
        layout.add(std::move(p));
    }
    {
        auto p =
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id::kLPCSmooth, 1}, id::kLPCSmooth,
                                                        juce::NormalisableRange<float>{0.0f, 50.0f, 0.1f, 0.4f}, 1.0f);
        paramListeners_.Add(p, [this](float l) {
            burg_lpc_.SetSmooth(l);
            block_burg_lpc_.SetSmear(l);
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kLPCGainAttack, 1}, id::kLPCGainAttack,
            juce::NormalisableRange<float>{10.0f, 100.0f, 1.0f, 0.4f}, 10.0f);
        paramListeners_.Add(p, [this](float l) {
            burg_lpc_.SetGainAttack(l);
            block_burg_lpc_.SetAttack(l);
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id::kLPCGainHold, 1}, id::kLPCGainHold,
                                                             juce::NormalisableRange<float>{1.0f, 100.0f, 1.0f, 0.4f},
                                                             10.0f);
        paramListeners_.Add(p, [this](float l) { burg_lpc_.SetGainHold(l); });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kLPCGainRelease, 1}, id::kLPCGainRelease,
            juce::NormalisableRange<float>{5.0f, 200.0f, 1.0f, 0.4f}, 20.0f);
        paramListeners_.Add(p, [this](float l) { burg_lpc_.SetGainRelease(l); });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kLPCOrder, 1}, id::kLPCOrder,
            juce::NormalisableRange<float>{4.0f, green_vocoder::dsp::LeakyBurgLPC::kNumPoles, 4.0f}, 36.0f);
        paramListeners_.Add(p, [this](float order) {
            burg_lpc_.SetLPCOrder(static_cast<int>(order));
            block_burg_lpc_.SetPoles(static_cast<size_t>(order));
        });
        layout.add(std::move(p));
    }

    // stft
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id::kStftWindowWidth, 1},
                                                             id::kStftWindowWidth, 0.0f, 5.0f, 2.0f);
        paramListeners_.Add(p, [this](float bw) { stft_vocoder_.SetBandwidth(bw); });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kMfccNumBands, 1}, id::kMfccNumBands,
            juce::NormalisableRange<float>{green_vocoder::dsp::STFTVocoder::kMinNumMfcc,
                                           green_vocoder::dsp::STFTVocoder::kMaxNumMfcc, 4.0f},
            20.0f);
        paramListeners_.Add(p, [this](float bw) { stft_vocoder_.SetNumMfcc(static_cast<size_t>(bw)); });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id::kStftRelease, 1}, id::kStftRelease,
                                                             juce::NormalisableRange<float>{1.0f, 1000.0f, 1.0f, 0.4f},
                                                             100.0f);
        paramListeners_.Add(p, [this](float bw) { stft_vocoder_.SetRelease(bw); });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id::kStftAttack, 1}, id::kStftAttack,
                                                             juce::NormalisableRange<float>{1.0f, 1000.0f, 1.0f, 0.4f},
                                                             1.0f);
        paramListeners_.Add(p, [this](float bw) { stft_vocoder_.SetAttack(bw); });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id::kStftBlend, 1}, id::kStftBlend, 0.0f,
                                                             0.99f, 0.2f);
        paramListeners_.Add(p, [this](float omega) { stft_vocoder_.SetBlend(omega); });
        layout.add(std::move(p));
    }
    {
        auto p =
            std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{id::kStftSize, 1}, id::kStftSize,
                                                         juce::StringArray{"256", "512", "1024", "2048", "4096"}, 2);
        paramListeners_.Add(p, [this](int idx) {
            static constexpr std::array kArray{256, 512, 1024, 2048, 4096};
            stft_vocoder_.SetFFTSize(kArray[idx]);
            block_burg_lpc_.SetBlockSize(kArray[idx]);
            SetLatency();
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id::kStftDetail, 1}, id::kStftDetail,
                                                             0.01f, 1.0f, 0.3f);
        paramListeners_.Add(p, [this](float omega) { stft_vocoder_.SetDetail(omega); });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{id::kStftType, 1}, id::kStftType,
                                                              juce::StringArray{"Standard", "Cepstrum", "MFCC"}, 1);
        paramListeners_.Add(
            p, [this](int mode) { stft_vocoder_.SetMode(static_cast<green_vocoder::dsp::STFTVocoder::Mode>(mode)); });
        layout.add(std::move(p));
    }

    // pitch tracking
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id::kTrackingLow, 1}, id::kTrackingLow,
                                                             20.0f, 300.0f, 80.0f);
        paramListeners_.Add(p, [this](float low) { yin_.SetMinPitch(low); });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id::kTrackingHigh, 1}, id::kTrackingHigh,
                                                             300.0f, 800.0f, 500.0f);
        paramListeners_.Add(p, [this](float max) { yin_.SetMaxPitch(max); });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id::kTrackingPitch, 1},
                                                             id::kTrackingPitch, -36.0f, 36.0f, 0.0f);
        paramListeners_.Add(p, [this](float pitch) { frequency_mul_ = std::exp2(pitch / 12.0f); });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id::kTrackingPwm, 1}, id::kTrackingPwm,
                                                             0.01f, 0.99f, 0.5f);
        paramListeners_.Add(p, [this](float pwm) { tracking_osc_.SetPWM(pwm); });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id::kTrackingNoise, 1},
                                                             id::kTrackingNoise, 0.0f, 1.0f, 0.5f);
        tracking_noise_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{id::kTrackingWaveform, 1}, id::kTrackingWaveform, juce::StringArray{"saw", "pwm"}, 0);
        tracking_waveform_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id::kTrackingGlide, 1}, id::kTrackingGlide,
            juce::NormalisableRange<float>{1.0f, 1000.0f, 1.0f, 0.4f}, 1.0f);
        paramListeners_.Add(p, [this](float bw) { pitch_glide_.MakeFilter(bw * getSampleRate() / 1000.0f); });
        layout.add(std::move(p));
    }

    value_tree_ = std::make_unique<juce::AudioProcessorValueTreeState>(*this, nullptr, kParameterValueTreeIdentify,
                                                                       std::move(layout));
    preset_manager_ = std::make_unique<pluginshared::PresetManager>(*value_tree_, *this);
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor() {
    paramListeners_.Clear();
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
    return 1; // NB: some hosts don't cope very well if you tell them there are 0 programs,
              // so this should be at least 1, even if you're not really implementing programs.
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
    float fs = static_cast<float>(sampleRate);
    size_t block_size = static_cast<size_t>(samplesPerBlock);

    burg_lpc_.Init(fs, block_size);
    stft_vocoder_.Init(fs);
    channel_vocoder_.Init(fs, block_size);
    block_burg_lpc_.Init(fs);

    yin_segement_.SetHop(1024);
    yin_segement_.SetSize(2048);
    yin_segement_.Reset();
    yin_.Init(static_cast<float>(sampleRate), 2048);
    osc_wpos_ = 0;
    pitch_glide_.Reset();
    first_init_ = true;

    pre_tilt_filter_.Reset();

    paramListeners_.MarkAll();
}

void AudioPluginAudioProcessor::reset() {}

void AudioPluginAudioProcessor::releaseResources() {
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) return false;

    // This checks if the input layout matches the output layout
#if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet()) return false;
#endif

    return true;
#endif
}

void AudioPluginAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    std::ignore = midiMessages;
    juce::ScopedNoDenormals noDenormals;

    paramListeners_.HandleDirty();

    // --- resolve channel routing ---
    bool const has_sidechain = buffer.getNumChannels() >= 4;

    bool const swap = channel_swap_->get();
    int pitch_ch_idx = pitch_channel_->getIndex(); // 0=Off, 1=ch0, 2=ch1, 3=ch2, 4=ch3
    if (!has_sidechain && pitch_ch_idx >= 3) pitch_ch_idx -= 2; // ch2/3 → ch0/1
    bool const use_pitch = pitch_ch_idx > 0;
    int const pitch_ch = pitch_ch_idx - 1; // 0-based channel index when active

#if I_AM_USING_LOOPBACK_DEBUG
    int const mod_ch = 0;
    int const carry_ch = 1;
#else
    int mod_ch = 0;
    int carry_ch = has_sidechain ? 2 : 0;
    if (swap) std::swap(mod_ch, carry_ch);
#endif

    size_t const num_samples = static_cast<size_t>(buffer.getNumSamples());

    // --- block processing ---
    for (size_t pos = 0; pos < num_samples; pos += kBlockSize) {
        size_t const n = std::min(kBlockSize, num_samples - pos);

        // fill modulator
        {
            float const* ml = buffer.getReadPointer(mod_ch) + pos;
#if I_AM_USING_LOOPBACK_DEBUG
            for (size_t i = 0; i < n; ++i) crossing_main_buffer_[i].Broadcast(ml[i]);
#else
            float const* mr = buffer.getReadPointer(mod_ch + 1) + pos;
            for (size_t i = 0; i < n; ++i) crossing_main_buffer_[i] = {ml[i], mr[i]};
#endif
        }

        // fill carrier (pitch tracking or direct channel pair)
        if (use_pitch) {
            ProcessPitchTracking(crossing_side_buffer_, buffer, pitch_ch, pos, n);
        } else {
            float const* sl = buffer.getReadPointer(carry_ch) + pos;
#if I_AM_USING_LOOPBACK_DEBUG
            for (size_t i = 0; i < n; ++i) crossing_side_buffer_[i].Broadcast(sl[i]);
#else
            float const* sr = buffer.getReadPointer(carry_ch + 1) + pos;
            for (size_t i = 0; i < n; ++i) crossing_side_buffer_[i] = {sl[i], sr[i]};
#endif
        }

        // pre-tilt filter
        for (size_t i = 0; i < n; ++i) {
            crossing_main_buffer_[i] = pre_tilt_filter_.Tick(crossing_main_buffer_[i]);
        }

        // vocoder
        {
            eVocoderType const type = static_cast<eVocoderType>(vocoder_type_param_->getIndex());
            if (last_vocoder_type_ != type) last_vocoder_type_ = type;

            switch (type) {
                case eVocoderType_LeakyBurgLPC:
                    burg_lpc_.Process({crossing_main_buffer_.data(), n}, {crossing_side_buffer_.data(), n});
                    break;
                case eVocoderType_STFTVocoder:
                    stft_vocoder_.Process(crossing_main_buffer_.data(), crossing_side_buffer_.data(), n);
                    break;
                case eVocoderType_ChannelVocoder:
                    channel_vocoder_.ProcessBlock(crossing_main_buffer_.data(), crossing_side_buffer_.data(), n);
                    break;
                case eVocoderType_BlockBurgLPC:
                    block_burg_lpc_.Process(crossing_main_buffer_.data(), crossing_side_buffer_.data(), n);
                    break;
                default:
                    jassertfalse;
                    break;
            }
        }

        // write output
        {
            float* out_l = buffer.getWritePointer(0) + pos;
            float* out_r = buffer.getWritePointer(1) + pos;
            for (size_t i = 0; i < n; ++i) {
                out_l[i] = crossing_main_buffer_[i][0];
                out_r[i] = crossing_main_buffer_[i][1];
            }
        }
    }

    // latency check
    if (latency_.load() != old_latency_) {
        old_latency_ = latency_.load();
        setLatencySamples(old_latency_);
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

void AudioPluginAudioProcessor::SetLatency() {
    int latency = 0;
    switch (vocoder_type_param_->getIndex()) {
        case eVocoderType_STFTVocoder:
        case eVocoderType_BlockBurgLPC:
            latency += stft_vocoder_.GetFFTSize();
            break;
        default:
            break;
    }

    // setLatencySamples(latency);
    latency_.store(latency);
}

void AudioPluginAudioProcessor::ProcessPitchTracking(std::array<qwqdsp_simd_element::PackFloat<2>, kBlockSize>& dst,
                                                     const juce::AudioBuffer<float>& buffer, int pitch_ch, size_t pos,
                                                     size_t n) {
    float const* pitch_buffer = buffer.getReadPointer(pitch_ch) + pos;
    yin_segement_.Push({pitch_buffer, n});

    float const noise_gain = tracking_noise_->get();
    while (yin_segement_.CanProcess()) {
        yin_.Process(yin_segement_.GetBlock());
        yin_segement_.Advance();

        auto pitch = yin_.GetPitch();
        size_t const iwant = static_cast<size_t>(yin_segement_.GetHop());
        size_t const can_write = std::min(osc_buffer_.size() - osc_wpos_, iwant);

        float target_pitch = pitch.pitch_hz * frequency_mul_;
        target_pitch = std::max(target_pitch, 0.1f);

        // fill trivial wave
        float curr_trivial_wave_gain = last_osc_mix_;
        float const delta_trivial_wave_gain =
            (1.0f - pitch.non_period_ratio - curr_trivial_wave_gain) / static_cast<float>(can_write);
        size_t osc_wpos = osc_wpos_;
        if (tracking_waveform_->getIndex() == 0) {
            for (size_t i = 0; i < can_write; ++i) {
                curr_trivial_wave_gain += delta_trivial_wave_gain;
                tracking_osc_.SetFreq(pitch_glide_.Tick(target_pitch), static_cast<float>(getSampleRate()));
                osc_buffer_[osc_wpos++] = tracking_osc_.Sawtooth() * curr_trivial_wave_gain;
            }
        }
        else {
            for (size_t i = 0; i < can_write; ++i) {
                curr_trivial_wave_gain += delta_trivial_wave_gain;
                tracking_osc_.SetFreq(pitch_glide_.Tick(target_pitch), static_cast<float>(getSampleRate()));
                osc_buffer_[osc_wpos++] = tracking_osc_.PWM_NoDC() * curr_trivial_wave_gain;
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

    size_t const cancopy = std::min(osc_wpos_, n);
    for (size_t i = 0; i < cancopy; ++i) dst[i] = {osc_buffer_[i], osc_buffer_[i]};
    for (size_t i = cancopy; i < n; ++i) dst[i].Broadcast(0);

    size_t const drag = osc_wpos_ - cancopy;
    for (size_t i = 0; i < drag; ++i) osc_buffer_[i] = osc_buffer_[i + cancopy];
    osc_wpos_ -= cancopy;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new AudioPluginAudioProcessor();
}
