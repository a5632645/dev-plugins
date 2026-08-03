#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"

#include <numbers>

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
    dsp_ = steep_flanger::CreateDsp();

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    params_.BuildLayout(layout);

    value_tree_ = std::make_unique<juce::AudioProcessorValueTreeState>(*this, nullptr, "PARAMETERS", std::move(layout));
    params_.BeginListening();
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
    params_.EndListening();
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
    dsp_->Init(static_cast<float>(sampleRate));
    dsp_->Reset();

    // 首次进入时重建系数
    params_.control_.should_update_fir_ = true;
    params_.control_.should_update_iir_ = true;
}

void SteepFlangerAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

void SteepFlangerAudioProcessor::reset() {
    dsp_->Reset();
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
    juce::ScopedNoDenormals noDenormals;

    if (has_fir_source_from_state_.exchange(false)) {
        params_.control_.fir_source = fir_source_from_state_;
    }

    auto lfo_info = params_.delay_lfo.SyncBpm2(getPlayHead());
    if (lfo_info.sync_lfo) {
        dsp_->SyncPhase(lfo_info.lfo_phase);
    }
    auto barber_lfo_info = params_.barber_lfo.SyncBpm2(getPlayHead());
    if (barber_lfo_info.sync_lfo) {
        dsp_->SyncBarberPhase(barber_lfo_info.lfo_phase * std::numbers::pi_v<float> * 2);
    }

    // bpm 同步 LFO 频率（非 UI 参数，每 block 更新）
    auto p = params_.ToDspParam();
    p.lfo_freq = lfo_info.lfo_freq;
    p.barber_speed = barber_lfo_info.lfo_freq;

    dsp_->Update(p, &params_.control_);

    int num_samples = buffer.getNumSamples();
    auto* left_ptr = buffer.getWritePointer(0);
    auto* right_ptr = buffer.getWritePointer(1);

    dsp_->Process(left_ptr, right_ptr, num_samples);
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

    const juce::SpinLock::ScopedLockType lock(params_.control_.custom_coeffs_lock_);
    juce::ValueTree data{"DATA"};
    for (size_t i = 0; i < global::kMaxCoeffLen; ++i) {
        data.appendChild({
            "ITEM",
            {
                {"TIME", params_.control_.custom_coeffs_[i]},
                {"SPECTRAL", params_.control_.custom_spectral_gains[i]},
            }
        }, nullptr);
    }
    juce::ValueTree custom_coeffs{"CUSTOM_COEFFS"};
    custom_coeffs.setProperty("FIR_SOURCE", static_cast<int>(params_.control_.fir_source.load()), nullptr);
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
                const juce::SpinLock::ScopedLockType lock(params_.control_.custom_coeffs_lock_);
                std::fill_n(params_.control_.custom_coeffs_.begin(), global::kMaxCoeffLen, 0.0f);
                std::fill_n(params_.control_.custom_spectral_gains.begin(), global::kMaxCoeffLen, 0.0f);
                for (size_t i = 0; auto item : data_sections) {
                    params_.control_.custom_coeffs_[i] = static_cast<float>(item.getProperty("TIME", 0.0));
                    params_.control_.custom_spectral_gains[i] = static_cast<float>(item.getProperty("SPECTRAL", 0.0));
                    ++i;
                }
                params_.control_.should_update_fir_ = true;
            }

            int tmp = custom_coeffs.getProperty("FIR_SOURCE",
                                                static_cast<int>(steep_flanger::DspParam::FirSource::kWindowSinc));
            fir_source_from_state_ = static_cast<steep_flanger::DspParam::FirSource>(tmp);
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
