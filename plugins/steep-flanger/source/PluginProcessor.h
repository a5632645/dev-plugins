#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

#include "qwqdsp/fx/delay_line.hpp"
#include "qwqdsp/misc/smoother.hpp"
#include "qwqdsp/spectral/complex_fft.hpp"
#include "qwqdsp/filter/biquad.hpp"
#include "qwqdsp/filter/iir_cpx_hilbert.hpp"

#include "shared.hpp"

struct JuceParamListener{
    struct FloatStore : public juce::AudioProcessorParameter::Listener {
        std::function<void(float)> func;
        juce::AudioParameterFloat* ptr;

        FloatStore(std::function<void(float)> func, juce::AudioParameterFloat* ptr) : func(func), ptr(ptr) {
            ptr->addListener(this);
        }
        void parameterValueChanged (int parameterIndex, float newValue) override {
            func(ptr->get());
            (void)parameterIndex;
            (void)newValue;
        }
        void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override {
            (void)parameterIndex;
            (void)gestureIsStarting;
        }
    };
    struct BoolStore : public juce::AudioProcessorParameter::Listener {
        std::function<void(bool)> func;
        juce::AudioParameterBool* ptr;

        BoolStore(std::function<void(bool)> func, juce::AudioParameterBool* ptr) : func(func), ptr(ptr) {
            ptr->addListener(this);
        }
        void parameterValueChanged (int parameterIndex, float newValue) override {
            func(ptr->get());
            (void)parameterIndex;
            (void)newValue;
        }
        void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override {
            (void)parameterIndex;
            (void)gestureIsStarting;
        }
    };
    struct IntStore : public juce::AudioProcessorParameter::Listener {
        std::function<void(int)> func;
        juce::AudioParameterInt* ptr;

        IntStore(std::function<void(int)> func, juce::AudioParameterInt* ptr) : func(func), ptr(ptr) {
            ptr->addListener(this);
        }
        void parameterValueChanged (int parameterIndex, float newValue) override {
            func(ptr->get());
            (void)parameterIndex;
            (void)newValue;
        }
        void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override {
            (void)parameterIndex;
            (void)gestureIsStarting;
        }
    };
    struct ChoiceStore : public juce::AudioProcessorParameter::Listener {
        std::function<void(int)> func;
        juce::AudioParameterChoice* ptr;

        ChoiceStore(std::function<void(int)> func, juce::AudioParameterChoice* ptr) : func(func), ptr(ptr) {
            ptr->addListener(this);
        }
        void parameterValueChanged (int parameterIndex, float newValue) override {
            func(ptr->getIndex());
            (void)parameterIndex;
            (void)newValue;
        }
        void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override {
            (void)parameterIndex;
            (void)gestureIsStarting;
        }
    };

    std::vector<std::unique_ptr<juce::AudioProcessorParameter::Listener>> listeners;
    void CallAll() {
        for (auto& l : listeners) {
            l->parameterValueChanged(0, 0);
        }
    }
    void Add(const std::unique_ptr<juce::AudioParameterFloat>& p, std::function<void(float)> func) {
        listeners.emplace_back(std::make_unique<FloatStore>(func, p.get()));
    }
    void Add(const std::unique_ptr<juce::AudioParameterBool>& p, std::function<void(bool)> func) {
        listeners.emplace_back(std::make_unique<BoolStore>(func, p.get()));
    }
    void Add(const std::unique_ptr<juce::AudioParameterInt>& p, std::function<void(int)> func) {
        listeners.emplace_back(std::make_unique<IntStore>(func, p.get()));
    }
    void Add(const std::unique_ptr<juce::AudioParameterChoice>& p, std::function<void(int)> func) {
        listeners.emplace_back(std::make_unique<ChoiceStore>(func, p.get()));
    }
};

class EditorUpdate : public juce::AsyncUpdater {
public:
    void UpdateGui() {
        if (juce::MessageManager::getInstance()->isThisTheMessageThread()) {
            cancelPendingUpdate();
            handleAsyncUpdate();
        }
        else {
            triggerAsyncUpdate();
        }
    }

    void handleAsyncUpdate() override;

    void OnEditorCreate(juce::AudioProcessorEditor* editor) {
        editor_ = editor;
        cancelPendingUpdate();
        UpdateGui();
    }

    void OnEditorDestory() {
        editor_ = nullptr;
        cancelPendingUpdate();
    }
private:
    std::atomic<juce::AudioProcessorEditor*> editor_;
};

// ---------------------------------------- speed up ----------------------------------------

struct alignas(16) Vec4i32 {
    int x[4];

    static constexpr Vec4i32 FromSingle(int v) {
        Vec4i32 r;
        r.x[0] = v;
        r.x[1] = v;
        r.x[2] = v;
        r.x[3] = v;
        return r;
    }
};

