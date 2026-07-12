#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"

//==============================================================================
SteepFlangerAudioProcessor::SteepFlangerAudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
{
    dsp_processor_ = dsp::GetProcessorDsp();

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // lfo
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"delay", 1},
            "delay",
            juce::NormalisableRange<float>{0.0f, global::kMaxDelayMs, 0.01f},
            1.0f
        );
        param_delay_ms_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"depth", 1},
            "depth",
            juce::NormalisableRange<float>{0.0f, global::kModuDelayMs, 0.01f},
            1.0f
        );
        param_delay_depth_ms_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto[pfreq, ptype] = delay_lfo_state_.Build("speed", 0, 10, 0.01f, 0.4f, true, "0", "1/64T", 0.2f, "4", false);
        layout.add(std::move(pfreq));
        layout.add(std::move(ptype));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"phase", 1},
            "phase",
            juce::NormalisableRange<float>{0.0f, 1.0f, 0.01f},
            0.03f
        );
        param_lfo_phase_ = p.get();
        layout.add(std::move(p));
    }

    // fir design
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"cutoff", 1},
            "cutoff",
            juce::NormalisableRange<float>{0.01f, 3.0f, 0.01f},
            std::numbers::pi_v<float> / 2
        );
        param_fir_cutoff_ = p.get();
        param_listener_.Add(p, [this](float) {
            dsp_state_.param.fir_source = dsp::DspParam::kWindowSinc;
            dsp_state_.param.should_update_fir_ = true;
            dsp_state_.param.should_update_iir_ = true;
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"coeff_len", 1},
            "coeff_len",
            juce::NormalisableRange<float>{4.0f, static_cast<float>(global::kMaxCoeffLen), 1.0f},
            8.0f
        );
        param_fir_coeff_len_ = p.get();
        param_listener_.Add(p, [this](float) {
            dsp_state_.param.should_update_fir_ = true;
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"side_lobe", 1},
            "side_lobe",
            juce::NormalisableRange<float>{20.0f, 100.0f, 0.1f},
            40.0f
        );
        param_fir_side_lobe_ = p.get();
        param_listener_.Add(p, [this](float) {
            dsp_state_.param.fir_source = dsp::DspParam::kWindowSinc;
            dsp_state_.param.should_update_fir_ = true;
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{"minum_phase", 1},
            "minum_phase",
            false
        );
        param_fir_min_phase_ = p.get();
        param_listener_.Add(p, [this](bool) {
            dsp_state_.param.should_update_fir_ = true;
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{"highpass", 1},
            "highpass",
            false
        );
        param_fir_highpass_ = p.get();
        param_listener_.Add(p, [this](bool) {
            dsp_state_.param.fir_source = dsp::DspParam::kWindowSinc;
            dsp_state_.param.should_update_fir_ = true;
            dsp_state_.param.should_update_iir_ = true;
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"drywet", 1},
            "drywet",
            0.0f, 1.0f, 1.0f
        );
        param_drywet_ = p.get();
        layout.add(std::move(p));
    }

    // feedback
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"fb_value", 1},
            "fb_value",
            juce::NormalisableRange<float>{-0.95f, 0.95f, 0.01f},
            0.0f
        );
        param_feedback_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"fb_damp", 1},
            "fb_damp",
            0.0f, 140.0f,
            90.0f
        );
        param_damp_pitch_ = p.get();
        layout.add(std::move(p));
    }

    // barberpole
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"barber_phase", 1},
            "barber_phase",
            juce::NormalisableRange<float>{0.0f, 1.0f, 0.01f},
            0.0f
        );
        param_barber_phase_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"barber_stereo", 1},
            "barber_stereo",
            juce::NormalisableRange<float>{0.0f, 1.0f, 0.01f},
            0.0f
        );
        param_barber_stereo_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto[p, p2] = barber_lfo_state_.Build("barber_speed", -10.0f, 10.0f, 0.01f, 0.4f, true, "-1/64T", "1/64T", 0.2f, "1", false);
        layout.add(std::move(p));
        layout.add(std::move(p2));
    }
    {
        auto p = std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{"barber_enable", 1},
            "barber_enable",
            false
        );
        param_barber_enable_ = p.get();
        layout.add(std::move(p));
    }

    // -------------------- iir --------------------
    {
        auto p = std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{"iir_enable", 1},
            "iir_enable",
            false
        );
        param_iir_mode_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{"iir_filter_num", 1},
            "iir_filter_num",
            1, global::kIirMaxNumFilters,
            4
        );
        param_iir_filter_num_ = p.get();
        param_listener_.Add(p, [this](float) {
            dsp_state_.param.should_update_iir_ = true;
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ripple", 1},
            "ripple",
            juce::NormalisableRange<float>{0.1f, 20.0f, 0.1f},
            1.0f
        );
        param_iir_ripple_ = p.get();
        param_listener_.Add(p, [this](float) {
            dsp_state_.param.should_update_iir_ = true;
        });
        layout.add(std::move(p));
    }

    value_tree_ = std::make_unique<juce::AudioProcessorValueTreeState>(*this, nullptr, "PARAMETERS", std::move(layout));
    preset_manager_ = std::make_unique<pluginshared::PresetManager>(*value_tree_, *this);
    preset_manager_->AddFactoryPreset(BinaryData::Flanger_xml, BinaryData::Flanger_xmlSize, "Flanger");
    preset_manager_->AddFactoryPreset(BinaryData::BarberpoleFlanger_xml, BinaryData::BarberpoleFlanger_xmlSize, "Barberpole Flanger");
    preset_manager_->AddFactoryPreset(BinaryData::HeavyBarberpole_xml, BinaryData::HeavyBarberpole_xmlSize, "Heary Barberpole");
    preset_manager_->AddFactoryPreset(BinaryData::IIRFlanger_xml, BinaryData::IIRFlanger_xmlSize, "IIR Flanger");
    preset_manager_->AddFactoryPreset(BinaryData::IIRBarberpole_xml, BinaryData::IIRBarberpole_xmlSize, "IIR Barberpole");
    preset_manager_->AddFactoryPreset(BinaryData::FIRResonator_xml, BinaryData::FIRResonator_xmlSize, "FIR Resonator");
    preset_manager_->AddFactoryPreset(BinaryData::IIRResonator_xml, BinaryData::IIRResonator_xmlSize, "IIR Resonator");
    preset_manager_->AddFactoryPreset(BinaryData::wormhole_robot_xml, BinaryData::wormhole_robot_xmlSize, "WormholeRobot");
    preset_manager_->AddFactoryPreset(BinaryData::YOIuse_cutoff_xml, BinaryData::YOIuse_cutoff_xmlSize, "YOI(use cutoff)");
    preset_manager_->AddFactoryPreset(BinaryData::dispersion_voice_xml, BinaryData::dispersion_voice_xmlSize, "Dispersion Voice");
}

