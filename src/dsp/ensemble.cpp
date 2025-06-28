#include "ensemble.hpp"

#include <cmath>
#include <numbers>

namespace dsp {

void Ensemble::Init(float sample_rate) {
    sample_rate_ = sample_rate;
    float mul = std::exp2(kMaxSemitone / 12.0f);
    float min_delay_s = (mul - 1.0f) / kMinFrequency;
    int min_delay_len = static_cast<int>(0.5f + min_delay_s * sample_rate);

    int power = 1;
    while (power <= min_delay_len) {
        power *= 2;
    }
    buffer_.resize(power);
    buffer_len_mask_ = power - 1;
}

void Ensemble::SetNumVoices(int num_voices) {
    num_voices_ = num_voices;
}

void Ensemble::SetDetune(float detune) {
    detune_ = detune;
    CalcCurrDelayLen();
}

void Ensemble::SetSperead(float spread) {
    spread_ = spread;
}

void Ensemble::SetMix(float mix) {
    mix_ = mix;
}

void Ensemble::SetRate(float rate) {
    rate_ = rate;
    CalcCurrDelayLen();
    lfo_freq_ = rate / sample_rate_;
}

void Ensemble::Process(std::span<float> block, std::span<float> right) {
    float pans[kMaxVoices]{};
    float pan_interval = 2.0f / (1.0f + num_voices_);
    float first_pan = pan_interval - 1.0f;
    for (int i = 0; i < num_voices_; ++i) {
        pans[i] = (first_pan + pan_interval * i) * spread_;
    }

    const float phase_add = 1.0f / num_voices_;
    int size = static_cast<int>(block.size());
    for (int i = 0; i < size; ++i) {
        float in = block[i];
        buffer_[buffer_wpos_] = in;
        buffer_.back() = buffer_.front();

        float wet_left = 0.0f;
        float wet_right = 0.0f;
        for (int voice = 0; voice < num_voices_; ++voice) {
            float voice_phase = voice * phase_add + lfo_phase_;
            voice_phase -= std::floor(voice_phase);

            float sin = std::cos(voice_phase * std::numbers::pi_v<float> * 2.0f);
            sin = sin * 0.5f + 0.5f;
            float delay = sin * current_delay_len_;
            
            float rpos = buffer_wpos_ - delay;
            int irpos = static_cast<int>(rpos) & buffer_len_mask_;
            int inext = (irpos + 1) & buffer_len_mask_;
            float frac = rpos - static_cast<int>(rpos);

            float v = std::lerp(buffer_[irpos], buffer_[inext], frac);

            float pan = pans[voice];
            wet_left += v * (1.0f - pan) / 2.0f;
            wet_right += v * (1.0f + pan) / 2.0f;
        }
        block[i] = std::lerp(in, wet_left, mix_);
        right[i] = std::lerp(in, wet_right, mix_);

        buffer_wpos_ = (buffer_wpos_ + 1) & buffer_len_mask_;
        lfo_phase_ += lfo_freq_;
        lfo_phase_ -= std::floor(lfo_phase_);
    }
}

void Ensemble::CalcCurrDelayLen() {
    float mul = std::exp2(detune_ / 12.0f);
    float delay_s = (mul - 1.0f) / rate_;
    current_delay_len_ = delay_s * sample_rate_;
}

}