static constexpr Vec4i32 operator+(const Vec4i32& a, const Vec4i32& b) {
    Vec4i32 r;
    r.x[0] = a.x[0] + b.x[0];
    r.x[1] = a.x[1] + b.x[1];
    r.x[2] = a.x[2] + b.x[2];
    r.x[3] = a.x[3] + b.x[3];
    return r;
}
static constexpr Vec4i32 operator-(const Vec4i32& a, const Vec4i32& b) {
    Vec4i32 r;
    r.x[0] = a.x[0] - b.x[0];
    r.x[1] = a.x[1] - b.x[1];
    r.x[2] = a.x[2] - b.x[2];
    r.x[3] = a.x[3] - b.x[3];
    return r;
}
static constexpr Vec4i32 operator*(const Vec4i32& a, const Vec4i32& b) {
    Vec4i32 r;
    r.x[0] = a.x[0] * b.x[0];
    r.x[1] = a.x[1] * b.x[1];
    r.x[2] = a.x[2] * b.x[2];
    r.x[3] = a.x[3] * b.x[3];
    return r;
}
static constexpr Vec4i32 operator/(const Vec4i32& a, const Vec4i32& b) {
    Vec4i32 r;
    r.x[0] = a.x[0] / b.x[0];
    r.x[1] = a.x[1] / b.x[1];
    r.x[2] = a.x[2] / b.x[2];
    r.x[3] = a.x[3] / b.x[3];
    return r;
}
static constexpr Vec4i32 operator&(const Vec4i32& a, const Vec4i32& b) {
    Vec4i32 r;
    r.x[0] = a.x[0] & b.x[0];
    r.x[1] = a.x[1] & b.x[1];
    r.x[2] = a.x[2] & b.x[2];
    r.x[3] = a.x[3] & b.x[3];
    return r;
}

struct alignas(16) Vec4 {
    float x[4];

    static constexpr Vec4 FromSingle(float v) {
        Vec4 r;
        r.x[0] = v;
        r.x[1] = v;
        r.x[2] = v;
        r.x[3] = v;
        return r;
    }

    constexpr Vec4& operator+=(const Vec4& v) {
        x[0] += v.x[0];
        x[1] += v.x[1];
        x[2] += v.x[2];
        x[3] += v.x[3];
        return *this;
    }

    constexpr  Vec4& operator-=(const Vec4& v) {
        x[0] -= v.x[0];
        x[1] -= v.x[1];
        x[2] -= v.x[2];
        x[3] -= v.x[3];
        return *this;
    }

    constexpr  Vec4& operator*=(const Vec4& v) {
        x[0] *= v.x[0];
        x[1] *= v.x[1];
        x[2] *= v.x[2];
        x[3] *= v.x[3];
        return *this;
    }

    constexpr Vec4& operator/=(const Vec4& v) {
        x[0] /= v.x[0];
        x[1] /= v.x[1];
        x[2] /= v.x[2];
        x[3] /= v.x[3];
        return *this;
    }

    constexpr Vec4 Frac() const noexcept {
        Vec4 r;
        r.x[0] = x[0] - static_cast<int>(x[0]);
        r.x[1] = x[1] - static_cast<int>(x[1]);
        r.x[2] = x[2] - static_cast<int>(x[2]);
        r.x[3] = x[3] - static_cast<int>(x[3]);
        return r;
    }

    constexpr Vec4i32 ToInt() const noexcept {
        Vec4i32 r;
        r.x[0] = static_cast<int>(x[0]);
        r.x[1] = static_cast<int>(x[1]);
        r.x[2] = static_cast<int>(x[2]);
        r.x[3] = static_cast<int>(x[3]);
        return r;
    }
};

static constexpr Vec4 operator+(const Vec4& a, const Vec4& b) {
    Vec4 r = a;
    r += b;
    return r;
}
static constexpr Vec4 operator-(const Vec4& a, const Vec4& b) {
    Vec4 r = a;
    r -= b;
    return r;
}
static constexpr Vec4 operator*(const Vec4& a, const Vec4& b) {
    Vec4 r = a;
    r *= b;
    return r;
}
static constexpr Vec4 operator/(const Vec4& a, const Vec4& b) {
    Vec4 r = a;
    r /= b;
    return r;
}

class Vec4DelayLine {
public:
    void Init(float max_ms, float fs) {
        float d = max_ms * fs / 1000.0f;
        size_t i = static_cast<size_t>(std::ceil(d) + 4.0f);
        Init(i);
    }

    void Init(size_t max_samples) {
        size_t a = 1;
        while (a < max_samples) {
            a *= 2;
        }
        if (buffer_.size() < a) {
            buffer_.resize(a);
        }
        mask_ = a - 1;
        Reset();
    }

    void Reset() noexcept {
        wpos_ = 0;
        std::fill(buffer_.begin(), buffer_.end(), 0.0f);
    }

    void Push(float x) noexcept {
        buffer_[wpos_] = x;
    }

