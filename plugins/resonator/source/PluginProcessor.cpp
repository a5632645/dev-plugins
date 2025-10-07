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
            juce::NormalisableRange<float>{0.0f, 127.0f, 0.01f},
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
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            "global_damp",
            "global_damp",
            100, 140, 130
        );
        param_listener_.Add(p, [this, ptr = p.get()](float v) {
            float const val01 = ptr->convertTo0to1(v);
            for (auto& d : damp_pitch_) {
                d->setValueNotifyingHost(val01);
            }
        });
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
            juce::NormalisableRange<float>{0.0f, 32000.0f, 0.1f, 0.4f},
            i == 0 ? 500.0f : 0.0f
        );
        decays_[i] = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            "global_decay",
            "global_decay",
            juce::NormalisableRange<float>{0.0f, 32000.0f, 0.1f, 0.4f},
            0.0f
        );
        param_listener_.Add(p, [this, ptr = p.get()](float v) {
            float const val01 = ptr->convertTo0to1(v);
            for (auto& d : decays_) {
                d->setValueNotifyingHost(val01);
            }
        });
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
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            "global_mix",
            "global_mix",
            -61.0f, 0.0f, -61.0f
        );
        param_listener_.Add(p, [this, ptr = p.get()](float v) {
            float const val01 = ptr->convertTo0to1(v);
            for (auto& d : mix_volume_) {
                d->setValueNotifyingHost(val01);
            }
        });
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
        auto p = std::make_unique<juce::AudioParameterBool>(
            "round_robin",
            "round_robin",
            false
        );
        allow_round_robin_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            "dry",
            "dry",
            0.0f, 1.0f, 1.0f
        );
        dry_mix_ = p.get();
        layout.add(std::move(p));
    }

    value_tree_ = std::make_unique<juce::AudioProcessorValueTreeState>(*this, nullptr, "PARAMETERS", std::move(layout));

    input_volume_.fill(1);
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

    bool midi_drive = midi_drive_->get();
    if (midi_drive != was_midi_drive_) {
        was_midi_drive_ = midi_drive;
        if (midi_drive) {
            note_manager_.initializeVoices();
            input_volume_.fill(0);
        }
        else {
            input_volume_.fill(1);
        }
    }

    // output mix volumes
    for (size_t i = 0; i < kNumResonators; ++i) {
        float const db = mix_volume_[i]->get();
        if (db < -60.0f) {
            mix_[i] = 0;
        }
        else {
            mix_[i] = qwqdsp::convert::Db2Gain(db);
        }
    }

    // update scatter matrix
    for (size_t i = 0; i < ScatterMatrix::kNumReflections; ++i) {
        reflections_[i] = matrix_reflections_[i]->get();
    }

    // update damp filter
    std::array<float, kNumResonators> damp_w;
    for (size_t i = 0; i < kNumResonators; ++i) {
        float const freq = qwqdsp::convert::Pitch2Freq(damp_pitch_[i]->get());
        damp_w[i] = freq * std::numbers::pi_v<float> * 2 / getSampleRate();
    }
    damp_.SetFrequency(damp_w);

    // update decays
    for (size_t i = 0; i < kNumResonators; ++i) {
        float const decay_ms = decays_[i]->get();
        if (decay_ms > 0.5f) {
            float const mul = -3.0f * delay_samples_[i] / (getSampleRate() * decay_ms / 1000.0f);
            feedbacks_[i] = std::pow(10.0f, mul);
            feedbacks_[i] = std::min(feedbacks_[i], 1.0f);
        }
        else {
            feedbacks_[i] = 0;
        }

        if (polarity_[i]->get()) {
            feedbacks_[i] = -feedbacks_[i];
        }
    }

    dry_volume_ = dry_mix_->get();

    if (was_midi_drive_) {
        ProcessMidi(buffer, midiMessages);
    }
    else {
        ProcessCommon(buffer, midiMessages);
    }
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



