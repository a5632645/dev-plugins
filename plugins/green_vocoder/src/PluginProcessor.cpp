#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <array>
#include <memory>

#include "global.hpp"

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

    // --- pre fx ---
    {
        auto p = params_.pre_tilt.Build();
        paramListeners_.Add(p, [this](float db) {
            tilt_mb_.pre_tilt_db = db;
            tilt_mb_.dirty.store(true, std::memory_order_release);
        });
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

    // --- pitch shifter (formant shift → multiple DSP modules) ---
    {
        auto p = params_.shift_pitch.Build();
        paramListeners_.Add(p, [this](float l) {
            cv_mb_.formant_shift = l;
            cv_mb_.dirty.store(true, std::memory_order_release);
            stft_mb_.formant_shift = l;
            stft_mb_.dirty.store(true, std::memory_order_release);
            leaky_lpc_mb_.formant_shift = l;
            leaky_lpc_mb_.dirty.store(true, std::memory_order_release);
            block_lpc_mb_.formant_shift = l;
            block_lpc_mb_.dirty.store(true, std::memory_order_release);
        });
        layout.add(std::move(p));
    }

    // --- channel vocoder ---
    {
        auto p = params_.cv_filter_bank_mode.Build();
        paramListeners_.Add(p, [this](int mode) {
            cv_mb_.filter_bank_mode = mode;
            cv_mb_.dirty.store(true, std::memory_order_release);
        });
        layout.add(std::move(p));
    }
    {
        auto p = params_.cv_attack.Build();
        paramListeners_.Add(p, [this](float v) {
            cv_mb_.attack = v;
            cv_mb_.dirty.store(true, std::memory_order_release);
        });
        layout.add(std::move(p));
    }
    {
        auto p = params_.cv_gate.Build();
        paramListeners_.Add(p, [this](float v) {
            cv_mb_.gate = v;
            cv_mb_.dirty.store(true, std::memory_order_release);
        });
        layout.add(std::move(p));
    }
    {
        auto p = params_.cv_release.Build();
        paramListeners_.Add(p, [this](float v) {
            cv_mb_.release = v;
            cv_mb_.dirty.store(true, std::memory_order_release);
        });
        layout.add(std::move(p));
    }
    {
        auto p = params_.cv_freq_begin.Build();
        paramListeners_.Add(p, [this](float v) {
            cv_mb_.freq_begin = v;
            cv_mb_.dirty.store(true, std::memory_order_release);
        });
        layout.add(std::move(p));
    }
    {
        auto p = params_.cv_freq_end.Build();
        paramListeners_.Add(p, [this](float v) {
            cv_mb_.freq_end = v;
            cv_mb_.dirty.store(true, std::memory_order_release);
        });
        layout.add(std::move(p));
    }
    {
        auto p = params_.cv_nbands.Build();
        paramListeners_.Add(p, [this](float v) {
            cv_mb_.nbands = static_cast<int>(std::round(v));
            cv_mb_.dirty.store(true, std::memory_order_release);
        });
        layout.add(std::move(p));
    }
    {
        auto p = params_.cv_scale.Build();
        paramListeners_.Add(p, [this](float v) {
            cv_mb_.scale = v;
            cv_mb_.dirty.store(true, std::memory_order_release);
        });
        layout.add(std::move(p));
    }
    {
        auto p = params_.cv_ripple.Build();
        paramListeners_.Add(p, [this](float v) {
            cv_mb_.ripple = v;
            cv_mb_.dirty.store(true, std::memory_order_release);
        });
        layout.add(std::move(p));
    }
    {
        auto p = params_.cv_carry_scale.Build();
        paramListeners_.Add(p, [this](float v) {
            cv_mb_.carry_scale = v;
            cv_mb_.dirty.store(true, std::memory_order_release);
        });
        layout.add(std::move(p));
    }
    {
        auto p = params_.cv_map.Build();
        paramListeners_.Add(p, [this](int i) {
            cv_mb_.map = i;
            cv_mb_.dirty.store(true, std::memory_order_release);
        });
        layout.add(std::move(p));
    }

    // --- lpc ---
    {
        auto p = params_.lpc_forget.Build();
        paramListeners_.Add(p, [this](float l) {
            leaky_lpc_mb_.forget_rate = l;
            leaky_lpc_mb_.dirty.store(true, std::memory_order_release);
        });
        layout.add(std::move(p));
    }
    {
        auto p = params_.lpc_smooth.Build();
        paramListeners_.Add(p, [this](float l) {
            leaky_lpc_mb_.smooth = l;
            leaky_lpc_mb_.dirty.store(true, std::memory_order_release);
            block_lpc_mb_.smear = l;
            block_lpc_mb_.dirty.store(true, std::memory_order_release);
        });
        layout.add(std::move(p));
    }
    {
        auto p = params_.lpc_gain_attack.Build();
        paramListeners_.Add(p, [this](float l) {
            leaky_lpc_mb_.gain_attack = l;
            leaky_lpc_mb_.dirty.store(true, std::memory_order_release);
            block_lpc_mb_.attack = l;
            block_lpc_mb_.dirty.store(true, std::memory_order_release);
        });
        layout.add(std::move(p));
    }
    {
        auto p = params_.lpc_gain_hold.Build();
        paramListeners_.Add(p, [this](float l) {
            leaky_lpc_mb_.gain_hold = l;
            leaky_lpc_mb_.dirty.store(true, std::memory_order_release);
        });
        layout.add(std::move(p));
    }
    {
        auto p = params_.lpc_gain_release.Build();
        paramListeners_.Add(p, [this](float l) {
            leaky_lpc_mb_.gain_release = l;
            leaky_lpc_mb_.dirty.store(true, std::memory_order_release);
        });
        layout.add(std::move(p));
    }
    {
        auto p = params_.lpc_order.Build();
        paramListeners_.Add(p, [this](float order) {
            int o = static_cast<int>(order);
            leaky_lpc_mb_.order = static_cast<float>(o);
            leaky_lpc_mb_.dirty.store(true, std::memory_order_release);
            block_lpc_mb_.poles = static_cast<float>(o);
            block_lpc_mb_.dirty.store(true, std::memory_order_release);
        });
        layout.add(std::move(p));
    }

    // --- stft ---
    {
        auto p = params_.stft_bandwidth.Build();
        paramListeners_.Add(p, [this](float bw) {
            stft_mb_.bandwidth = bw;
            stft_mb_.dirty.store(true, std::memory_order_release);
        });
        layout.add(std::move(p));
    }
    {
        auto p = params_.mfcc_nbands.Build();
        paramListeners_.Add(p, [this](float bw) {
            stft_mb_.num_mfcc = static_cast<int>(bw);
            stft_mb_.dirty.store(true, std::memory_order_release);
        });
        layout.add(std::move(p));
    }
    {
        auto p = params_.stft_release.Build();
        paramListeners_.Add(p, [this](float bw) {
            stft_mb_.release = bw;
            stft_mb_.dirty.store(true, std::memory_order_release);
        });
        layout.add(std::move(p));
    }
    {
        auto p = params_.stft_attack.Build();
        paramListeners_.Add(p, [this](float bw) {
            stft_mb_.attack = bw;
            stft_mb_.dirty.store(true, std::memory_order_release);
        });
        layout.add(std::move(p));
    }
    {
        auto p = params_.stft_blend.Build();
        paramListeners_.Add(p, [this](float omega) {
            stft_mb_.blend = omega;
            stft_mb_.dirty.store(true, std::memory_order_release);
        });
        layout.add(std::move(p));
    }
    {
        auto p = params_.stft_size.Build();
        paramListeners_.Add(p, [this](int idx) {
            static constexpr std::array kArray{256, 512, 1024, 2048, 4096};
            int size = kArray[idx];
            stft_mb_.fft_size = size;
            stft_mb_.dirty.store(true, std::memory_order_release);
            block_lpc_mb_.block_size = size;
            block_lpc_mb_.dirty.store(true, std::memory_order_release);
        });
        layout.add(std::move(p));
    }
    {
        auto p = params_.stft_detail.Build();
        paramListeners_.Add(p, [this](float omega) {
            stft_mb_.detail = omega;
            stft_mb_.dirty.store(true, std::memory_order_release);
        });
        layout.add(std::move(p));
    }
    {
        auto p = params_.stft_type.Build();
        paramListeners_.Add(p, [this](int mode) {
            stft_mb_.mode = mode;
            stft_mb_.dirty.store(true, std::memory_order_release);
        });
        layout.add(std::move(p));
    }

    // --- pitch tracking ---
    {
        auto p = params_.track_low.Build();
        paramListeners_.Add(p, [this](float low) {
            pitch_osc_mb_.min_pitch = low;
            pitch_osc_mb_.dirty.store(true, std::memory_order_release);
        });
        layout.add(std::move(p));
    }
    {
        auto p = params_.track_high.Build();
        paramListeners_.Add(p, [this](float max) {
            pitch_osc_mb_.max_pitch = max;
            pitch_osc_mb_.dirty.store(true, std::memory_order_release);
        });
        layout.add(std::move(p));
    }
    {
        auto p = params_.track_pitch.Build();
        paramListeners_.Add(p, [this](float pitch) {
            pitch_osc_mb_.pitch_shift = pitch;
            pitch_osc_mb_.dirty.store(true, std::memory_order_release);
        });
        layout.add(std::move(p));
    }
    {
        auto p = params_.track_pwm.Build();
        paramListeners_.Add(p, [this](float pwm) {
            pitch_osc_mb_.pwm = pwm;
            pitch_osc_mb_.dirty.store(true, std::memory_order_release);
        });
        layout.add(std::move(p));
    }
    {
        auto p = params_.track_noise.Build();
        paramListeners_.Add(p, [this](float g) {
            pitch_osc_mb_.noise_gain = g;
            pitch_osc_mb_.dirty.store(true, std::memory_order_release);
        });
        layout.add(std::move(p));
    }
    {
        auto p = params_.track_waveform.Build();
        paramListeners_.Add(p, [this](int idx) {
            pitch_osc_mb_.waveform = idx;
            pitch_osc_mb_.dirty.store(true, std::memory_order_release);
        });
        layout.add(std::move(p));
    }
    {
        auto p = params_.track_glide.Build();
        paramListeners_.Add(p, [this](float bw) {
            pitch_osc_mb_.glide = bw;
            pitch_osc_mb_.dirty.store(true, std::memory_order_release);
        });
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
    return 1;
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
    engine_.Init(sampleRate, static_cast<size_t>(samplesPerBlock));
    paramListeners_.MarkAll();
}

void AudioPluginAudioProcessor::reset() {}

void AudioPluginAudioProcessor::releaseResources() {
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) return false;
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

    // --- sync dirty mailboxes → Engine ---
    if (tilt_mb_.dirty.exchange(false, std::memory_order_acq_rel))
        engine_.UpdateTiltFilter(tilt_mb_);
    if (leaky_lpc_mb_.dirty.exchange(false, std::memory_order_acq_rel))
        engine_.UpdateLeakyLPC(leaky_lpc_mb_);
    if (block_lpc_mb_.dirty.exchange(false, std::memory_order_acq_rel))
        engine_.UpdateBlockLPC(block_lpc_mb_);
    if (stft_mb_.dirty.exchange(false, std::memory_order_acq_rel))
        engine_.UpdateSTFT(stft_mb_);
    if (cv_mb_.dirty.exchange(false, std::memory_order_acq_rel))
        engine_.UpdateChannelVocoder(cv_mb_);
    if (pitch_osc_mb_.dirty.exchange(false, std::memory_order_acq_rel))
        engine_.UpdatePitchOsc(pitch_osc_mb_);

    // --- resolve channel routing ---
    bool const has_sidechain = buffer.getNumChannels() >= 4;

    bool const swap = params_.channel_swap.Get();
    int pitch_ch_idx = params_.pitch_channel.Get();             // 0=Off, 1=ch0, 2=ch1, 3=ch2, 4=ch3
    if (!has_sidechain && pitch_ch_idx >= 3) pitch_ch_idx -= 2; // ch2/3 → ch0/1
    bool const use_pitch = pitch_ch_idx > 0;
    int const pitch_ch = pitch_ch_idx - 1; // 0-based channel index when active

    int mod_ch = 0;
    int carry_ch = has_sidechain ? 2 : 0;
    if (swap) std::swap(mod_ch, carry_ch);

    eVocoderType const vocoder_type = static_cast<eVocoderType>(params_.vocoder_type.Get());

    // --- delegate to engine ---
    engine_.Process(buffer, mod_ch, carry_ch, pitch_ch, use_pitch, vocoder_type);

    // --- latency check ---
    int new_latency = engine_.GetLatency();
    if (new_latency != old_latency_) {
        old_latency_ = new_latency;
        setLatencySamples(new_latency);
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

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new AudioPluginAudioProcessor();
}
