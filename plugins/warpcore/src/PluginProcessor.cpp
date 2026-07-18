#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "global.hpp"
#include "BinaryData.h"

//==============================================================================
EmptyAudioProcessor::EmptyAudioProcessor()
    : AudioProcessor(BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
      ) {
    dsp_ = warpcore::CreateDsp();

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    {
        auto p = std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{"warp", 1},
            "warp",
            1, global::kMaxBands, 50
        );
        param_listener_.Add(p, [this](int v) {
            param_.bands = v;
            param_changed_ = true;
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"f_high", 1},
            "f_high",
            juce::NormalisableRange<float>{4000.0f, 20010.0f, 0.4f}, 20010.0f,
            juce::AudioParameterFloatAttributes{}.withStringFromValueFunction([](auto x, auto maxlen) -> juce::String {
                if (x >= 20000.0f) {
                    return "Full";
                }
                else {
                    return juce::String(x, maxlen);
                }
            })
        );
        param_listener_.Add(p, [this](float v) {
            param_.f_high = v;
            param_changed_ = true;
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"scale", 1},
            "scale",
            juce::NormalisableRange<float>{0.1f, 3.0f, 0.01f}, 1.0f
        );
        param_listener_.Add(p, [this](float v) {
            param_.filter_scale = v;
            param_changed_ = true;
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"pitch", 1},
            "pitch",
            juce::NormalisableRange<float>{-24.0f, 24.0f, 0.01f}, 0.0f
        );
        param_listener_.Add(p, [this](float v) {
            param_.pitch_shift = v;
            param_changed_ = true;
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{"pitch_affect", 1},
            "pitch_affect",
            true
        );
        param_listener_.Add(p, [this](bool v) {
            param_.pitch_affect = v;
            param_changed_ = true;
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{"fill_gap", 1},
            "fill_gap",
            false
        );
        param_listener_.Add(p, [this](bool v) {
            param_.fill_gap = v;
            param_changed_ = true;
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"drywet", 1},
            "drywet",
            juce::NormalisableRange<float>{0.0f, 1.0f, 0.01f}, 1.0f
        );
        param_listener_.Add(p, [this](float v) {
            param_.drywet = v;
            param_changed_ = true;
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{"poles", 1},
            "poles",
            1, global::kMaxPoles, 2
        );
        param_listener_.Add(p, [this](int v) {
            param_.filter_order = v;
            param_changed_ = true;
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"freq_mode", 1},
            "freq_mode",
            juce::StringArray{
                "voice: 0 + n",
                "voice: 1 + n",
                "music: 0 + 2n",
                "music: 1 + 2n",
            },
            2
        );
        param_listener_.Add(p, [this](int v) {
            param_.freq_distribution = static_cast<warpcore::FreqDistrbution>(v);
            param_changed_ = true;
        });
        layout.add(std::move(p));
    }

    value_tree_ = std::make_unique<juce::AudioProcessorValueTreeState>(*this, nullptr, kParameterValueTreeIdentify,
                                                                       std::move(layout));
    preset_manager_ = std::make_unique<pluginshared::PresetManager>(*value_tree_, *this);
    preset_manager_->AddFactoryPreset(BinaryData::PiWarpLike_xml, BinaryData::PiWarpLike_xmlSize, "PiWarp Like");
    preset_manager_->AddFactoryPreset(BinaryData::WormholeLike_xml, BinaryData::WormholeLike_xmlSize, "Wormhole Like");
}

EmptyAudioProcessor::~EmptyAudioProcessor() {
    param_listener_.Clear();
    preset_manager_ = nullptr;
    value_tree_ = nullptr;
}

//==============================================================================
const juce::String EmptyAudioProcessor::getName() const {
    return JucePlugin_Name;
}

bool EmptyAudioProcessor::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool EmptyAudioProcessor::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool EmptyAudioProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double EmptyAudioProcessor::getTailLengthSeconds() const {
    return 0.0;
}

int EmptyAudioProcessor::getNumPrograms() {
    return 1; // NB: some hosts don't cope very well if you tell them there are 0 programs,
              // so this should be at least 1, even if you're not really implementing programs.
}

int EmptyAudioProcessor::getCurrentProgram() {
    return 0;
}

void EmptyAudioProcessor::setCurrentProgram(int index) {
    juce::ignoreUnused(index);
}

const juce::String EmptyAudioProcessor::getProgramName(int index) {
    juce::ignoreUnused(index);
    return {};
}

void EmptyAudioProcessor::changeProgramName(int index, const juce::String& newName) {
    juce::ignoreUnused(index, newName);
}

//==============================================================================
void EmptyAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    juce::ignoreUnused(samplesPerBlock);

    float fs = static_cast<float>(sampleRate);
    dsp_->Init(fs);
    dsp_->Reset();
    param_listener_.MarkAll();
}

void EmptyAudioProcessor::reset() {
    dsp_->Reset();
}

void EmptyAudioProcessor::releaseResources() {
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool EmptyAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
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
#if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet()) return false;
#endif

    return true;
#endif
}

void EmptyAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ignoreUnused(midiMessages);

    juce::ScopedNoDenormals noDenormals;
    param_listener_.HandleDirty();
    if (param_changed_.exchange(false)) {
        use_param_ = param_;
        dsp_->Update(use_param_);
    }

    int const num_samples = buffer.getNumSamples();
    float* left_ptr = buffer.getWritePointer(0);
    float* right_ptr = nullptr;
    if (buffer.getNumChannels() == 2) {
        right_ptr = buffer.getWritePointer(1);
    }

    dsp_->Process(left_ptr, right_ptr, num_samples);
}

//==============================================================================
bool EmptyAudioProcessor::hasEditor() const {
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* EmptyAudioProcessor::createEditor() {
    return new EmptyAudioProcessorEditor(*this);
    // return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void EmptyAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    suspendProcessing(true);

    juce::ValueTree plugin_state{"PLUGIN_STATE"};
    plugin_state.appendChild(value_tree_->copyState(), nullptr);

    if (auto xml = plugin_state.createXml(); xml != nullptr) {
        copyXmlToBinary(*xml, destData);
    }

    suspendProcessing(false);
}

void EmptyAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    suspendProcessing(true);

    auto xml = *getXmlFromBinary(data, sizeInBytes);
    auto plugin_state = juce::ValueTree::fromXml(xml);
    if (plugin_state.isValid()) {
        auto parameter = plugin_state.getChildWithName(kParameterValueTreeIdentify);
        value_tree_->replaceState(parameter);
    }

    reset();
    suspendProcessing(false);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new EmptyAudioProcessor();
}