void ResonatorAudioProcessor::ProcessCommon(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    size_t const num_samples = buffer.getNumSamples();
    float* left_ptr = buffer.getWritePointer(0);
    float* right_ptr = buffer.getWritePointer(1);

    std::array<float, kNumResonators> pole_radius;
    for (size_t i = 0; i < kNumResonators; ++i) {
        pole_radius[i] = dispersion_pole_radius_[i]->get();
    }

    // update delaylines
    std::array<float, kNumResonators> omegas;
    std::array<float, kNumResonators> allpass_set_delay;
    for (size_t i = 0; i < kNumResonators; ++i) {
        float pitch = pitches_[i]->get() + fine_tune_[i]->get() / 100.0f;
        if (polarity_[i]->get()) {
            pitch += 12;
        }
        float const freq = qwqdsp::convert::Pitch2Freq(pitch);
        omegas[i] = freq * std::numbers::pi_v<float> / getSampleRate();
        delay_samples_[i] = getSampleRate() / freq;
        allpass_set_delay[i] = delay_samples_[i] * pole_radius[i] / (mana::ThrianAllpass::kMaxNumAPF + 0.1f);
    }

    // update allpass filters
    auto allpass_delay = dispersion_.SetFilter(allpass_set_delay, omegas);

    // remove allpass delays
    for (size_t i = 0; i < kNumResonators; ++i) {
        delay_samples_[i] -= allpass_delay[i];
        delay_samples_[i] = std::max(delay_samples_[i], 0.0f);
    }

    // processing
    ProcessDSP(left_ptr, num_samples);

    std::copy_n(left_ptr, num_samples, right_ptr);
}

void ResonatorAudioProcessor::ProcessDSP(float* ptr, size_t len) {
    // processing
    for (size_t i = 0; i < len; ++i) {
        float out = dry_volume_ * ptr[i];
        for (size_t j = 0; j < kNumResonators; ++j) {
            out += fb_values_[j] * mix_[j];
        }

        for (size_t j = 0; j < kNumResonators; ++j) {
            fb_values_[j] += input_volume_[j] * ptr[i];
        }

        delay_.Tick(fb_values_, delay_samples_);
        dispersion_.Tick(fb_values_);
        damp_.Tick(fb_values_);
        matrix_.Tick(fb_values_, reflections_);
        for (size_t j = 0; j < kNumResonators; ++j) {
            fb_values_[j] *= feedbacks_[j];
        }

        ptr[i] = out;
    }
}

void ResonatorAudioProcessor::ProcessMidi(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi_buffer) {
    size_t const num_samples = buffer.getNumSamples();
    float* left_ptr = buffer.getWritePointer(0);
    float* right_ptr = buffer.getWritePointer(1);

    int buffer_pos = 0;
    for (auto midi : midi_buffer) {
        auto message = midi.getMessage();
        if (message.isNoteOnOrOff()) {
            ProcessDSP(left_ptr + buffer_pos, midi.samplePosition - buffer_pos);
            buffer_pos = midi.samplePosition;

            if (message.isNoteOn()) {
                int resonator_idx = note_manager_.noteOn(message.getNoteNumber(), allow_round_robin_->get());
                float pitch = message.getNoteNumber() + fine_tune_[resonator_idx]->get() / 100.0f;
                if (polarity_[resonator_idx]->get()) {
                    pitch += 12;
                }
                float const freq = qwqdsp::convert::Pitch2Freq(pitch);
                float const omega = freq * std::numbers::pi_v<float> / getSampleRate();
                float const delay_samples = getSampleRate() / freq;
                float const allpass_set_delay = delay_samples * dispersion_pole_radius_[resonator_idx]->get() / (mana::ThrianAllpass::kMaxNumAPF + 0.1f);

                // update allpass filters
                float const allpass_delay = dispersion_.SetSingleFilter(resonator_idx, allpass_set_delay, omega);

                // remove allpass delays
                delay_samples_[resonator_idx] = delay_samples - allpass_delay;
                delay_samples_[resonator_idx] = std::max(delay_samples_[resonator_idx], 0.0f);

                // addition input volume
                input_volume_[resonator_idx] = message.getFloatVelocity();
            }
            else {
                int resonator_idx = note_manager_.noteOff(message.getNoteNumber());
                // just mute
                input_volume_[resonator_idx] = 0;
            }
        }
    }
    ProcessDSP(left_ptr + buffer_pos, num_samples - buffer_pos);

    std::copy_n(left_ptr, num_samples, right_ptr);
}
