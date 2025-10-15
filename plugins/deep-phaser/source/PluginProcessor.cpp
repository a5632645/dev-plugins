#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "qwqdsp/filter/window_fir.hpp"
#include "qwqdsp/window/kaiser.hpp"
#include "qwqdsp/polymath.hpp"
#include "qwqdsp/convert.hpp"
#include "x86/sse2.h"

//==============================================================================
DeepPhaserAudioProcessor::DeepPhaserAudioProcessor()
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

    // allpass
    {
        auto p = std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{"state", 1},
            "state",
            1.0f, static_cast<int>(AllpassBuffer::kMaxIndex / 4),
            12.0f
        );
        param_state_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"blend", 1},
            "blend",
            juce::NormalisableRange<float>{-0.95f, 0.95f, 0.001f},
            -0.6f
        );
        param_allpass_blend_ = p.get();
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
            is_using_custom_ = false;
            should_update_fir_ = true;
        });
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"coeff_len", 1},
            "coeff_len",
            juce::NormalisableRange<float>{4.0f, static_cast<float>(kMaxCoeffLen), 1.0f},
            6.0f
        );
        param_fir_coeff_len_ = p.get();
        param_listener_.Add(p, [this](float) {
            should_update_fir_ = true;
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
            is_using_custom_ = false;
            should_update_fir_ = true;
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
            should_update_fir_ = true;
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
            is_using_custom_ = false;
            should_update_fir_ = true;
        });
        layout.add(std::move(p));
    }

    // feedback
    {
        auto p = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"fb_value", 1},
            "fb_value",
            -0.95f, 0.95f, 0.0f
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
            juce::ParameterID{"barber_speed", 1},
            "barber_speed",
            juce::NormalisableRange<float>{-10.0f, 10.0f, 0.01f},
            0.5f
        );
        param_barber_speed_ = p.get();
        layout.add(std::move(p));
    }
    {
        auto p = std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{"barber_enable", 1},
            "barber_enable",
            true
        );
        param_barber_enable_ = p.get();
        layout.add(std::move(p));
    }

    value_tree_ = std::make_unique<juce::AudioProcessorValueTreeState>(*this, nullptr, "PARAMETERS", std::move(layout));

    complex_fft_.Init(kFFTSize);
}

DeepPhaserAudioProcessor::~DeepPhaserAudioProcessor()
{
    value_tree_ = nullptr;
}

//==============================================================================
const juce::String DeepPhaserAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool DeepPhaserAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool DeepPhaserAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool DeepPhaserAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double DeepPhaserAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int DeepPhaserAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int DeepPhaserAudioProcessor::getCurrentProgram()
{
    return 0;
}

void DeepPhaserAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String DeepPhaserAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void DeepPhaserAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void DeepPhaserAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    std::ignore = samplesPerBlock;
    
    barber_phase_smoother_.SetSmoothTime(20.0f, static_cast<float>(sampleRate));
    damp_.Reset();
    delay_left_.Reset();
    delay_right_.Reset();
    barber_oscillator_.Reset();
    barber_osc_keep_amp_counter_ = 0;
    // VIC正交振荡器衰减非常慢，设定为5分钟保持一次
    barber_osc_keep_amp_need_ = static_cast<size_t>(sampleRate * 60 * 5);

    should_update_fir_ = true;
}

void DeepPhaserAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool DeepPhaserAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void DeepPhaserAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    std::ignore = midiMessages;

    juce::ScopedNoDenormals noDenormals;

    size_t const len = static_cast<size_t>(buffer.getNumSamples());
    auto* left_ptr = buffer.getWritePointer(0);
    auto* right_ptr = buffer.getWritePointer(1);

    float const fs = static_cast<float>(getSampleRate());

    size_t cando = len;
    while (cando != 0) {
        size_t num_process = std::min<size_t>(512, cando);
        cando -= num_process;

        if (should_update_fir_.exchange(false)) {
            UpdateCoeff();
        }

        float const feedback_mul = param_feedback_->get();
        float const damp_pitch = param_damp_pitch_->get();
        float const damp_freq = qwqdsp::convert::Pitch2Freq(damp_pitch);
        float const damp_w = qwqdsp::convert::Freq2W(damp_freq, fs);
        damp_lowpass_coeff_ = damp_.ComputeCoeff(damp_w);

        barber_phase_smoother_.SetTarget(param_barber_phase_->get());
        barber_oscillator_.SetFreq(param_barber_speed_->get(), fs);

        float const max_base_state = AllpassBuffer::kMaxIndex / static_cast<float>(coeff_len_);
        float current_num_state = static_cast<float>(param_state_->get());
        size_t num_calc_apf = static_cast<size_t>(param_state_->get()) * coeff_len_div_4_;
        num_calc_apf = std::min(num_calc_apf, AllpassBuffer::kNumApf);
        current_num_state = std::min(max_base_state, current_num_state);
        int const num_state = static_cast<int>(current_num_state);

        float curr_damp_coeff = last_damp_lowpass_coeff_;
        float delta_damp_coeff = (damp_lowpass_coeff_ - curr_damp_coeff) / (static_cast<float>(num_process));

        float inv_samples = 1.0f / static_cast<float>(num_process);
        std::array<SimdType, kSIMDMaxCoeffLen / 4> delta_coeffs;
        for (size_t i = 0; i < coeff_len_div_4_; ++i) {
            auto last = simde_mm_load_ps(last_coeffs_[i].x);
            auto target = simde_mm_load_ps(coeffs_.data() + 4 * i);
            auto delta = simde_mm_mul_ps(simde_mm_sub_ps(target, last), simde_mm_set_ps1(inv_samples));
            simde_mm_store_ps(delta_coeffs[i].x, delta);
        }

        float target_allpass_coeff = param_allpass_blend_->get();
        float delta_allpass_coeff = (target_allpass_coeff - last_allpass_coeff_) * inv_samples;

        if (!param_barber_enable_->get()) {
            for (size_t j = 0; j < num_process; ++j) {
                curr_damp_coeff += delta_damp_coeff;
                last_allpass_coeff_ += delta_allpass_coeff;

                for (size_t i = 0; i < coeff_len_div_4_; ++i) {
                    last_coeffs_[i] += delta_coeffs[i];
                }
    
                float left_sum = 0;
                SimdIntType current_delay;
                current_delay.x[0] = 0;
                current_delay.x[1] = num_state;
                current_delay.x[2] = num_state * 2;
                current_delay.x[3] = num_state * 3;
                SimdIntType delay_inc = SimdIntType::FromSingle(num_state * 4);
                delay_left_.Push(*left_ptr + left_fb_ * feedback_mul, last_allpass_coeff_, num_calc_apf);
                for (size_t i = 0; i < coeff_len_div_4_; ++i) {
                    SimdType taps_out = delay_left_.GetAfterPush(current_delay);
                    current_delay += delay_inc;

                    taps_out *= last_coeffs_[i];
                    left_sum += taps_out.x[0];
                    left_sum += taps_out.x[1];
                    left_sum += taps_out.x[2];
                    left_sum += taps_out.x[3];
                }

                float right_sum = 0;
                current_delay.x[0] = 0;
                current_delay.x[1] = num_state;
                current_delay.x[2] = num_state * 2;
                current_delay.x[3] = num_state * 3;
                delay_inc = SimdIntType::FromSingle(num_state * 4);
                delay_right_.Push(*right_ptr + right_fb_ * feedback_mul, last_allpass_coeff_, num_calc_apf);
                for (size_t i = 0; i < coeff_len_div_4_; ++i) {
                    SimdType taps_out = delay_right_.GetAfterPush(current_delay);
                    current_delay += delay_inc;
                    taps_out *= last_coeffs_[i];
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
                last_allpass_coeff_ += delta_allpass_coeff;

                for (size_t i = 0; i < coeff_len_div_4_; ++i) {
                    last_coeffs_[i] += delta_coeffs[i];
                }

                delay_left_.Push(*left_ptr + left_fb_ * feedback_mul, last_allpass_coeff_, num_calc_apf);
                delay_right_.Push(*right_ptr + right_fb_ * feedback_mul, last_allpass_coeff_, num_calc_apf);

                SimdIntType left_current_delay;
                SimdIntType right_current_delay;
                left_current_delay.x[0] = 0;
                left_current_delay.x[1] = num_state;
                left_current_delay.x[2] = num_state * 2;
                left_current_delay.x[3] = num_state * 3;
                right_current_delay.x[0] = 0;
                right_current_delay.x[1] = num_state;
                right_current_delay.x[2] = num_state * 2;
                right_current_delay.x[3] = num_state * 3;
                SimdIntType left_delay_inc = SimdIntType::FromSingle(num_state * 4);
                SimdIntType right_delay_inc = SimdIntType::FromSingle(num_state * 4);

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
                for (size_t i = 0; i < coeff_len_div_4_; ++i) {
                    SimdType left_taps_out = delay_left_.GetAfterPush(left_current_delay);
                    SimdType right_taps_out = delay_right_.GetAfterPush(right_current_delay);
                    left_current_delay += left_delay_inc;
                    right_current_delay += right_delay_inc;

                    left_taps_out *= last_coeffs_[i];
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

                    right_taps_out *= last_coeffs_[i];
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
bool DeepPhaserAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* DeepPhaserAudioProcessor::createEditor()
{
    return new DeepPhaserAudioProcessorEditor (*this);
    // return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void DeepPhaserAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
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

void DeepPhaserAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    suspendProcessing(true);
    auto xml = *getXmlFromBinary(data, sizeInBytes);
    auto state = juce::ValueTree::fromXml(xml);
    if (state.isValid()) {
        value_tree_->replaceState(state);
        auto coeffs = xml.getChildByName("CUSTOM_COEFFS");
        if (coeffs) {
            is_using_custom_ = coeffs->getBoolAttribute("USING", false);
            auto data_sections = coeffs->getChildByName("DATA");
            if (data_sections) {
                auto it = data_sections->getChildIterator();
                for (size_t i = 0; auto item : it) {
                    custom_coeffs_[i] = static_cast<float>(item->getDoubleAttribute("TIME"));
                    custom_spectral_gains[i] = static_cast<float>(item->getDoubleAttribute("SPECTRAL"));
                    ++i;
                    // protect loading old version 65 length coeffs
                    if (i == kMaxCoeffLen) break;
                }
                should_update_fir_ = true;
            }
        }
    }
    suspendProcessing(false);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DeepPhaserAudioProcessor();
}




void DeepPhaserAudioProcessor::UpdateCoeff() {
    size_t coeff_len = static_cast<size_t>(param_fir_coeff_len_->get());
    coeff_len_ = coeff_len;

    if (!is_using_custom_) {
        std::span<float> kernel{coeffs_.data(), coeff_len};
        float const cutoff_w = param_fir_cutoff_->get();
        if (param_fir_highpass_->get()) {
            qwqdsp::filter::WindowFIR::Highpass(kernel, std::numbers::pi_v<float> - cutoff_w);
        }
        else {
            qwqdsp::filter::WindowFIR::Lowpass(kernel, cutoff_w);
        }
        float const beta = qwqdsp::window::Kaiser::Beta(param_fir_side_lobe_->get());
        qwqdsp::window::Kaiser::ApplyWindow(kernel, beta, false);
    }
    else {
        std::copy_n(custom_coeffs_.begin(), coeff_len, coeffs_.begin());
    }

    coeff_len_div_4_ = (coeff_len + 3) / 4;
    size_t const idxend = coeff_len_div_4_ * 4;
    for (size_t i = coeff_len; i < idxend; ++i) {
        coeffs_[i] = 0;
    }

    float pad[kFFTSize]{};
    std::span<float> kernel{coeffs_.data(), coeff_len};
    constexpr size_t num_bins = complex_fft_.NumBins(kFFTSize);
    std::array<float, num_bins> gains{};
    std::copy(kernel.begin(), kernel.end(), pad);
    complex_fft_.FFTGainPhase(pad, gains);

    if (param_fir_min_phase_->get()) {
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

    float const max_spectral_gain = *std::max_element(gains.begin(), gains.end());
    float const gain = 1.0f / (max_spectral_gain + 1e-10f);
    for (auto& x : kernel) {
        x *= gain;
    }

    have_new_coeff_ = true;
}

void DeepPhaserAudioProcessor::Panic() {
    juce::ScopedLock _{getCallbackLock()};
    left_fb_ = 0;
    right_fb_ = 0;
    delay_left_.Reset();
    delay_right_.Reset();
    hilbert_complex_.Reset();
}
