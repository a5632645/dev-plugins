#pragma once
#include "qwqdsp/oscillator/smooth_noise.hpp"
#include <qwqdsp/simd_element/delay_line_stereo.hpp>
#include <qwqdsp/simd_element/delay_line_multiple.hpp>
#include <qwqdsp/simd_element/simd_pack.hpp>
#include "qwqdsp/filter/fast_set_iir_paralle.hpp"

namespace green_vocoder::dsp {

class Ensemble {
public:
    static constexpr int kMaxVoices = 16;
    static constexpr float kMaxSemitone = 0.2f;
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
    void Process(qwqdsp_simd_element::PackFloat<2>* main, size_t num_samples);
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
    
    qwqdsp_simd_element::DelayLineStereo<4, false> delay_;
    qwqdsp_oscillator::SmoothNoise noises_[kMaxVoices / 2];
    qwqdsp_filter::FastSetIirParalle<qwqdsp_filter::fastset_coeff::Order2_1e7> delay_samples_smoother_;
};

}
