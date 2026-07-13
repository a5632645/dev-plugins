#pragma once
#include <bit>
#include <juce_audio_processors/juce_audio_processors.h>

class JuceParamListener {
public:
    static constexpr size_t kBitCount = sizeof(uint64_t) * 8;
    static constexpr size_t kInitSize = 256;

    struct Callable {
        virtual ~Callable() = default;
        virtual void Call() = 0;

        Callable(JuceParamListener& p)
            : parent(p) {}

        JuceParamListener& parent;
        size_t scalar_idx;
        uint64_t mask;
    };

    JuceParamListener() {
        callbacks_.reserve(kInitSize);
        listeners_.reserve(kInitSize);
        dirty_marks_.reserve(kInitSize / kBitCount);
    }

    void MarkAll() {
        if (dirty_marks_.empty()) return;

        size_t last = callbacks_.size() % kBitCount;
        for (size_t i = 0; i < dirty_marks_.size() - 1; ++i) {
            std::atomic_ref<uint64_t>(dirty_marks_[i]).exchange(std::numeric_limits<uint64_t>::max());
        }
        uint64_t fillMask = (static_cast<uint64_t>(1) << last) - 1;
        if (last == 0) fillMask = std::numeric_limits<uint64_t>::max();
        std::atomic_ref<uint64_t>(dirty_marks_.back()).exchange(fillMask);
    }

    void Mark(size_t scalar, size_t mask) {
        std::atomic_ref<uint64_t>(dirty_marks_[scalar]).fetch_or(mask);
    }

    void Add(const std::unique_ptr<juce::AudioParameterFloat>& p, std::function<void(float)> func) {
        class FloatStore
            : public juce::AudioProcessorParameter::Listener
            , public Callable {
        public:
            FloatStore(JuceParamListener& p, std::function<void(float)> _func, juce::AudioParameterFloat* _ptr)
                : Callable(p)
                , func(_func)
                , ptr(_ptr) {
                ptr->addListener(this);
            }
            ~FloatStore() override {
                ptr->removeListener(this);
            }
            void parameterValueChanged(int parameterIndex, float newValue) override {
                // func(ptr->get());
                (void)parameterIndex;
                (void)newValue;
                parent.Mark(scalar_idx, mask);
            }
            void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {
                (void)parameterIndex;
                (void)gestureIsStarting;
            }
            void Call() override {
                func(ptr->get());
            }
        private:
            std::function<void(float)> func;
            juce::AudioParameterFloat* ptr;
        };

        [[maybe_unused]] const auto [_, _2] = AddCallback(std::make_unique<FloatStore>(*this, func, p.get()));
    }
    void Add(const std::unique_ptr<juce::AudioParameterBool>& p, std::function<void(bool)> func) {
        struct BoolStore
            : public juce::AudioProcessorParameter::Listener
            , public Callable {
            std::function<void(bool)> func;
            juce::AudioParameterBool* ptr;

            BoolStore(JuceParamListener& p, std::function<void(bool)> _func, juce::AudioParameterBool* _ptr)
                : Callable(p)
                , func(_func)
                , ptr(_ptr) {
                ptr->addListener(this);
            }
            ~BoolStore() override {
                ptr->removeListener(this);
            }
            void parameterValueChanged(int parameterIndex, float newValue) override {
                // func(ptr->get());
                (void)parameterIndex;
                (void)newValue;
                parent.Mark(scalar_idx, mask);
            }
            void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {
                (void)parameterIndex;
                (void)gestureIsStarting;
            }
            void Call() override {
                func(ptr->get());
            }
        };

        [[maybe_unused]] const auto [_, _2] = AddCallback(std::make_unique<BoolStore>(*this, func, p.get()));
    }
    void Add(const std::unique_ptr<juce::AudioParameterInt>& p, std::function<void(int)> func) {
        struct IntStore
            : public juce::AudioProcessorParameter::Listener
            , public Callable {
            std::function<void(int)> func;
            juce::AudioParameterInt* ptr;

            IntStore(JuceParamListener& p, std::function<void(int)> _func, juce::AudioParameterInt* _ptr)
                : Callable(p)
                , func(_func)
                , ptr(_ptr) {
                ptr->addListener(this);
            }
            ~IntStore() override {
                ptr->removeListener(this);
            }
            void parameterValueChanged(int parameterIndex, float newValue) override {
                // func(ptr->get());
                (void)parameterIndex;
                (void)newValue;
                parent.Mark(scalar_idx, mask);
            }
            void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {
                (void)parameterIndex;
                (void)gestureIsStarting;
            }
            void Call() override {
                func(ptr->get());
            }
        };

        [[maybe_unused]] const auto [_, _2] = AddCallback(std::make_unique<IntStore>(*this, func, p.get()));
    }
    void Add(const std::unique_ptr<juce::AudioParameterChoice>& p, std::function<void(int)> func) {
        struct ChoiceStore
            : public juce::AudioProcessorParameter::Listener
            , public Callable {
            std::function<void(int)> func;
            juce::AudioParameterChoice* ptr;

            ChoiceStore(JuceParamListener& p, std::function<void(int)> _func, juce::AudioParameterChoice* _ptr)
                : Callable(p)
                , func(_func)
                , ptr(_ptr) {
                ptr->addListener(this);
            }
            ~ChoiceStore() override {
                ptr->removeListener(this);
            }
            void parameterValueChanged(int parameterIndex, float newValue) override {
                // func(ptr->getIndex());
                parent.Mark(scalar_idx, mask);
                (void)parameterIndex;
                (void)newValue;
            }
            void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {
                (void)parameterIndex;
                (void)gestureIsStarting;
            }
            void Call() override {
                func(ptr->getIndex());
            }
        };

        [[maybe_unused]] const auto [_, _2] = AddCallback(std::make_unique<ChoiceStore>(*this, func, p.get()));
    }

