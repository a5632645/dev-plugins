#pragma once
#include <span>
#include "qwqdsp/oscillator/smooth_noise.hpp"
#include "qwqdsp/fx/delay_line.hpp"
#include "qwqdsp/filter/fast_set_iir_paralle.hpp"

namespace dsp {

class Ensemble {
public:
    static constexpr int kMaxVoices = 16;
    static constexpr float kMaxSemitone = 0.15f;
    static constexpr float kMinFrequency = 0.01f;
    static constexpr float kMaxTime = 50.0f;
    static constexpr float kMinTime = 15.0f;

    enum class Mode {
        Sine,
        Noise,
    };

    void Init(float sample_rate);
    void SetNumVoices(int num_voices);
    void SetDetune(float pitch);
    void SetRate(float rate);
    void SetSperead(float spread);
    void SetMix(float mix);
    void SetMode(Mode mode);

    void Process(std::span<float> block, std::span<float> right);
private:
    void CalcCurrDelayLen();

    int num_voices_{};
    float spread_{};
    float mix_{};
    float rate_{};
    float detune_{};
    Mode mode_{};

    float lfo_freq_{};
    float sample_rate_{};
    float lfo_phase_{};
    float current_delay_len_{};
    float gain_{};
    
    qwqdsp_fx::DelayLine<qwqdsp_fx::DelayLineInterp::Lagrange3rd> delay_;
    qwqdsp_oscillator::SmoothNoise noises_[kMaxVoices];
    qwqdsp_filter::FastSetIirParalle<qwqdsp_filter::fastset_coeff::Order2_1e7> delay_samples_smoother_;
};

}
