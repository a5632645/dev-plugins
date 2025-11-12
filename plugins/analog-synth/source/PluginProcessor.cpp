#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AnalogSynthAudioProcessor::AnalogSynthAudioProcessor()
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

    layout.add(synth_.param_osc1_detune.Build());
    layout.add(synth_.param_osc1_vol.Build());
    layout.add(synth_.param_osc1_shape.Build());
    layout.add(synth_.param_osc1_pwm.Build());
    layout.add(synth_.param_osc2_detune.Build());
    layout.add(synth_.param_osc2_vol.Build());
    layout.add(synth_.param_osc2_shape.Build());
    layout.add(synth_.param_osc2_pwm.Build());
    layout.add(synth_.param_osc2_sync.Build());
    layout.add(synth_.param_osc3_detune.Build());
    layout.add(synth_.param_osc3_vol.Build());
    layout.add(synth_.param_osc3_shape.Build());
    layout.add(synth_.param_osc3_pwm.Build());
    layout.add(synth_.param_osc3_unison.Build());
    layout.add(synth_.param_osc3_unison_detune.Build());
    layout.add(synth_.param_osc3_uniso_type.Build());
    layout.add(synth_.param_osc3_retrigger.Build());
    layout.add(synth_.param_osc3_phase.Build());
    layout.add(synth_.param_osc3_phase_random.Build());
    layout.add(synth_.param_osc4_slope.Build());
    layout.add(synth_.param_osc4_width.Build());
    layout.add(synth_.param_osc4_n.Build());
    layout.add(synth_.param_osc4_w0_detune.Build());
    layout.add(synth_.param_osc4_w_ratio.Build());
    layout.add(synth_.param_osc4_use_max_n.Build());
    layout.add(synth_.param_osc4_shape.Build());
    layout.add(synth_.param_osc4_vol.Build());
    layout.add(synth_.param_noise_type.Build());
    layout.add(synth_.param_noise_vol.Build());
    layout.add(synth_.param_env_volume_attack.Build());
    layout.add(synth_.param_env_volume_decay.Build());
    layout.add(synth_.param_env_volume_sustain.Build());
    layout.add(synth_.param_env_volume_release.Build());
    layout.add(synth_.param_env_volume_exp.Build());
    layout.add(synth_.param_env_mod_attack.Build());
    layout.add(synth_.param_env_mod_decay.Build());
    layout.add(synth_.param_env_mod_sustain.Build());
    layout.add(synth_.param_env_mod_release.Build());
    layout.add(synth_.param_env_mod_exp.Build());
    layout.add(synth_.param_cutoff_pitch.Build());
    layout.add(synth_.param_Q.Build());
    layout.add(synth_.param_filter_direct.Build());
    layout.add(synth_.param_filter_lp.Build());
    layout.add(synth_.param_filter_bp.Build());
    layout.add(synth_.param_filter_hp.Build());
    layout.add(synth_.param_lfo1_freq.Build1());
    layout.add(synth_.param_lfo1_freq.Build2());
    layout.add(synth_.param_lfo1_freq.Build3());
    layout.add(synth_.param_lfo1_shape.Build());
    layout.add(synth_.param_lfo1_retrigger.Build());
    layout.add(synth_.param_lfo1_phase.Build());
    layout.add(synth_.param_lfo2_freq.Build1());
    layout.add(synth_.param_lfo2_freq.Build2());
    layout.add(synth_.param_lfo2_freq.Build3());
    layout.add(synth_.param_lfo2_shape.Build());
    layout.add(synth_.param_lfo2_retrigger.Build());
    layout.add(synth_.param_lfo2_phase.Build());
    layout.add(synth_.param_lfo3_freq.Build1());
    layout.add(synth_.param_lfo3_freq.Build2());
    layout.add(synth_.param_lfo3_freq.Build3());
    layout.add(synth_.param_lfo3_shape.Build());
    layout.add(synth_.param_lfo3_retrigger.Build());
    layout.add(synth_.param_lfo3_phase.Build());
    layout.add(synth_.param_delay_enable.Build());
    layout.add(synth_.param_delay_ms.Build());
    layout.add(synth_.param_delay_pingpong.Build());
    layout.add(synth_.param_delay_feedback.Build());
    layout.add(synth_.param_delay_lp.Build());
    layout.add(synth_.param_delay_hp.Build());
    layout.add(synth_.param_delay_mix.Build());
    layout.add(synth_.param_chorus_enable.Build());
    layout.add(synth_.param_chorus_delay.Build());
    layout.add(synth_.param_chorus_feedback.Build());
    layout.add(synth_.param_chorus_mix.Build());
    layout.add(synth_.param_chorus_depth.Build());
    layout.add(synth_.param_chorus_rate.Build1());
    layout.add(synth_.param_chorus_rate.Build2());
    layout.add(synth_.param_chorus_rate.Build3());
    layout.add(synth_.param_distortion_drive.Build());
    layout.add(synth_.param_distortion_enable.Build());
    layout.add(synth_.param_reverb_enable.Build());
    layout.add(synth_.param_reverb_mix.Build());
    layout.add(synth_.param_reverb_predelay.Build());
    layout.add(synth_.param_reverb_lowpass.Build());
    layout.add(synth_.param_reverb_decay.Build());
    layout.add(synth_.param_reverb_size.Build());
    layout.add(synth_.param_reverb_damp.Build());
    layout.add(synth_.param_phaser_enable.Build());
    layout.add(synth_.param_phaser_mix.Build());
    layout.add(synth_.param_phaser_center.Build());
    layout.add(synth_.param_phaser_depth.Build());
    layout.add(synth_.param_phaser_rate.Build1());
    layout.add(synth_.param_phaser_rate.Build2());
    layout.add(synth_.param_phaser_rate.Build3());
    layout.add(synth_.param_phase_feedback.Build());
    layout.add(synth_.param_phaser_Q.Build());
    layout.add(synth_.param_phaser_stereo.Build());

    value_tree_ = std::make_unique<juce::AudioProcessorValueTreeState>(*this, nullptr, "PARAMETERS", std::move(layout));
    
    preset_manager_ = std::make_unique<pluginshared::PresetManager>(*value_tree_, *this);
    preset_manager_->external_load_default_operations = [this] {
        synth_.modulation_matrix.RemoveAll();
    };
}

