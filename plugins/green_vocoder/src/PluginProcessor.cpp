#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <array>
#include <memory>

#include "global.hpp"
#include "params.hpp"

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

    // pre fx
    {
        auto p = params_.pre_tilt.Build();
        paramListeners_.Add(p, [this](float db) { pre_tilt_filter_.SetTilt(static_cast<float>(getSampleRate()), db); });
        layout.add(std::move(p));
    }
    {
        auto p = params_.channel_swap.Build();
        paramListeners_.Add(p, [](bool) {});
        layout.add(std::move(p));
    }
    {
        auto p = params_.pitch_channel.Build();
        layout.add(std::move(p));
    }

    // vocoder type
    {
        auto p = params_.vocoder_type.Build();
        paramListeners_.Add(p, [this](int) { SetLatency(); });
        layout.add(std::move(p));
    }

    // pitch shifter
    {
        auto p = params_.shift_pitch.Build();
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
        auto p = params_.cv_filter_bank_mode.Build();
        paramListeners_.Add(p, [this](int mode) {
            channel_vocoder_.SetFilterBankMode(static_cast<green_vocoder::dsp::ChannelVocoder::FilterBankMode>(mode));
        });
        layout.add(std::move(p));
    }
    {
        auto p = params_.cv_attack.Build();
        paramListeners_.Add(p, [this](float v) { channel_vocoder_.SetAttack(v); });
        layout.add(std::move(p));
    }
    {
        auto p = params_.cv_gate.Build();
        paramListeners_.Add(p, [this](float v) { channel_vocoder_.SetGate(v); });
        layout.add(std::move(p));
    }
    {
        auto p = params_.cv_release.Build();
        paramListeners_.Add(p, [this](float v) { channel_vocoder_.SetRelease(v); });
        layout.add(std::move(p));
    }
    {
        auto p = params_.cv_freq_begin.Build();
        paramListeners_.Add(p, [this](float v) { channel_vocoder_.SetFreqBegin(v); });
        layout.add(std::move(p));
    }
    {
        auto p = params_.cv_freq_end.Build();
        paramListeners_.Add(p, [this](float v) { channel_vocoder_.SetFreqEnd(v); });
        layout.add(std::move(p));
    }
    {
        auto p = params_.cv_nbands.Build();
        paramListeners_.Add(p, [this](float v) { channel_vocoder_.SetNumBands(static_cast<int>(std::round(v))); });
        layout.add(std::move(p));
    }
    {
        auto p = params_.cv_scale.Build();
        paramListeners_.Add(p, [this](float v) { channel_vocoder_.SetModulatorScale(v); });
        layout.add(std::move(p));
    }
    {
        auto p = params_.cv_ripple.Build();
        paramListeners_.Add(p, [this](float v) { channel_vocoder_.SetFilterRipple(v); });
        layout.add(std::move(p));
    }
    {
        auto p = params_.cv_carry_scale.Build();
        paramListeners_.Add(p, [this](float v) { channel_vocoder_.SetCarryScale(v); });
        layout.add(std::move(p));
    }
    {
        auto p = params_.cv_map.Build();
        paramListeners_.Add(
            p, [this](int i) { channel_vocoder_.SetMap(static_cast<green_vocoder::dsp::eChannelVocoderMap>(i)); });
        layout.add(std::move(p));
    }

    // lpc
    {
        auto p = params_.lpc_forget.Build();
        paramListeners_.Add(p, [this](float l) { burg_lpc_.SetForget(l); });
        layout.add(std::move(p));
    }
    {
        auto p = params_.lpc_smooth.Build();
        paramListeners_.Add(p, [this](float l) {
            burg_lpc_.SetSmooth(l);
            block_burg_lpc_.SetSmear(l);
        });
        layout.add(std::move(p));
    }
    {
        auto p = params_.lpc_gain_attack.Build();
        paramListeners_.Add(p, [this](float l) {
            burg_lpc_.SetGainAttack(l);
            block_burg_lpc_.SetAttack(l);
        });
        layout.add(std::move(p));
    }
    {
        auto p = params_.lpc_gain_hold.Build();
        paramListeners_.Add(p, [this](float l) { burg_lpc_.SetGainHold(l); });
        layout.add(std::move(p));
    }
    {
        auto p = params_.lpc_gain_release.Build();
        paramListeners_.Add(p, [this](float l) { burg_lpc_.SetGainRelease(l); });
        layout.add(std::move(p));
    }
    {
        auto p = params_.lpc_order.Build();
        paramListeners_.Add(p, [this](float order) {
            burg_lpc_.SetLPCOrder(static_cast<int>(order));
            block_burg_lpc_.SetPoles(static_cast<size_t>(order));
        });
        layout.add(std::move(p));
    }

    // stft
    {
        auto p = params_.stft_bandwidth.Build();
        paramListeners_.Add(p, [this](float bw) { stft_vocoder_.SetBandwidth(bw); });
        layout.add(std::move(p));
    }
    {
        auto p = params_.mfcc_nbands.Build();
        paramListeners_.Add(p, [this](float bw) { stft_vocoder_.SetNumMfcc(static_cast<size_t>(bw)); });
        layout.add(std::move(p));
    }
    {
        auto p = params_.stft_release.Build();
        paramListeners_.Add(p, [this](float bw) { stft_vocoder_.SetRelease(bw); });
        layout.add(std::move(p));
    }
    {
        auto p = params_.stft_attack.Build();
        paramListeners_.Add(p, [this](float bw) { stft_vocoder_.SetAttack(bw); });
        layout.add(std::move(p));
    }
    {
        auto p = params_.stft_blend.Build();
        paramListeners_.Add(p, [this](float omega) { stft_vocoder_.SetBlend(omega); });
        layout.add(std::move(p));
    }
    {
        auto p = params_.stft_size.Build();
        paramListeners_.Add(p, [this](int idx) {
            static constexpr std::array kArray{256, 512, 1024, 2048, 4096};
            stft_vocoder_.SetFFTSize(kArray[idx]);
            block_burg_lpc_.SetBlockSize(kArray[idx]);
            SetLatency();
        });
        layout.add(std::move(p));
    }
    {
        auto p = params_.stft_detail.Build();
        paramListeners_.Add(p, [this](float omega) { stft_vocoder_.SetDetail(omega); });
        layout.add(std::move(p));
    }
    {
        auto p = params_.stft_type.Build();
        paramListeners_.Add(
            p, [this](int mode) { stft_vocoder_.SetMode(static_cast<green_vocoder::dsp::STFTVocoder::Mode>(mode)); });
        layout.add(std::move(p));
    }

    // pitch tracking
    {
        auto p = params_.track_low.Build();
        paramListeners_.Add(p, [this](float low) { pitch_osc_.SetMinPitch(low); });
        layout.add(std::move(p));
    }
    {
        auto p = params_.track_high.Build();
        paramListeners_.Add(p, [this](float max) { pitch_osc_.SetMaxPitch(max); });
        layout.add(std::move(p));
    }
    {
        auto p = params_.track_pitch.Build();
        paramListeners_.Add(p, [this](float pitch) { pitch_osc_.SetPitchShift(pitch); });
        layout.add(std::move(p));
    }
    {
        auto p = params_.track_pwm.Build();
        paramListeners_.Add(p, [this](float pwm) { pitch_osc_.SetPWM(pwm); });
        layout.add(std::move(p));
    }
    {
        auto p = params_.track_noise.Build();
        paramListeners_.Add(p, [this](float g) { pitch_osc_.SetNoiseGain(g); });
        layout.add(std::move(p));
    }
    {
        auto p = params_.track_waveform.Build();
        paramListeners_.Add(p, [this](int idx) { pitch_osc_.SetWaveform(idx); });
        layout.add(std::move(p));
    }
    {
        auto p = params_.track_glide.Build();
        paramListeners_.Add(p, [this](float bw) { pitch_osc_.SetGlide(bw); });
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

    pitch_osc_.Init(static_cast<float>(sampleRate));
    pitch_osc_.Reset();
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

    bool const swap = params_.channel_swap.Get();
    int pitch_ch_idx = params_.pitch_channel.Get();             // 0=Off, 1=ch0, 2=ch1, 3=ch2, 4=ch3
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
            std::array<float, 256> mono;
            float const* src = buffer.getReadPointer(pitch_ch) + pos;
            std::copy_n(src, n, mono.begin());
            pitch_osc_.Process(mono.data(), static_cast<int>(n));
            for (size_t i = 0; i < n; ++i) crossing_side_buffer_[i] = {mono[i], mono[i]};
        }
        else {
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
            eVocoderType const type = static_cast<eVocoderType>(params_.vocoder_type.Get());
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
    switch (params_.vocoder_type.Get()) {
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

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new AudioPluginAudioProcessor();
}
