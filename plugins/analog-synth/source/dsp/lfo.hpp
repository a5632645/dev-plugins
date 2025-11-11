#pragma once
#include "imodulator.hpp"

#include <qwqdsp/polymath.hpp>
#include <qwqdsp/osciilor/smooth_noise.hpp>

namespace analogsynth {
class Lfo : public IModulator {
public:
    enum class Shape {
        Sine = 0,
        Tri,
        SawUp,
        SawDown,
        Noise,
        SmoothNoise,
        HoldNoise,
        NumShapes
    };

    struct Parameter {
        float phase_inc;
        Shape shape;
    };

    Lfo(juce::StringRef name)
        : IModulator(name) {
        noise_.GetNoise().SetSeed(static_cast<uint32_t>(rand()));
    }

    void Process(Parameter const& param, size_t num_samples) noexcept {
        switch (param.shape) {
            case Shape::Sine:
                for (size_t i = 0; i < num_samples; ++i) {
                    phase_ += param.phase_inc;
                    phase_ -= std::floor(phase_);
                    constexpr float twopi = std::numbers::pi_v<float> * 2;
                    constexpr float pi = std::numbers::pi_v<float>;
                    modulator_output[i] = qwqdsp::polymath::SinParabola(twopi * phase_ - pi);
                }
                break;
            case Shape::Tri:
                for (size_t i = 0; i < num_samples; ++i) {
                    phase_ += param.phase_inc;
                    phase_ -= std::floor(phase_);
                    modulator_output[i] = qwqdsp::polymath::Triangle(phase_);
                }
                break;
            case Shape::SawUp:
                for (size_t i = 0; i < num_samples; ++i) {
                    phase_ += param.phase_inc;
                    phase_ -= std::floor(phase_);
                    modulator_output[i] = phase_ * 2 - 1;
                }
                break;
            case Shape::SawDown:
                for (size_t i = 0; i < num_samples; ++i) {
                    phase_ += param.phase_inc;
                    phase_ -= std::floor(phase_);
                    modulator_output[i] = 1 - phase_ * 2;
                }
                break;
            case Shape::Noise:
                for (size_t i = 0; i < num_samples; ++i) {
                    modulator_output[i] = noise_.GetNoise().Next();
                }
                break;
            case Shape::SmoothNoise:
                noise_.SetRate(param.phase_inc);
                for (size_t i = 0; i < num_samples; ++i) {
                    modulator_output[i] = noise_.Tick();
                }
                break;
            case Shape::HoldNoise:
                for (size_t i = 0; i < num_samples; ++i) {
                    phase_ += param.phase_inc;
                    if (phase_ > 1) {
                        noise_.GetNoise().NextUInt();
                        phase_ -= 1;
                    }
                    float val01 = static_cast<float>(noise_.GetNoise().GetReg()) * noise_.GetNoise().kScale;
                    modulator_output[i] = val01 * 2 - 1;
                }
                break;
            case Shape::NumShapes:
                break;
        }

        for (size_t i = 0; i < num_samples; ++i) {
            modulator_output[i] = 0.5f * modulator_output[i] + 0.5f;
        }
    }

    void Reset(float phase = 0.0f) noexcept {
        phase_ = phase;
    }
private:
    float phase_{};
    qwqdsp::oscillor::SmoothNoise noise_;
};
}