SteepFlangerAudioProcessor::~SteepFlangerAudioProcessor()
{
    param_listener_.Clear();
    value_tree_ = nullptr;
}

//==============================================================================
const juce::String SteepFlangerAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SteepFlangerAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SteepFlangerAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SteepFlangerAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SteepFlangerAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SteepFlangerAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SteepFlangerAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SteepFlangerAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String SteepFlangerAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void SteepFlangerAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void SteepFlangerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    if (!dsp_processor_.IsValid()) return;

    dsp_processor_.init(dsp_state_, static_cast<float>(sampleRate));
    dsp_processor_.reset(dsp_state_);

    param_listener_.MarkAll();
}

void SteepFlangerAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

void SteepFlangerAudioProcessor::reset() {
    if (dsp_processor_.IsValid()) {
        dsp_processor_.reset(dsp_state_);
    }
}

bool SteepFlangerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void SteepFlangerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    if (!dsp_processor_.IsValid()) return;

    juce::ScopedNoDenormals noDenormals;
    param_listener_.HandleDirty();

    if (has_fir_source_from_state_.exchange(false)) {
        dsp_state_.param.fir_source = fir_source_from_state_;
    }

    auto lfo_info = delay_lfo_state_.SyncBpm2(getPlayHead());
    if (lfo_info.sync_lfo) {
        dsp_state_.phase_ = lfo_info.lfo_phase;
    }
    auto barber_lfo_info = barber_lfo_state_.SyncBpm2(getPlayHead());
    if (barber_lfo_info.sync_lfo) {
        dsp_state_.barber_oscillator_.Reset(barber_lfo_info.lfo_phase * std::numbers::pi_v<float> * 2);
    }

    dsp_state_.param.delay_ms = param_delay_ms_->get();
    dsp_state_.param.depth_ms = param_delay_depth_ms_->get();
    dsp_state_.param.lfo_freq = lfo_info.lfo_freq;
    dsp_state_.param.lfo_phase = param_lfo_phase_->get();
    dsp_state_.param.fir_cutoff = param_fir_cutoff_->get();
    dsp_state_.param.fir_coeff_len = static_cast<size_t>(param_fir_coeff_len_->get());
    dsp_state_.param.fir_side_lobe = param_fir_side_lobe_->get();
    dsp_state_.param.fir_min_phase = param_fir_min_phase_->get();
    dsp_state_.param.fir_highpass = param_fir_highpass_->get();
    dsp_state_.param.feedback = param_feedback_->get();
    dsp_state_.param.damp_pitch = param_damp_pitch_->get();
    dsp_state_.param.barber_phase = param_barber_phase_->get();
    dsp_state_.param.barber_speed = barber_lfo_info.lfo_freq;
    dsp_state_.param.barber_enable = param_barber_enable_->get();
    dsp_state_.param.barber_stereo_phase = param_barber_stereo_->get() * std::numbers::pi_v<float> / 2;
    dsp_state_.param.drywet = param_drywet_->get();
    dsp_state_.param.iir_num_filters = static_cast<size_t>(param_iir_filter_num_->get());
    dsp_state_.param.ripple = param_iir_ripple_->get();
    dsp_state_.param.iir_mode = param_iir_mode_->get();
    // dsp_processor_.update(dsp_state_, dsp_state_.param);

    int num_samples = buffer.getNumSamples();
    auto* left_ptr = buffer.getWritePointer(0);
    auto* right_ptr = buffer.getWritePointer(1);

    dsp_processor_.process(dsp_state_, left_ptr, right_ptr, num_samples);
}

