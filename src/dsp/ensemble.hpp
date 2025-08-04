/*
 * TODO: 
 *       add delay controls
*/

#pragma once
#include "noise.hpp"
#include <span>
#include <vector>

namespace dsp {

class Ensemble {
public:
    static constexpr int kMaxVoices = 16;
    static constexpr float kMaxSemitone = 0.5f;
    static constexpr float kMinFrequency = 0.1f;
    static constexpr float kMaxTime = 30.0f;

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
    
    std::vector<float> buffer_;
    int buffer_wpos_{};
    int buffer_len_mask_{};
    Noise noises_[kMaxVoices];
};

}