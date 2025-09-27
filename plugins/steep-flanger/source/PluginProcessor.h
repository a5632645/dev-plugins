#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <cstdlib>

#include "qwqdsp/misc/smoother.hpp"
#include "qwqdsp/spectral/complex_fft.hpp"
#include "qwqdsp/filter/biquad.hpp"
#include "qwqdsp/filter/iir_cpx_hilbert.hpp"
#include "qwqdsp/osciilor/vic_sine_osc.hpp"

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

    constexpr float ReduceAdd() const noexcept {
        return x[0] + x[1] + x[2] + x[3];
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

// copied from https://github.com/AutoPas/AutoPas/blob/0ea349ddb6c6048e1d00b753864e2c6fedd5b74a/src/autopas/utils/AlignedAllocator.h#L29
// add MSVC & others
template <class T, size_t Alignment>
class AlignedAllocator {
public:
    // needed for compatibility with stl::allocator
    /// value type
    using value_type = T;
    /// pointer type
    using pointer = T *;
    /// const pointer type
    using const_pointer = const T *;
    /// reference type
    using reference = T &;
    /// const reference type
    using const_reference = const T &;
    /// size type
    using size_type = size_t;

    /**
    * Equivalent allocator for other types
    * Class whose member other is an alias of allocator for type U.
    * (from cplusplus.com)
    * @tparam U
    */
    template <class U>
    struct rebind {
        /// other
        using other = AlignedAllocator<U, Alignment>;
    };

    /**
    * \brief Default empty constructor
    */
    AlignedAllocator() = default;

    /**
    * \brief Copy constructor
    */
    template <class U>
    AlignedAllocator(const AlignedAllocator<U, Alignment> &) {}

    /**
    * \brief Default destructor
    */
    ~AlignedAllocator() = default;

    /**
    * \brief Returns maximum possible value of n, with which we can call
    * allocate(n)
    * \return maximum size possible to allocate
    */
    size_t max_size() const noexcept { return (std::numeric_limits<size_t>::max() - size_t(Alignment)) / sizeof(T); }

    /**
    * \brief Allocate aligned memory for n objects of type T
    * \param n size to allocate
    * \return Pointer to the allocated memory
    */
    T *allocate(std::size_t n) {
        void* ptr{};
#ifdef _MSC_VER
        ptr = _aligned_malloc(n * sizeof(T), Alignment);
#else
        ptr = std::aligned_alloc(Alignment, n * sizeof(T));
#endif
        if (ptr == nullptr) {
            throw std::bad_alloc{};
        }
        return reinterpret_cast<T*>(ptr);
    }

    /**
    * \brief Deallocate memory pointed to by ptr
    * \param ptr pointer to deallocate
    */
    void deallocate(T *ptr, std::size_t /*n*/) {
#ifdef _MSC_VER
        _aligned_free(ptr);
#else
        free(ptr);
#endif
    }

    /**
    * \brief Construct object of type U at already allocated memory, pointed to
    * by p
    * \param p pointer to the object
    * \param args arguments for the construction
    */
    template <class U, class... Args>
    void construct(U *p, Args &&...args) {
#if _HAS_CXX20
        std::construct_at(p, std::forward<Args...>(args)...);
#else // ^^^ _HAS_CXX20 / !_HAS_CXX20 vvv
        ::new (static_cast<void*>(p)) U(std::forward<Args...>(args)...);
#endif // ^^^ !_HAS_CXX20 ^^^
    }

    /**
    * \brief Destroy object pointed to by p, but does not deallocate the memory
    * \param p pointer to the object that should be destroyed
    */
    template <class U>
    void destroy(U *p) {
        p->~U();
    }
};


class Vec4DelayLine {
public:
    void Init(float max_ms, float fs) {
        float d = max_ms * fs / 1000.0f;
        size_t i = static_cast<size_t>(std::ceil(d));
        Init(i);
    }

    void Init(size_t max_samples) {
        size_t a = 1;
        while (a < max_samples) {
            a *= 2;
        }
        mask_ = a - 1;
        delay_length_ = a;

        a += 4;
        if (buffer_.size() < a) {
            buffer_.resize(a);
        }
        Reset();
    }

    void Reset() noexcept {
        wpos_ = 0;
        std::fill(buffer_.begin(), buffer_.end(), 0.0f);
    }

    // void Push(float x) noexcept {
    //     buffer_[wpos_] = x;
    // }

    // void PushFinish() noexcept {
    //     ++wpos_;
    //     wpos_ &= mask_;
    // }

    // Vec4 GetAfterPush(Vec4 delay_samples) noexcept {
    //     Vec4 frpos = Vec4::FromSingle(wpos_ + buffer_.size()) - delay_samples;
    //     Vec4i32 rpos = frpos.ToInt();
    //     Vec4i32 mask = Vec4i32::FromSingle(mask_);
    //     Vec4i32 irpos = rpos & mask;
    //     Vec4i32 inext1 = (rpos + Vec4i32::FromSingle(1)) & mask;
    //     Vec4i32 inext2 = (rpos + Vec4i32::FromSingle(2)) & mask;
    //     Vec4i32 inext3 = (rpos + Vec4i32::FromSingle(3)) & mask;
    //     Vec4 frac = frpos.Frac();

    //     Vec4 y0;
    //     y0.x[0] = buffer_[irpos.x[0]];
    //     y0.x[1] = buffer_[irpos.x[1]];
    //     y0.x[2] = buffer_[irpos.x[2]];
    //     y0.x[3] = buffer_[irpos.x[3]];
    //     Vec4 y1;
    //     y1.x[0] = buffer_[inext1.x[0]];
    //     y1.x[1] = buffer_[inext1.x[1]];
    //     y1.x[2] = buffer_[inext1.x[2]];
    //     y1.x[3] = buffer_[inext1.x[3]];
    //     Vec4 y2;
    //     y2.x[0] = buffer_[inext2.x[0]];
    //     y2.x[1] = buffer_[inext2.x[1]];
    //     y2.x[2] = buffer_[inext2.x[2]];
    //     y2.x[3] = buffer_[inext2.x[3]];
    //     Vec4 y3;
    //     y3.x[0] = buffer_[inext3.x[0]];
    //     y3.x[1] = buffer_[inext3.x[1]];
    //     y3.x[2] = buffer_[inext3.x[2]];
    //     y3.x[3] = buffer_[inext3.x[3]];

    //     Vec4 d1 = frac - Vec4::FromSingle(1.0f);
    //     Vec4 d2 = frac - Vec4::FromSingle(2.0f);
    //     Vec4 d3 = frac - Vec4::FromSingle(3.0f);

    //     auto c1 = d1 * d2 * d3 / Vec4::FromSingle(-6.0f);
    //     auto c2 = d2 * d3 * Vec4::FromSingle(0.5f);
    //     auto c3 = d1 * d3 * Vec4::FromSingle(-0.5f);
    //     auto c4 = d1 * d2 / Vec4::FromSingle(6.0f);

    //     return y0 * c1 + frac * (y1 * c2 + y2 * c3 + y3 * c4);
    // }

    Vec4 GetRaw(Vec4 frpos) noexcept {
        // Vec4 frpos = Vec4::FromSingle(wpos_ + buffer_.size()) - delay_samples;
        Vec4i32 rpos = frpos.ToInt();
        Vec4i32 mask = Vec4i32::FromSingle(mask_);
        Vec4i32 irpos = rpos & mask;
        // Vec4i32 inext1 = (rpos + Vec4i32::FromSingle(1)) & mask;
        // Vec4i32 inext2 = (rpos + Vec4i32::FromSingle(2)) & mask;
        // Vec4i32 inext3 = (rpos + Vec4i32::FromSingle(3)) & mask;
        Vec4 frac = frpos.Frac();

        Vec4 interp0;
        interp0.x[0] = buffer_[irpos.x[0]];
        interp0.x[1] = buffer_[irpos.x[0] + 1];
        interp0.x[2] = buffer_[irpos.x[0] + 2];
        interp0.x[3] = buffer_[irpos.x[0] + 3];
        Vec4 interp1;
        interp1.x[0] = buffer_[irpos.x[1]];
        interp1.x[1] = buffer_[irpos.x[1] + 1];
        interp1.x[2] = buffer_[irpos.x[1] + 2];
        interp1.x[3] = buffer_[irpos.x[1] + 3];
        Vec4 interp2;
        interp2.x[0] = buffer_[irpos.x[2]];
        interp2.x[1] = buffer_[irpos.x[2] + 1];
        interp2.x[2] = buffer_[irpos.x[2] + 2];
        interp2.x[3] = buffer_[irpos.x[2] + 3];
        Vec4 interp3;
        interp3.x[0] = buffer_[irpos.x[3]];
        interp3.x[1] = buffer_[irpos.x[3] + 1];
        interp3.x[2] = buffer_[irpos.x[3] + 2];
        interp3.x[3] = buffer_[irpos.x[3] + 3];

        // Vec4 y0;
        // y0.x[0] = buffer_[irpos.x[0]];
        // y0.x[1] = buffer_[irpos.x[1]];
        // y0.x[2] = buffer_[irpos.x[2]];
        // y0.x[3] = buffer_[irpos.x[3]];
        // Vec4 y1;
        // y1.x[0] = buffer_[inext1.x[0]];
        // y1.x[1] = buffer_[inext1.x[1]];
        // y1.x[2] = buffer_[inext1.x[2]];
        // y1.x[3] = buffer_[inext1.x[3]];
        // Vec4 y2;
        // y2.x[0] = buffer_[inext2.x[0]];
        // y2.x[1] = buffer_[inext2.x[1]];
        // y2.x[2] = buffer_[inext2.x[2]];
        // y2.x[3] = buffer_[inext2.x[3]];
        // Vec4 y3;
        // y3.x[0] = buffer_[inext3.x[0]];
        // y3.x[1] = buffer_[inext3.x[1]];
        // y3.x[2] = buffer_[inext3.x[2]];
        // y3.x[3] = buffer_[inext3.x[3]];
        Vec4 y0;
        y0.x[0] = interp0.x[0];
        y0.x[1] = interp1.x[0];
        y0.x[2] = interp2.x[0];
        y0.x[3] = interp3.x[0];
        Vec4 y1;
        y1.x[0] = interp0.x[1];
        y1.x[1] = interp1.x[1];
        y1.x[2] = interp2.x[1];
        y1.x[3] = interp3.x[1];
        Vec4 y2;
        y2.x[0] = interp0.x[2];
        y2.x[1] = interp1.x[2];
        y2.x[2] = interp2.x[2];
        y2.x[3] = interp3.x[2];
        Vec4 y3;
        y3.x[0] = interp0.x[3];
        y3.x[1] = interp1.x[3];
        y3.x[2] = interp2.x[3];
        y3.x[3] = interp3.x[3];

        Vec4 d1 = frac - Vec4::FromSingle(1.0f);
        Vec4 d2 = frac - Vec4::FromSingle(2.0f);
        Vec4 d3 = frac - Vec4::FromSingle(3.0f);

        auto c1 = d1 * d2 * d3 / Vec4::FromSingle(-6.0f);
        auto c2 = d2 * d3 * Vec4::FromSingle(0.5f);
        auto c3 = d1 * d3 * Vec4::FromSingle(-0.5f);
        auto c4 = d1 * d2 / Vec4::FromSingle(6.0f);

        return y0 * c1 + frac * (y1 * c2 + y2 * c3 + y3 * c4);
    }

    void PushBlockNotChangeWpos(std::span<float> block) {
        size_t const can = buffer_.size() - wpos_;
        if (can < block.size()) {
            std::copy_n(block.begin(), can, buffer_.begin() + wpos_);
            std::copy(block.begin() + can, block.end(), buffer_.begin());
        }
        else {
            std::copy(block.begin(), block.end(), buffer_.begin() + wpos_);
        }
    }

    // 给优化开洞(
    std::vector<float, AlignedAllocator<float, 16>> buffer_;
    size_t delay_length_{};
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
    qwqdsp::filter::IIRHilbertDeeperCpx<> left_hilbert_;
    qwqdsp::filter::IIRHilbertDeeperCpx<> right_hilbert_;
    qwqdsp::misc::ExpSmoother barber_phase_smoother_;
    qwqdsp::oscillor::VicSineOsc barber_oscillator_;
    size_t barber_osc_keep_amp_counter_{};
    size_t barber_osc_keep_amp_need_{};

    bool minum_phase_{};
    bool highpass_{};
    bool is_using_custom_{};
    qwqdsp::spectral::ComplexFFT complex_fft_;

    EditorUpdate editor_update_;
#if HAVE_MEASUREMENT
    juce::AudioProcessLoadMeasurer measurer;
#endif

    void UpdateCoeff();
    void PostCoeffsProcessing();
    void UpdateFeedback();
    void Panic();

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SteepFlangerAudioProcessor)
};