//==============================================================================
bool SteepFlangerAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SteepFlangerAudioProcessor::createEditor()
{
    return new EmptyAudioProcessorEditor (*this);
}

//==============================================================================
void SteepFlangerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    suspendProcessing(true);
    auto& dsp_param_ = dsp_state_.param;

    const juce::SpinLock::ScopedLockType lock(dsp_state_.param.custom_coeffs_lock_);
    juce::ValueTree data{"DATA"};
    for (size_t i = 0; i < global::kMaxCoeffLen; ++i) {
        data.appendChild({
            "ITEM",
            {
                {"TIME", dsp_state_.param.custom_coeffs_[i]},
                {"SPECTRAL", dsp_state_.param.custom_spectral_gains[i]},
            }
        }, nullptr);
    }
    juce::ValueTree custom_coeffs{"CUSTOM_COEFFS"};
    custom_coeffs.setProperty("FIR_SOURCE", static_cast<int>(dsp_state_.param.fir_source), nullptr);
    custom_coeffs.appendChild(data, nullptr);

    juce::ValueTree plugin_state{"PLUGIN_STATE"};
    plugin_state.appendChild(value_tree_->copyState(), nullptr);
    plugin_state.appendChild(custom_coeffs, nullptr);

    if (auto xml = plugin_state.createXml(); xml != nullptr) {
        copyXmlToBinary(*xml, destData);
    }

    suspendProcessing(false);
}

void SteepFlangerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    suspendProcessing(true);
    auto& dsp_param_ = dsp_state_.param;

    auto xml = *getXmlFromBinary(data, sizeInBytes);
    auto plugin_state = juce::ValueTree::fromXml(xml);

    if (plugin_state.isValid()) {
        auto parameters = plugin_state.getChildWithName("PARAMETERS");
        if (parameters.isValid()) {
            value_tree_->replaceState(parameters);
        }

        auto custom_coeffs = plugin_state.getChildWithName("CUSTOM_COEFFS");
        if (custom_coeffs.isValid()) {
            auto data_sections = custom_coeffs.getChildWithName("DATA");
            if (data_sections.isValid()) {
                const juce::SpinLock::ScopedLockType lock(dsp_state_.param.custom_coeffs_lock_);
                std::fill_n(dsp_state_.param.custom_coeffs_.begin(), global::kMaxCoeffLen, 0.0f);
                std::fill_n(dsp_state_.param.custom_spectral_gains.begin(), global::kMaxCoeffLen, 0.0f);
                for (size_t i = 0; auto item : data_sections) {
                    dsp_state_.param.custom_coeffs_[i] = static_cast<float>(item.getProperty("TIME", 0.0));
                    dsp_state_.param.custom_spectral_gains[i] = static_cast<float>(item.getProperty("SPECTRAL", 0.0));
                    ++i;
                }
                dsp_state_.param.should_update_fir_ = true;
            }

            int tmp = custom_coeffs.getProperty("FIR_SOURCE", static_cast<int>(dsp::DspParam::FirSource::kWindowSinc));
            fir_source_from_state_ = static_cast<dsp::DspParam::FirSource>(tmp);
            has_fir_source_from_state_ = true;
        }
    }

    reset();
    suspendProcessing(false);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SteepFlangerAudioProcessor();
}