    void PushFinish() noexcept {
        ++wpos_;
        wpos_ &= mask_;
    }

    Vec4 GetAfterPush(Vec4 delay_samples) noexcept {
        Vec4 frpos = Vec4::FromSingle(wpos_ + buffer_.size()) - delay_samples;
        Vec4i32 rpos = frpos.ToInt();
        Vec4i32 mask = Vec4i32::FromSingle(mask_);
        Vec4i32 irpos = rpos & mask;
        Vec4i32 inext1 = (rpos + Vec4i32::FromSingle(1)) & mask;
        Vec4i32 inext2 = (rpos + Vec4i32::FromSingle(2)) & mask;
        Vec4i32 inext3 = (rpos + Vec4i32::FromSingle(3)) & mask;
        Vec4 frac = frpos.Frac();

        Vec4 y0;
        y0.x[0] = buffer_[irpos.x[0]];
        y0.x[1] = buffer_[irpos.x[1]];
        y0.x[2] = buffer_[irpos.x[2]];
        y0.x[3] = buffer_[irpos.x[3]];
        Vec4 y1;
        y1.x[0] = buffer_[inext1.x[0]];
        y1.x[1] = buffer_[inext1.x[1]];
        y1.x[2] = buffer_[inext1.x[2]];
        y1.x[3] = buffer_[inext1.x[3]];
        Vec4 y2;
        y2.x[0] = buffer_[inext2.x[0]];
        y2.x[1] = buffer_[inext2.x[1]];
        y2.x[2] = buffer_[inext2.x[2]];
        y2.x[3] = buffer_[inext2.x[3]];
        Vec4 y3;
        y3.x[0] = buffer_[inext3.x[0]];
        y3.x[1] = buffer_[inext3.x[1]];
        y3.x[2] = buffer_[inext3.x[2]];
        y3.x[3] = buffer_[inext3.x[3]];

        Vec4 d1 = frac - Vec4::FromSingle(1.0f);
        Vec4 d2 = frac - Vec4::FromSingle(2.0f);
        Vec4 d3 = frac - Vec4::FromSingle(3.0f);

        auto c1 = d1 * d2 * d3 / Vec4::FromSingle(-6.0f);
        auto c2 = d2 * d3 * Vec4::FromSingle(0.5f);
        auto c3 = d1 * d3 * Vec4::FromSingle(-0.5f);
        auto c4 = d1 * d2 / Vec4::FromSingle(6.0f);

        return y0 * c1 + frac * (y1 * c2 + y2 * c3 + y3 * c4);
    }

private:
    std::vector<float> buffer_;
    size_t wpos_{};
    size_t mask_{};
};

// ---------------------------------------- juce processor ----------------------------------------
class SteepFlangerAudioProcessor final : public juce::AudioProcessor
{
public:
    //==============================================================================
    SteepFlangerAudioProcessor();
    ~SteepFlangerAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    JuceParamListener param_listener_;
    std::unique_ptr<juce::AudioProcessorValueTreeState> value_tree_;

    static constexpr size_t kSIMDMaxCoeffLen = ((kMaxCoeffLen + 3) / 4) * 4;

    Vec4DelayLine delay_left_;
    Vec4DelayLine delay_right_;
    qwqdsp::misc::ExpSmoother left_delay_smoother_;
    qwqdsp::misc::ExpSmoother right_delay_smoother_;
    alignas(16) std::array<float, kSIMDMaxCoeffLen> coeffs_{};
    std::array<float, kMaxCoeffLen> custom_coeffs_{};
    std::array<float, kMaxCoeffLen> custom_spectral_gains{};
    size_t coeff_len_{};
    size_t coeff_len_div_4_{};
    float side_lobe_{20};
    float cutoff_w_{};

    // lfo
    float phase_{};
    float phase_inc_{};
    float delay_samples_{};
    float depth_samples_{};
    float phase_shift_{};

    // feedback
    float feedback_value_{};
    float feedback_mul_{};
    bool feedback_enable_{};
    float left_fb_{};
    float right_fb_{};
    qwqdsp::filter::Biquad left_damp_;
    qwqdsp::filter::Biquad right_damp_;

    // barberpole
    bool barber_enable_{};
    float barber_phase_{};
    float barber_phase_inc_{};
    qwqdsp::filter::IIRHilbertDeeperCpx<> left_hilbert_;
    qwqdsp::filter::IIRHilbertDeeperCpx<> right_hilbert_;
    qwqdsp::misc::ExpSmoother barber_phase_smoother_;

    bool minum_phase_{};
    bool highpass_{};
    bool is_using_custom_{};
    qwqdsp::spectral::ComplexFFT complex_fft_;

    EditorUpdate editor_update_;

    void UpdateCoeff();
    void PostCoeffsProcessing();
    void UpdateFeedback();
    void Panic();

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SteepFlangerAudioProcessor)
};