    [[nodiscard]]
    std::pair<size_t, uint64_t> AddCallback(std::unique_ptr<Callable> p) {
        auto& val = callbacks_.emplace_back(std::move(p));
        size_t temp = callbacks_.size() - 1;
        size_t scalar = temp / kBitCount;
        size_t mask_idx = temp - scalar * kBitCount;
        uint64_t mask = static_cast<uint64_t>(1) << mask_idx;
        val->scalar_idx = scalar;
        val->mask = mask;
        if (mask_idx == 0) {
            dirty_marks_.push_back(0);
        }
        return {scalar, mask};
    }

    void AddParamCauseCallback(juce::RangedAudioParameter* param, size_t scalar, size_t mask) {
        struct A : juce::AudioProcessorParameter::Listener {
            juce::RangedAudioParameter& param;
            JuceParamListener& parent;
            size_t scalar;
            uint64_t mask;

            A(juce::RangedAudioParameter& p2, JuceParamListener& p, size_t s, uint64_t m)
                : param(p2)
                , parent(p)
                , scalar(s)
                , mask(m) {
                p2.addListener(this);
            }

            ~A() override {
                param.removeListener(this);
            }

            void parameterValueChanged(int parameterIndex, float newValue) override {
                // func(ptr->getIndex());
                parent.Mark(scalar, mask);
                (void)parameterIndex;
                (void)newValue;
            }
            void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {
                (void)parameterIndex;
                (void)gestureIsStarting;
            }
        };

        jassert(param != nullptr);
        listeners_.emplace_back(std::make_unique<A>(*param, *this, scalar, mask));
    }

    void HandleDirty() {
        uint64_t base_idx = 0;
        for (auto& s : dirty_marks_) {
            uint64_t dirty_mask = std::atomic_ref<uint64_t>(s).exchange(0);
            while (dirty_mask != 0) {
                uint64_t idx = static_cast<uint64_t>(std::countr_zero(dirty_mask));
                dirty_mask &= ~(static_cast<uint64_t>(1) << idx);

                idx += base_idx;
                callbacks_[idx]->Call();
            }

            base_idx += kBitCount;
        }
    }

    void Clear() {
        listeners_.clear();
        callbacks_.clear();
        dirty_marks_.clear();
    }
private:
    std::vector<std::unique_ptr<Callable>> callbacks_;
    std::vector<std::unique_ptr<juce::AudioProcessorParameter::Listener>> listeners_;
    std::vector<uint64_t> dirty_marks_;
};