AnalogSynthAudioProcessor::~AnalogSynthAudioProcessor()
{
    value_tree_ = nullptr;
}

//==============================================================================
const juce::String AnalogSynthAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AnalogSynthAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AnalogSynthAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AnalogSynthAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AnalogSynthAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AnalogSynthAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AnalogSynthAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AnalogSynthAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String AnalogSynthAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void AnalogSynthAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void AnalogSynthAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    std::ignore = samplesPerBlock;
    synth_.Init(static_cast<float>(sampleRate));
    synth_.Reset();
    param_listener_.CallAll();
}

void AnalogSynthAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool AnalogSynthAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void AnalogSynthAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    synth_.SyncBpm(*this);
    synth_.Process(buffer, midiMessages);
}

//==============================================================================
bool AnalogSynthAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AnalogSynthAudioProcessor::createEditor()
{
    return new AnalogSynthAudioProcessorEditor (*this);
    // return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void AnalogSynthAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    suspendProcessing(true);
    juce::ValueTree state{"State"};
    state.appendChild(value_tree_->copyState(), nullptr);
    state.appendChild(synth_.modulation_matrix.SaveState(), nullptr);
    state.appendChild(synth_.SaveFxChainState(), nullptr);
    if (auto xml = state.createXml(); xml != nullptr) {
        copyXmlToBinary(*xml, destData);
    }
    suspendProcessing(false);
}

void AnalogSynthAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    suspendProcessing(true);
    auto xml = *getXmlFromBinary(data, sizeInBytes);
    auto state = juce::ValueTree::fromXml(xml);
    if (state.isValid()) {
        value_tree_->replaceState(state.getChildWithName("PARAMETERS"));
        synth_.modulation_matrix.LoadState(state);
        synth_.LoadFxChainState(state);
    }
    suspendProcessing(false);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AnalogSynthAudioProcessor();
}
