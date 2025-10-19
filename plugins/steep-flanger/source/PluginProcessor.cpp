#include "PluginProcessor.h"
#include "PluginEditor.h"

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
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // lfo
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"delay", 1},
            "delay",
            juce::NormalisableRange<float>{0.0f, 20.0f, 0.01f},
            1.0f
        );
        param_delay_ms_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"depth", 1},
            "depth",
            juce::NormalisableRange<float>{0.0f, 10.0f, 0.01f},
            1.0f
        );
        param_delay_depth_ms_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"speed", 1},
            "speed",
            juce::NormalisableRange<float>{0.0f, 10.0f, 0.01f},
            0.3f
        );
        param_lfo_speed_ = p.get();
        layout.add(std::move(p));
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
            juce::NormalisableRange<float>{0.1415926f, 3.0f, 0.01f},
            std::numbers::pi_v<float> / 2
        );
        param_fir_cutoff_ = p.get();
        param_listener_.Add(p, [this](float) {
            dsp_param_.is_using_custom_ = false;
            dsp_param_.should_update_fir_ = true;
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"coeff_len", 1},
            "coeff_len",
            juce::NormalisableRange<float>{4.0f, static_cast<float>(kMaxCoeffLen), 1.0f},
            8.0f
        );
        param_fir_coeff_len_ = p.get();
        param_listener_.Add(p, [this](float) {
            dsp_param_.should_update_fir_ = true;
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
            dsp_param_.is_using_custom_ = false;
            dsp_param_.should_update_fir_ = true;
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
            dsp_param_.should_update_fir_ = true;
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
            dsp_param_.is_using_custom_ = false;
            dsp_param_.should_update_fir_ = true;
        });
        layout.add(std::move(p));
    }

    // feedback
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"fb_value", 1},
            "fb_value",
            juce::NormalisableRange<float>{-20.1f, 20.1f, 0.1f},
            -20.1f
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
    {
        auto p = std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{"fb_enable", 1},
            "fb_enable",
            false
        );
        param_feedback_enable_ = p.get();
        param_listener_.Add(p, [this](bool) {
            dsp_param_.should_update_fir_ = true;
        });
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
            juce::ParameterID{"barber_speed", 1},
            "barber_speed",
            juce::NormalisableRange<float>{-10.0f, 10.0f, 0.01f},
            0.0f
        );
        param_barber_speed_ = p.get();
        layout.add(std::move(p));
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

    value_tree_ = std::make_unique<juce::AudioProcessorValueTreeState>(*this, nullptr, "PARAMETERS", std::move(layout));
    preset_manager_ = std::make_unique<pluginshared::PresetManager>(*value_tree_, *this);
    preset_manager_->external_load_default_operations = [this]{
        dsp_param_.is_using_custom_ = false;
        std::ranges::fill(dsp_param_.custom_coeffs_, float{});
        std::ranges::fill(dsp_param_.custom_spectral_gains, float{});
    };
}

SteepFlangerAudioProcessor::~SteepFlangerAudioProcessor()
{
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
    std::ignore = samplesPerBlock;
    
    dsp_.Init(static_cast<float>(sampleRate), 30.0f);
    dsp_.Reset();
    dsp_param_.should_update_fir_ = true;
}

void SteepFlangerAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
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

void SteepFlangerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    std::ignore = midiMessages;

    juce::ScopedNoDenormals noDenormals;

    dsp_param_.delay_ms = param_delay_ms_->get();
    dsp_param_.depth_ms = param_delay_depth_ms_->get();
    dsp_param_.lfo_freq = param_lfo_speed_->get();
    dsp_param_.lfo_phase = param_lfo_phase_->get();
    dsp_param_.fir_cutoff = param_fir_cutoff_->get();
    dsp_param_.fir_coeff_len = static_cast<size_t>(param_fir_coeff_len_->get());
    dsp_param_.fir_side_lobe = param_fir_side_lobe_->get();
    dsp_param_.fir_min_phase = param_fir_min_phase_->get();
    dsp_param_.fir_highpass = param_fir_highpass_->get();
    dsp_param_.feedback = param_feedback_->get();
    dsp_param_.damp_pitch = param_damp_pitch_->get();
    dsp_param_.feedback_enable = param_feedback_enable_->get();
    dsp_param_.barber_phase = param_barber_phase_->get();
    dsp_param_.barber_speed = param_barber_speed_->get();
    dsp_param_.barber_enable = param_barber_enable_->get();

    size_t const len = static_cast<size_t>(buffer.getNumSamples());
    auto* left_ptr = buffer.getWritePointer(0);
    auto* right_ptr = buffer.getWritePointer(1);

    dsp_.Process(left_ptr, right_ptr, len, dsp_param_);
}

//==============================================================================
bool SteepFlangerAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SteepFlangerAudioProcessor::createEditor()
{
    return new SteepFlangerAudioProcessorEditor (*this);
    // return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void SteepFlangerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    suspendProcessing(true);
    if (auto state = value_tree_->copyState().createXml(); state != nullptr) {
        auto custom_coeffs = state->createNewChildElement("CUSTOM_COEFFS");
        custom_coeffs->setAttribute("USING", dsp_param_.is_using_custom_);
        auto data = custom_coeffs->createNewChildElement("DATA");
        for (size_t i = 0; i < kMaxCoeffLen; ++i) {
            auto time = data->createNewChildElement("ITEM");
            time->setAttribute("TIME", dsp_param_.custom_coeffs_[i]);
            time->setAttribute("SPECTRAL", dsp_param_.custom_spectral_gains[i]);
        }
        copyXmlToBinary(*state, destData);
    }
    suspendProcessing(false);
}

void SteepFlangerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    suspendProcessing(true);
    auto xml = *getXmlFromBinary(data, sizeInBytes);
    auto state = juce::ValueTree::fromXml(xml);
    if (state.isValid()) {
        value_tree_->replaceState(state);
        auto coeffs = xml.getChildByName("CUSTOM_COEFFS");
        if (coeffs) {
            dsp_param_.is_using_custom_ = coeffs->getBoolAttribute("USING", false);
            auto data_sections = coeffs->getChildByName("DATA");
            if (data_sections) {
                auto it = data_sections->getChildIterator();
                for (size_t i = 0; auto item : it) {
                    dsp_param_.custom_coeffs_[i] = static_cast<float>(item->getDoubleAttribute("TIME"));
                    dsp_param_.custom_spectral_gains[i] = static_cast<float>(item->getDoubleAttribute("SPECTRAL"));
                    ++i;
                    // protect loading old version 65 length coeffs
                    if (i == kMaxCoeffLen) break;
                }
                dsp_param_.should_update_fir_ = true;
            }
        }
    }
    suspendProcessing(false);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SteepFlangerAudioProcessor();
}
