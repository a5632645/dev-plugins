#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <nlohmann/json.hpp>

constexpr auto kResultsSize = 1024;

static constexpr int kResulitionTable[] = {
    64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384
};
static const juce::StringArray kResulitionNames{
    "64", "128", "256", "512", "1024", "2048", "4096", "8192", "16384"
};

//==============================================================================
DispersiveDelayAudioProcessor::DispersiveDelayAudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
{
    curve_ = std::make_unique<mana::CurveV2>(kResultsSize, mana::CurveV2::CurveInitEnum::kRamp);
    curve_->AddListener(this);

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    {
        auto p = std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ "flat",0 },
                                                             "flat",
                                                             -50.0f, -0.1f, -0.1f);
        beta_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"f_begin",0},
                                                             "f_begin",
                                                             0.0f, 1.0f, 0.0f);
        f_begin_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"f_end",0},
                                                             "f_end",
                                                             0.0f, 1.0f, 1.0f);
        f_end_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"delay_time",0},
                                                             "delay_time",
                                                             juce::NormalisableRange<float>{0.1f, 800.0f, 0.1f, 0.4f},
                                                             20.0f);
        delay_time_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"pitch_x",0},
                                                             "pitch_x",
                                                             true);
        pitch_x_asix_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ "min_bw",0 },
                                                           "min_bw",
                                                           0.0f, 100.0f, 0.0f);
        min_bw_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{ "resolution",0 },
                                                              "resolution",
                                                              kResulitionNames,
                                                              4);
        resolution_ = p.get();
        layout.add(std::move(p));
    }

    value_tree_ = std::make_unique<juce::AudioProcessorValueTreeState>(*this, nullptr, "PARAMETERS", std::move(layout));

    value_tree_->addParameterListener("flat", this);
    value_tree_->addParameterListener("f_begin", this);
    value_tree_->addParameterListener("f_end", this);
    value_tree_->addParameterListener("delay_time", this);
    value_tree_->addParameterListener("pitch_x", this);
    value_tree_->addParameterListener("min_bw", this);
    value_tree_->addParameterListener("resolution", this);
}

DispersiveDelayAudioProcessor::~DispersiveDelayAudioProcessor()
{
    value_tree_ = nullptr;
}

//==============================================================================
const juce::String DispersiveDelayAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool DispersiveDelayAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool DispersiveDelayAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool DispersiveDelayAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double DispersiveDelayAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int DispersiveDelayAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int DispersiveDelayAudioProcessor::getCurrentProgram()
{
    return 0;
}

void DispersiveDelayAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String DispersiveDelayAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void DispersiveDelayAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void DispersiveDelayAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // why this value not set on startup
    parameterChanged(beta_->getParameterID(), beta_->get());
}

void DispersiveDelayAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool DispersiveDelayAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void DispersiveDelayAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    delays_.Process(
        buffer.getWritePointer(0),
        buffer.getWritePointer(1),
        buffer.getNumSamples()
    );
}

//==============================================================================
bool DispersiveDelayAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* DispersiveDelayAudioProcessor::createEditor()
{
    return new DispersiveDelayAudioProcessorEditor (*this);
    // return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void DispersiveDelayAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    suspendProcessing(true);
    nlohmann::json j;
    j["curve"] = curve_->SaveState();
    j["flat"] = beta_->get();
    j["min_bw"] = min_bw_->get();
    j["f_begin"] = f_begin_->get();
    j["f_end"] = f_end_->get();
    j["delay_time"] = delay_time_->get();
    j["pitch_x"] = pitch_x_asix_->get();
    j["resolution"] = resolution_->getIndex();

    auto d = j.dump();
    destData.append(d.data(), d.size());
    suspendProcessing(false);
}

inline static float GetDefaultValue(juce::AudioParameterFloat* p) {
    return static_cast<juce::RangedAudioParameter*>(p)->getDefaultValue();
}

inline static float GetDefaultValue(juce::AudioParameterInt* p) {
    return static_cast<juce::RangedAudioParameter*>(p)->getDefaultValue();
}

void DispersiveDelayAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    suspendProcessing(true);
    std::string d{ reinterpret_cast<const char*>(data), static_cast<size_t>(sizeInBytes) };
    auto bck_vt = value_tree_->copyState();
    try {
        nlohmann::json j = nlohmann::json::parse(d);
        curve_->LoadState(j["curve"]);
        f_begin_->setValueNotifyingHost(f_begin_->convertTo0to1(j.value<float>("f_begin", GetDefaultValue(f_begin_))));
        min_bw_->setValueNotifyingHost(min_bw_->convertTo0to1(j.value<float>("min_bw", GetDefaultValue(min_bw_))));
        f_end_->setValueNotifyingHost(f_end_->convertTo0to1(j.value<float>("f_end", GetDefaultValue(f_end_))));
        delay_time_->setValueNotifyingHost(delay_time_->convertTo0to1(j.value<float>("delay_time", GetDefaultValue(delay_time_))));
        if (j.contains("pitch_x")) {
            pitch_x_asix_->setValueNotifyingHost(pitch_x_asix_->convertTo0to1(j.value("pitch-x", true)));
        }
        resolution_->setValueNotifyingHost(resolution_->convertTo0to1(j.value<int>("resolution", kResulitionNames.indexOf("1024"))));
        beta_->setValueNotifyingHost(beta_->convertTo0to1(j.value<float>("flat", GetDefaultValue(beta_))));
        UpdateFilters();
    }
    catch (...) {
        if (auto* ed = getActiveEditor(); ed != nullptr) {
            juce::NativeMessageBox::showMessageBoxAsync(
                juce::MessageBoxIconType::WarningIcon,
                "Error",
                "Error loading state."
            );
        }
        value_tree_->replaceState(bck_vt);
        UpdateFilters();
    }
    suspendProcessing(false);
}

void DispersiveDelayAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == beta_->getParameterID()) {
        auto ripple = std::pow(10.0f, beta_->get() / 20.0f);
        delays_.SetBeta(ripple);
    }
    else if (parameterID == min_bw_->getParameterID()) {
        auto bw = min_bw_->get();
        delays_.SetMinBw(bw);
        UpdateFilters();
    }
    else {
        UpdateFilters();
    }
}

void DispersiveDelayAudioProcessor::UpdateFilters()
{
    const juce::ScopedLock lock{ getCallbackLock() };

    auto resolution_size = kResulitionTable[resolution_->getIndex()];
    auto f_begin = f_begin_->get();
    auto f_end = f_end_->get();
    if (f_begin > f_end) {
        std::swap(f_begin, f_end);
    }

    auto delay = delay_time_->get();
    delays_.SetCurve(*curve_,resolution_size, delay, f_begin, f_end, pitch_x_asix_->get());
}

void DispersiveDelayAudioProcessor::RandomParameter()
{
    beta_->setValueNotifyingHost(random_.nextFloat());
    min_bw_->setValueNotifyingHost(random_.nextFloat());
    f_begin_->setValueNotifyingHost(random_.nextFloat());
    f_end_->setValueNotifyingHost(random_.nextFloat());
    delay_time_->setValueNotifyingHost(random_.nextFloat());
    pitch_x_asix_->setValueNotifyingHost(random_.nextFloat());

    auto bw = min_bw_->get();
    delays_.SetMinBw(bw);
    UpdateFilters();
    auto ripple = std::pow(10.0f, beta_->get() / 20.0f);
    delays_.SetBeta(ripple);
}

void DispersiveDelayAudioProcessor::PanicFilterFb()
{
    const juce::ScopedLock lock{ getCallbackLock() };
    delays_.PaincFilterFb();
}

void DispersiveDelayAudioProcessor::OnAddPoint(mana::CurveV2* generator, mana::CurveV2::Point p, int before_idx)
{
    UpdateFilters();
}

void DispersiveDelayAudioProcessor::OnRemovePoint(mana::CurveV2* generator, int remove_idx)
{
    UpdateFilters();
}

void DispersiveDelayAudioProcessor::OnPointXyChanged(mana::CurveV2* generator, int changed_idx)
{
    UpdateFilters();
}

void DispersiveDelayAudioProcessor::OnPointPowerChanged(mana::CurveV2* generator, int changed_idx)
{
    UpdateFilters();
}

void DispersiveDelayAudioProcessor::OnReload(mana::CurveV2* generator)
{
    UpdateFilters();
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DispersiveDelayAudioProcessor();
}
