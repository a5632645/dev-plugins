#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "qwqdsp/filter/window_fir.hpp"
#include "qwqdsp/window/kaiser.hpp"
#include "qwqdsp/polymath.hpp"
#include "qwqdsp/convert.hpp"
#include "qwqdsp/filter/rbj.hpp"

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
        param_listener_.Add(p, [this](float delay) {
            juce::ScopedLock _{getCallbackLock()};
            delay_samples_ = delay * getSampleRate() / 1000.0f;
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"depth", 1},
            "depth",
            juce::NormalisableRange<float>{0.0f, 10.0f, 0.01f},
            1.0f
        );
        param_listener_.Add(p, [this](float depth) {
            juce::ScopedLock _{getCallbackLock()};
            depth_samples_ = depth * getSampleRate() / 1000.0f;
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"speed", 1},
            "speed",
            juce::NormalisableRange<float>{0.0f, 10.0f, 0.01f},
            0.3f
        );
        param_listener_.Add(p, [this](float speed) {
            juce::ScopedLock _{getCallbackLock()};
            phase_inc_ = speed / getSampleRate();
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"phase", 1},
            "phase",
            juce::NormalisableRange<float>{0.0f, 1.0f, 0.01f},
            0.03f
        );
        param_listener_.Add(p, [this](float phase) {
            juce::ScopedLock _{getCallbackLock()};
            phase_shift_ = phase;
        });
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
        param_listener_.Add(p, [this](float cutoff) {
            juce::ScopedLock _{getCallbackLock()};
            cutoff_w_ = cutoff;
            UpdateCoeff();
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
        param_listener_.Add(p, [this](float coeff_len) {
            juce::ScopedLock _{getCallbackLock()};
            coeff_len_ = coeff_len;
            UpdateCoeff();
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
        param_listener_.Add(p, [this](float side_lobe) {
            juce::ScopedLock _{getCallbackLock()};
            side_lobe_ = side_lobe;
            UpdateCoeff();
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{"minum_phase", 1},
            "minum_phase",
            false
        );
        param_listener_.Add(p, [this](bool minum_phase) {
            juce::ScopedLock _{getCallbackLock()};
            minum_phase_ = minum_phase;
            UpdateCoeff();
            UpdateFeedback();
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{"highpass", 1},
            "highpass",
            false
        );
        param_listener_.Add(p, [this](bool highpass) {
            juce::ScopedLock _{getCallbackLock()};
            highpass_ = highpass;
            UpdateCoeff();
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
        param_listener_.Add(p, [this](float fb_value) {
            juce::ScopedLock _{getCallbackLock()};
            feedback_value_ = fb_value;
            UpdateFeedback();
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"fb_damp", 1},
            "fb_damp",
            juce::NormalisableRange<float>{50.0f, 20010.0f, 0.1f},
            5000.0f
        );
        param_listener_.Add(p, [this](float damp_freq) {
            juce::ScopedLock _{getCallbackLock()};
            if (damp_freq > 20000.0f) {
                left_damp_.Set(1, 0, 0, 0, 0);
            }
            else {
                float const w = qwqdsp::convert::Freq2W(damp_freq, getSampleRate());
                qwqdsp::filter::RBJ design;
                design.Lowpass(w, std::numbers::sqrt2_v<float> / 2);
                left_damp_.Set(design.b0, design.b1, design.b2, design.a1, design.a2);
            }
            right_damp_.Copy(left_damp_);
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{"fb_enable", 1},
            "fb_enable",
            false
        );
        param_listener_.Add(p, [this](bool fb_enable) {
            juce::ScopedLock _{getCallbackLock()};
            feedback_enable_ = fb_enable;
            UpdateCoeff();
            UpdateFeedback();
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
        param_listener_.Add(p, [this](float barber_phase) {
            juce::ScopedLock _{getCallbackLock()};
            barber_phase_smoother_.SetTarget(barber_phase);
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"barber_speed", 1},
            "barber_speed",
            juce::NormalisableRange<float>{-10.0f, 10.0f, 0.01f},
            0.0f
        );
        param_listener_.Add(p, [this](float barber_speed) {
            juce::ScopedLock _{getCallbackLock()};
            barber_phase_inc_ = barber_speed / getSampleRate();
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{"barber_enable", 1},
            "barber_enable",
            false
        );
        param_listener_.Add(p, [this](bool barber_enable) {
            juce::ScopedLock _{getCallbackLock()};
            barber_enable_ = barber_enable;
        });
        layout.add(std::move(p));
    }

    value_tree_ = std::make_unique<juce::AudioProcessorValueTreeState>(*this, nullptr, "PARAMETERS", std::move(layout));

    complex_fft_.Init(kFFTSize);
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
    float const samples_need = sampleRate * 35.0f / 1000.0f;
    delay_left_.Init(samples_need * kMaxCoeffLen);
    delay_right_.Init(samples_need * kMaxCoeffLen);
    left_delay_smoother_.SetSmoothTime(20.0f, sampleRate);
    right_delay_smoother_.SetSmoothTime(20.0f, sampleRate);
    barber_phase_smoother_.SetSmoothTime(20.0f, sampleRate);
    param_listener_.CallAll();
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
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    size_t const len = buffer.getNumSamples();
    // std::span<float> left{buffer.getWritePointer(0), len};
    // std::span<float> right{buffer.getWritePointer(1), len};
    auto* left_ptr = buffer.getWritePointer(0);
    auto* right_ptr = buffer.getWritePointer(1);

    size_t cando = len;
    while (cando != 0) {
        size_t num_process = std::min<size_t>(64, cando);
        cando -= num_process;

        // update lfo
        phase_ += phase_inc_ * num_process;
        float right_phase = phase_ + phase_shift_;
        {
            float t;
            phase_ = std::modf(phase_, &t);
            right_phase = std::modf(right_phase, &t);
        }
        float lfo_sine = qwqdsp::polymath::SinPi(phase_ * std::numbers::pi_v<float>);
        float delay = delay_samples_ + lfo_sine * depth_samples_;
        delay = std::max(0.0f, delay);
        left_delay_smoother_.SetTarget(delay);

        lfo_sine = qwqdsp::polymath::SinPi(right_phase * std::numbers::pi_v<float>);
        delay = delay_samples_ + lfo_sine * depth_samples_;
        delay = std::max(0.0f, delay);
        right_delay_smoother_.SetTarget(delay);

        // fir polyphase filtering
        if (!barber_enable_) {
            for (size_t i = 0; i < num_process; ++i) {
                delay_left_.Push(*left_ptr + left_fb_ * feedback_mul_);
    
                float sum = 0;
                float const num_notch = left_delay_smoother_.Tick();
                for (size_t i = 0; i < coeff_len_; ++i) {
                    sum += coeffs_[i] * delay_left_.GetAfterPush(i * num_notch);
                }
    
                left_fb_ = left_damp_.Tick(sum);
                *left_ptr = sum;
                ++left_ptr;
            }
            for (size_t i = 0; i < num_process; ++i) {
                delay_right_.Push(*right_ptr + right_fb_ * feedback_mul_);
    
                float sum = 0;
                float const num_notch = right_delay_smoother_.Tick();
                for (size_t i = 0; i < coeff_len_; ++i) {
                    sum += coeffs_[i] * delay_right_.GetAfterPush(i * num_notch);
                }
    
                right_fb_ = right_damp_.Tick(sum);
                *right_ptr = sum;
                ++right_ptr;
            }
        }
        else {
            for (size_t i = 0; i < num_process; ++i) {
                delay_left_.Push(*left_ptr + left_fb_ * feedback_mul_);
                delay_right_.Push(*right_ptr + right_fb_ * feedback_mul_);
    
                barber_phase_ += barber_phase_inc_;
                barber_phase_ -= std::floor(barber_phase_);
                float barber = barber_phase_ + barber_phase_smoother_.Tick();
                barber -= std::floor(barber);
    
                float const left_num_notch = left_delay_smoother_.Tick();
                float const right_num_notch = right_delay_smoother_.Tick();
                auto const left_rotation_mul = std::polar(1.0f, barber * std::numbers::pi_v<float> * 2);
                auto const right_rotation_mul = std::polar(1.0f, barber * std::numbers::pi_v<float> * 2);
                std::complex<float> left_rotation_coeff = 1;
                std::complex<float> right_rotation_coeff = 1;
                std::complex<float> left_sum = 0;
                std::complex<float> right_sum = 0;
                for (size_t i = 0; i < coeff_len_; ++i) {
                    left_sum += left_rotation_coeff * coeffs_[i] * delay_left_.GetAfterPush(i * left_num_notch);
                    right_sum += right_rotation_coeff * coeffs_[i] * delay_right_.GetAfterPush(i * right_num_notch);
                    left_rotation_coeff *= left_rotation_mul;
                    right_rotation_coeff *= right_rotation_mul;
                }
                
                float const left_out = left_hilbert_.Tick(left_sum).real();
                float const right_out = right_hilbert_.Tick(right_sum).real();
                left_fb_ = left_damp_.Tick(left_out);
                right_fb_ = right_damp_.Tick(right_out);
                *left_ptr = left_out;
                *right_ptr = right_out;
                ++left_ptr;
                ++right_ptr;
            }
        }
    }
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
    if (auto state = value_tree_->copyState().createXml()) {
        copyXmlToBinary(*state, destData);
    }
}

void SteepFlangerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto state = juce::ValueTree::fromXml(*getXmlFromBinary(data, sizeInBytes));
    if (state.isValid()) {
        value_tree_->replaceState(state);
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SteepFlangerAudioProcessor();
}




void SteepFlangerAudioProcessor::UpdateCoeff() {
    if (highpass_) {
        coeff_len_ |= 1;
    }
    
    std::span<float> kernel{coeffs_.data(), coeff_len_};
    if (highpass_) {
        qwqdsp::filter::WindowFIR::Highpass(kernel, cutoff_w_);
    }
    else {
        qwqdsp::filter::WindowFIR::Lowpass(kernel, cutoff_w_);
    }
    float const beta = qwqdsp::window::Kaiser::Beta(side_lobe_);
    qwqdsp::window::Kaiser::ApplyWindow(kernel, beta, false);

    if (minum_phase_) {
        float pad[kFFTSize]{};
        std::copy(kernel.begin(), kernel.end(), pad);
        constexpr size_t num_bins = complex_fft_.NumBins(kFFTSize);
        float gains[num_bins]{};
        complex_fft_.FFTGainPhase(pad, gains);

        float log_gains[num_bins]{};
        for (size_t i = 0; i < num_bins; ++i) {
            log_gains[i] = std::log(gains[i] + 1e-18f);
        }

        float phases[num_bins]{};
        complex_fft_.IFFT<float>(pad, log_gains, phases);
        pad[0] = 0;
        pad[num_bins / 2] = 0;
        for (size_t i = num_bins / 2 + 1; i < num_bins; ++i) {
            pad[i] = -pad[i];
        }

        complex_fft_.FFT(pad, log_gains, phases);
        complex_fft_.IFFTGainPhase(pad, gains, phases);

        for (size_t i = 0; i < kernel.size(); ++i) {
            kernel[i] = pad[i];
        }
    }
    
    if (!feedback_enable_) {
        float energy = 0;
        for (auto x : kernel) {
            energy += x * x;
        }
        float g = 1.0f / std::sqrt(energy);
        for (auto& x : kernel) {
            x *= g;
        }
    }
}

void SteepFlangerAudioProcessor::UpdateFeedback() {
    if (!feedback_enable_) {
        feedback_mul_ = 0;
    }
    else {
        float abs_db = std::abs(feedback_value_);
        float abs_gain = qwqdsp::convert::Db2Gain(-abs_db);
        abs_gain = std::min(abs_gain, 0.95f);
        if (feedback_value_ > 0) {
            feedback_mul_ = -abs_gain;
        }
        else {
            feedback_mul_ = abs_gain;
        }
    }
}

void SteepFlangerAudioProcessor::Panic() {
    juce::ScopedLock _{getCallbackLock()};
    left_fb_ = 0;
    right_fb_ = 0;
    delay_left_.Reset();
    delay_right_.Reset();
    left_hilbert_.Reset();
    right_hilbert_.Reset();
}