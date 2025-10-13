#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "qwqdsp/filter/window_fir.hpp"
#include "qwqdsp/window/kaiser.hpp"
#include "qwqdsp/polymath.hpp"
#include "qwqdsp/convert.hpp"
#include "qwqdsp/filter/rbj.hpp"
#include "x86/sse2.h"

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
            is_using_custom_ = false;
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
            is_using_custom_ = false;
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
            is_using_custom_ = false;
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
            0.0f, 140.0f,
            90.0f
        );
        param_listener_.Add(p, [this](float damp_pitch) {
            juce::ScopedLock _{getCallbackLock()};
            float damp_freq = qwqdsp::convert::Pitch2Freq(damp_pitch);
            if (damp_freq > 20000.0f) {
                // no filting
                damp_lowpass_coeff_ = 1.0f;
            }
            else {
                float const w = qwqdsp::convert::Freq2W(damp_freq, static_cast<float>(getSampleRate()));
                damp_lowpass_coeff_ = damp_.ComputeCoeff(w);
            }
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
            barber_oscillator_.SetFreq(barber_speed, getSampleRate());
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
    float const samples_need = sampleRate * 30.0f / 1000.0f;
    delay_left_.Init(samples_need * kMaxCoeffLen + 256 + 4);
    delay_right_.Init(samples_need * kMaxCoeffLen + 256 + 4);
    barber_phase_smoother_.SetSmoothTime(20.0f, sampleRate);
    damp_.Reset();
    barber_oscillator_.Reset();
    barber_osc_keep_amp_counter_ = 0;
    // VIC正交振荡器衰减非常慢，设定为5分钟保持一次
    barber_osc_keep_amp_need_ = sampleRate * 60 * 5;
    param_listener_.CallAll();

#if HAVE_MEASUREMENT
    measurer.reset(sampleRate, samplesPerBlock);
#endif
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

// ---------------------------------------- 简单的SIMD 复数，为了旋转 ----------------------------------------
struct Vec4Complex {
    SimdType re;
    SimdType im;

    static constexpr Vec4Complex FromSingle(float re, float im) noexcept {
        Vec4Complex r;
        r.re = SimdType::FromSingle(re);
        r.im = SimdType::FromSingle(im);
        return r;
    }

    static Vec4Complex FromPolar(SimdType w) noexcept {
        Vec4Complex r;
        r.re.x[0] = std::cos(w.x[0]);
        r.re.x[1] = std::cos(w.x[1]);
        r.re.x[2] = std::cos(w.x[2]);
        r.re.x[3] = std::cos(w.x[3]);
        r.im.x[0] = std::sin(w.x[0]);
        r.im.x[1] = std::sin(w.x[1]);
        r.im.x[2] = std::sin(w.x[2]);
        r.im.x[3] = std::sin(w.x[3]);
        return r;
    }

    constexpr Vec4Complex& operator*=(const Vec4Complex& a) {
        SimdType new_re = re * a.re - im * a.im;
        SimdType new_im = re * a.im + im * a.re;
        re = new_re;
        im = new_im;
        return *this;
    }
};

void SteepFlangerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
#if HAVE_MEASUREMENT
    juce::AudioProcessLoadMeasurer::ScopedTimer _{measurer};
#endif

    std::ignore = midiMessages;

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    size_t const len = static_cast<size_t>(buffer.getNumSamples());
    auto* left_ptr = buffer.getWritePointer(0);
    auto* right_ptr = buffer.getWritePointer(1);

    size_t cando = len;
    while (cando != 0) {
        size_t num_process = std::min<size_t>(256, cando);
        cando -= num_process;

        // update delay times
        phase_ += phase_inc_ * static_cast<float>(num_process);
        float right_phase = phase_ + phase_shift_;
        {
            float t;
            phase_ = std::modf(phase_, &t);
            right_phase = std::modf(right_phase, &t);
        }
        float left_phase = phase_;

        SimdType lfo_modu;
        lfo_modu.x[0] = qwqdsp::polymath::SinPi(left_phase * std::numbers::pi_v<float>);
        lfo_modu.x[1] = qwqdsp::polymath::SinPi(right_phase * std::numbers::pi_v<float>);

        SimdType target_delay_samples = SimdType::FromSingle(delay_samples_) + lfo_modu * SimdType::FromSingle(depth_samples_);
        target_delay_samples = SimdType::Max(target_delay_samples, SimdType::FromSingle(0.0f));
        float const delay_time_smooth_factor = 1.0f - std::exp(-1.0f / (static_cast<float>(getSampleRate()) / static_cast<float>(num_process) * 20.0f / 1000.0f));
        last_exp_delay_samples_ += SimdType::FromSingle(delay_time_smooth_factor) * (target_delay_samples - last_exp_delay_samples_);
        SimdType curr_num_notch = last_delay_samples_;
        SimdType delta_num_notch = (last_exp_delay_samples_ - curr_num_notch) / SimdType::FromSingle(static_cast<float>(num_process));

        float curr_damp_coeff = last_damp_lowpass_coeff_;
        float delta_damp_coeff = (damp_lowpass_coeff_ - curr_damp_coeff) / (static_cast<float>(num_process));

        delay_left_.WrapBuffer();
        delay_right_.WrapBuffer();

        // fir polyphase filtering
        if (!barber_enable_) {
            for (size_t j = 0; j < num_process; ++j) {
                curr_num_notch += delta_num_notch;
                curr_damp_coeff += delta_damp_coeff;
    
                float left_sum = 0;
                float const left_num_notch = curr_num_notch.x[0];
                // for (size_t i = 0; i < coeff_len_; ++i) {
                //     sum += coeffs_[i] * delay_left_.GetAfterPush(i * num_notch);
                // }
                SimdType current_delay;
                current_delay.x[0] = 0;
                current_delay.x[1] = left_num_notch;
                current_delay.x[2] = left_num_notch * 2;
                current_delay.x[3] = left_num_notch * 3;
                SimdType delay_inc = SimdType::FromSingle(left_num_notch * 4);
                auto coeff_it = coeffs_.data();
                delay_left_.Push(*left_ptr + left_fb_ * feedback_mul_);
                for (size_t i = 0; i < coeff_len_div_4_; ++i) {
                    SimdType taps_out = delay_left_.GetAfterPush(current_delay);
                    current_delay += delay_inc;
                    SimdType vec_coeffs;
                    vec_coeffs.x[0] = *coeff_it++;
                    vec_coeffs.x[1] = *coeff_it++;
                    vec_coeffs.x[2] = *coeff_it++;
                    vec_coeffs.x[3] = *coeff_it++;
                    taps_out *= vec_coeffs;
                    left_sum += taps_out.x[0];
                    left_sum += taps_out.x[1];
                    left_sum += taps_out.x[2];
                    left_sum += taps_out.x[3];
                }

                float right_sum = 0;
                float const right_num_notch = curr_num_notch.x[1];
                // for (size_t i = 0; i < coeff_len_; ++i) {
                //     sum += coeffs_[i] * delay_right_.GetAfterPush(i * num_notch);
                // }
                current_delay.x[0] = 0;
                current_delay.x[1] = right_num_notch;
                current_delay.x[2] = right_num_notch * 2;
                current_delay.x[3] = right_num_notch * 3;
                delay_inc = SimdType::FromSingle(right_num_notch * 4);
                coeff_it = coeffs_.data();
                delay_right_.Push(*right_ptr + right_fb_ * feedback_mul_);
                for (size_t i = 0; i < coeff_len_div_4_; ++i) {
                    SimdType taps_out = delay_right_.GetAfterPush(current_delay);
                    current_delay += delay_inc;
                    SimdType vec_coeffs;
                    vec_coeffs.x[0] = *coeff_it++;
                    vec_coeffs.x[1] = *coeff_it++;
                    vec_coeffs.x[2] = *coeff_it++;
                    vec_coeffs.x[3] = *coeff_it++;
                    taps_out *= vec_coeffs;
                    right_sum += taps_out.x[0];
                    right_sum += taps_out.x[1];
                    right_sum += taps_out.x[2];
                    right_sum += taps_out.x[3];
                }

                SimdType damp_x;
                damp_x.x[0] = left_sum;
                damp_x.x[1] = right_sum;
                *left_ptr = left_sum;
                *right_ptr = right_sum;
                ++left_ptr;
                ++right_ptr;
                damp_x = damp_.TickLowpass(damp_x, SimdType::FromSingle(curr_damp_coeff));
                left_fb_ = damp_x.x[0];
                right_fb_ = damp_x.x[1];
            }
        }
        else {
            for (size_t j = 0; j < num_process; ++j) {
                curr_damp_coeff += delta_damp_coeff;
                curr_num_notch += delta_num_notch;
                delay_left_.Push(*left_ptr + left_fb_ * feedback_mul_);
                delay_right_.Push(*right_ptr + right_fb_ * feedback_mul_);

                float const left_num_notch = curr_num_notch.x[0];
                float const right_num_notch = curr_num_notch.x[1];
                SimdType left_current_delay;
                SimdType right_current_delay;
                left_current_delay.x[0] = 0;
                left_current_delay.x[1] = left_num_notch;
                left_current_delay.x[2] = left_num_notch * 2;
                left_current_delay.x[3] = left_num_notch * 3;
                right_current_delay.x[0] = 0;
                right_current_delay.x[1] = right_num_notch;
                right_current_delay.x[2] = right_num_notch * 2;
                right_current_delay.x[3] = right_num_notch * 3;
                SimdType left_delay_inc = SimdType::FromSingle(left_num_notch * 4);
                SimdType right_delay_inc = SimdType::FromSingle(right_num_notch * 4);

                // auto const left_rotation_mul = std::polar(1.0f, barber * std::numbers::pi_v<float> * 2);
                // auto const right_rotation_mul = std::polar(1.0f, barber * std::numbers::pi_v<float> * 2);
                // std::complex<float> left_rotation_coeff = 1;
                // std::complex<float> right_rotation_coeff = 1;
                // std::complex<float> left_sum = 0;
                // std::complex<float> right_sum = 0;

                // barber_phase_ += barber_phase_inc_;
                // barber_phase_ -= std::floor(barber_phase_);
                // float barber = barber_phase_ + barber_phase_smoother_.Tick();
                // barber -= std::floor(barber);

                // for (size_t i = 0; i < coeff_len_; ++i) {
                //     left_sum += left_rotation_coeff * coeffs_[i] * delay_left_.GetAfterPush(i * left_num_notch);
                //     right_sum += right_rotation_coeff * coeffs_[i] * delay_right_.GetAfterPush(i * right_num_notch);
                //     left_rotation_coeff *= left_rotation_mul;
                //     right_rotation_coeff *= right_rotation_mul;
                // }
                auto const addition_rotation = std::polar(1.0f, barber_phase_smoother_.Tick() * std::numbers::pi_v<float> * 2);
                barber_oscillator_.Tick();
                auto const rotation_once = barber_oscillator_.GetCpx() * addition_rotation;
                auto const rotation_2 = rotation_once * rotation_once;
                auto const rotation_3 = rotation_once * rotation_2;
                auto const rotation_4 = rotation_2 * rotation_2;
                Vec4Complex left_rotation_coeff;
                left_rotation_coeff.re.x[0] = 1;
                left_rotation_coeff.re.x[1] = rotation_once.real();
                left_rotation_coeff.re.x[2] = rotation_2.real();
                left_rotation_coeff.re.x[3] = rotation_3.real();
                left_rotation_coeff.im.x[0] = 0;
                left_rotation_coeff.im.x[1] = rotation_once.imag();
                left_rotation_coeff.im.x[2] = rotation_2.imag();
                left_rotation_coeff.im.x[3] = rotation_3.imag();
                Vec4Complex right_rotation_coeff = left_rotation_coeff;
                Vec4Complex left_rotation_mul;
                left_rotation_mul.re.x[0] = rotation_4.real();
                left_rotation_mul.re.x[1] = rotation_4.real();
                left_rotation_mul.re.x[2] = rotation_4.real();
                left_rotation_mul.re.x[3] = rotation_4.real();
                left_rotation_mul.im.x[0] = rotation_4.imag();
                left_rotation_mul.im.x[1] = rotation_4.imag();
                left_rotation_mul.im.x[2] = rotation_4.imag();
                left_rotation_mul.im.x[3] = rotation_4.imag();
                Vec4Complex right_rotation_mul = left_rotation_mul;

                float left_re_sum = 0;
                float left_im_sum = 0;
                float right_re_sum = 0;
                float right_im_sum = 0;
                auto coeff_it = coeffs_.data();
                for (size_t i = 0; i < coeff_len_div_4_; ++i) {
                    SimdType left_taps_out = delay_left_.GetAfterPush(left_current_delay);
                    SimdType right_taps_out = delay_right_.GetAfterPush(right_current_delay);
                    left_current_delay += left_delay_inc;
                    right_current_delay += right_delay_inc;

                    SimdType vec_coeffs;
                    vec_coeffs.x[0] = *coeff_it++;
                    vec_coeffs.x[1] = *coeff_it++;
                    vec_coeffs.x[2] = *coeff_it++;
                    vec_coeffs.x[3] = *coeff_it++;

                    left_taps_out *= vec_coeffs;
                    SimdType temp = left_taps_out * left_rotation_coeff.re;
                    left_re_sum += temp.x[0];
                    left_re_sum += temp.x[1];
                    left_re_sum += temp.x[2];
                    left_re_sum += temp.x[3];
                    temp = left_taps_out * left_rotation_coeff.im;
                    left_im_sum += temp.x[0];
                    left_im_sum += temp.x[1];
                    left_im_sum += temp.x[2];
                    left_im_sum += temp.x[3];

                    right_taps_out *= vec_coeffs;
                    temp = right_taps_out * right_rotation_coeff.re;
                    right_re_sum += temp.x[0];
                    right_re_sum += temp.x[1];
                    right_re_sum += temp.x[2];
                    right_re_sum += temp.x[3];
                    temp = right_taps_out * right_rotation_coeff.im;
                    right_im_sum += temp.x[0];
                    right_im_sum += temp.x[1];
                    right_im_sum += temp.x[2];
                    right_im_sum += temp.x[3];

                    left_rotation_coeff *= left_rotation_mul;
                    right_rotation_coeff *= right_rotation_mul;
                }
                
                SimdType remove_positive_spectrum = hilbert_complex_.Tick(SimdType{
                    left_re_sum, left_im_sum, right_re_sum, right_im_sum
                });
                // this will mirror the positive spectrum to negative domain, forming a real value signal
                SimdType damp_x = remove_positive_spectrum.Shuffle<0, 2, 1, 3>();
                *left_ptr = damp_x.x[0];
                *right_ptr = damp_x.x[1];
                ++left_ptr;
                ++right_ptr;
                damp_x = damp_.TickLowpass(damp_x, SimdType::FromSingle(curr_damp_coeff));
                left_fb_ = damp_x.x[0];
                right_fb_ = damp_x.x[1];
            }
        }
        last_delay_samples_ = last_exp_delay_samples_;
        last_damp_lowpass_coeff_ = damp_lowpass_coeff_;
    }
    barber_osc_keep_amp_counter_ += len;
    [[unlikely]]
    if (barber_osc_keep_amp_counter_ > barber_osc_keep_amp_need_) {
        barber_osc_keep_amp_counter_ = 0;
        barber_oscillator_.KeepAmp();
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
    suspendProcessing(true);
    if (auto state = value_tree_->copyState().createXml(); state != nullptr) {
        auto custom_coeffs = state->createNewChildElement("CUSTOM_COEFFS");
        custom_coeffs->setAttribute("USING", is_using_custom_);
        auto data = custom_coeffs->createNewChildElement("DATA");
        for (size_t i = 0; i < kMaxCoeffLen; ++i) {
            auto time = data->createNewChildElement("ITEM");
            time->setAttribute("TIME", custom_coeffs_[i]);
            time->setAttribute("SPECTRAL", custom_spectral_gains[i]);
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
            is_using_custom_ = coeffs->getBoolAttribute("USING", false);
            auto data = coeffs->getChildByName("DATA");
            if (data) {
                auto it = data->getChildIterator();
                for (size_t i = 0; auto item : it) {
                    custom_coeffs_[i] = item->getDoubleAttribute("TIME");
                    custom_spectral_gains[i] = item->getDoubleAttribute("SPECTRAL");
                    ++i;
                }

                if (is_using_custom_) {
                    UpdateCoeff();
                }
            }
        }
        editor_update_.UpdateGui();
    }
    suspendProcessing(false);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SteepFlangerAudioProcessor();
}




void SteepFlangerAudioProcessor::UpdateCoeff() {
    if (!is_using_custom_) {
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
    }
    else {
        std::copy_n(custom_coeffs_.begin(), coeff_len_, coeffs_.begin());
    }

    coeff_len_div_4_ = (coeff_len_ + 3) / 4;
    size_t const idxend = coeff_len_div_4_ * 4;
    for (size_t i = coeff_len_; i < idxend; ++i) {
        coeffs_[i] = 0;
    }

    PostCoeffsProcessing();

    editor_update_.UpdateGui();
}

void SteepFlangerAudioProcessor::PostCoeffsProcessing() {
    std::span<float> kernel{coeffs_.data(), coeff_len_};

    float pad[kFFTSize]{};
    constexpr size_t num_bins = complex_fft_.NumBins(kFFTSize);
    std::array<float, num_bins> gains{};
    if (minum_phase_ || feedback_enable_) {
        std::copy(kernel.begin(), kernel.end(), pad);
        complex_fft_.FFTGainPhase(pad, gains);
    }

    if (minum_phase_) {
        float log_gains[num_bins]{};
        for (size_t i = 0; i < num_bins; ++i) {
            log_gains[i] = std::log(gains[i] + 1e-18f);
        }

        float phases[num_bins]{};
        complex_fft_.IFFT(pad, log_gains, phases);
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
        float g = 1.0f / std::sqrt(energy + 1e-18f);
        for (auto& x : kernel) {
            x *= g;
        }
    }
    else {
        float const max_spectral_gain = *std::max_element(gains.begin(), gains.end());
        float const gain = 1.0f / (max_spectral_gain + 1e-10f);
        for (auto& x : kernel) {
            x *= gain;
        }
    }
}

void SteepFlangerAudioProcessor::UpdateFeedback() {
    if (!feedback_enable_) {
        feedback_mul_ = 0;
    }
    else {
        float abs_db = std::abs(feedback_value_);
        if (minum_phase_) {
            abs_db = std::max(abs_db, 4.1f);
        }
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
    hilbert_complex_.Reset();
}



void EditorUpdate::handleAsyncUpdate() {
    auto* ptr = editor_.load();
    if (ptr != nullptr) {
        auto* editor = static_cast<SteepFlangerAudioProcessorEditor*>(ptr);
        editor->UpdateGui();
    }
}