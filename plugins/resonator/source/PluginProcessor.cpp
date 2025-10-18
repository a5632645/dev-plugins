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
            juce::ParameterID{juce::String{"pitch"} + juce::String{i}, 1},
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
    for (size_t i = 0; i < kNumResonators; ++i) {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::String{"gain"} + juce::String{i},
            juce::String{"gain"} + juce::String{i},
            -20.0f, 0.0f, -6.01f
        );
        damp_gain_db_[i] = p.get();
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
    for (size_t i = 0; i < kNumResonators; ++i) {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::String{"reflection"} + juce::String{i},
            juce::String{"reflection"} + juce::String{i},
            -1.0f, 1.0f, 0.0f
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
    preset_manager_ = std::make_unique<pluginshared::PresetManager>(*value_tree_, *this);
    preset_manager_->external_load_default_operations = [this]{
        dsp_.TrunOnAllInput(1);
    };
    dsp_.TrunOnAllInput(1);
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
    std::ignore = samplesPerBlock;
    dsp_.Init(static_cast<float>(sampleRate), 0.0f);
    dsp_.Reset();
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
            dsp_.TrunOnAllInput(0);
        }
        else {
            dsp_.TrunOnAllInput(1);
        }
    }

    for (size_t i = 0; i < kNumResonators; ++i) {
        dsp_.polarity[i] = polarity_[i]->get();
        dsp_.dispersion[i] = dispersion_pole_radius_[i]->get();
        dsp_.decay_ms[i] = decays_[i]->get();
        dsp_.damp_pitch[i] = damp_pitch_[i]->get();
        dsp_.damp_gain_db[i] = damp_gain_db_[i]->get();
        dsp_.mix_db[i] = mix_volume_[i]->get();
        dsp_.dry = dry_mix_->get();
        dsp_.norm_reflections[i] = matrix_reflections_[i]->get();
    }
    dsp_.UpdateBasicParams();

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
    size_t const num_samples = static_cast<size_t>(buffer.getNumSamples());
    float* left_ptr = buffer.getWritePointer(0);
    float* right_ptr = buffer.getWritePointer(1);

    for (size_t i = 0; i < kNumResonators; ++i) {
        dsp_.pitches[i] = pitches_[i]->get();
        dsp_.fine_tune[i] = fine_tune_[i]->get();
    }
    dsp_.UpdateAllPitches();

    dsp_.Process(left_ptr, right_ptr, num_samples);
}

void ResonatorAudioProcessor::ProcessMidi(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi_buffer) {
    size_t const num_samples = static_cast<size_t>(buffer.getNumSamples());
    float* left_ptr = buffer.getWritePointer(0);
    float* right_ptr = buffer.getWritePointer(1);

    for (size_t i = 0; i < kNumResonators; ++i) {
        dsp_.fine_tune[i] = fine_tune_[i]->get();
    }
    dsp_.UpdateAllPitches();

    int buffer_pos = 0;
    for (auto midi : midi_buffer) {
        auto message = midi.getMessage();
        if (message.isNoteOnOrOff()) {
            dsp_.Process(left_ptr + buffer_pos, right_ptr + buffer_pos, static_cast<size_t>(midi.samplePosition - buffer_pos));
            buffer_pos = midi.samplePosition;

            if (message.isNoteOn()) {
                int resonator_idx = note_manager_.noteOn(message.getNoteNumber(), allow_round_robin_->get());
                dsp_.NoteOn(static_cast<size_t>(resonator_idx), static_cast<float>(message.getNoteNumber()), message.getFloatVelocity());
            }
            else {
                int resonator_idx = note_manager_.noteOff(message.getNoteNumber());
                dsp_.Noteoff(static_cast<size_t>(resonator_idx));
            }
        }
    }

    dsp_.Process(left_ptr + buffer_pos, right_ptr + buffer_pos, num_samples - static_cast<size_t>(buffer_pos));
}
