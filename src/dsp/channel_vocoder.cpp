#include "channel_vocoder.hpp"
#include "utli.hpp"
#include <numbers>

namespace dsp {

void ChannelVocoder::Init(float sample_rate) {
    sample_rate_ = sample_rate;
    UpdateFilters();
}

void ChannelVocoder::SetNumBands(int bands) {
    num_bans_ = bands;
    UpdateFilters();
}

void ChannelVocoder::SetFreqBegin(float begin) {
    freq_begin_ = begin;
    UpdateFilters();
}

void ChannelVocoder::SetFreqEnd(float end) {
    freq_end_ = end;
    UpdateFilters();
}

void ChannelVocoder::SetAttack(float attack) {
    attack_ = utli::GetDecayValue(sample_rate_, attack);
}

void ChannelVocoder::SetRelease(float release) {
    release_ = utli::GetDecayValue(sample_rate_, release);
}

void ChannelVocoder::SetModulatorScale(float scale) {
    scale_ = scale;
    UpdateFilters();
}

void ChannelVocoder::SetCarryScale(float scale) {
    carry_scale_ = scale;
    UpdateFilters();
}

struct LogMap {
    static float FromFreq(float freq) {
        return std::log(freq);
    }
    
    static float ToFreq(float log) {
        return std::exp(log);
    }
};

struct MelMap {
    static float FromFreq(float freq) {
        return 1127.0f * std::log(1.0f + freq / 700.0f);
    }
    
    static float ToFreq(float mel) {
        return 700.0f * (std::exp(mel / 1127.0f) - 1.0f);
    }
};
using AssignMap = LogMap;

void ChannelVocoder::UpdateFilters() {
    float pitch_begin = AssignMap::FromFreq(freq_begin_);
    float pitch_end = AssignMap::FromFreq(freq_end_);
    float pitch_interval = (pitch_end - pitch_begin) / num_bans_;
    float begin = 0.0f;
    for (int i = 0; i < num_bans_ + 1; ++i) {
        float pitch = pitch_begin + pitch_interval * i;
        float freq = AssignMap::ToFreq(pitch);
        float omega = std::numbers::pi_v<float> * 2.0f * freq / sample_rate_;
        if (i == 0) {
            begin = omega;
            continue;
        }

        float bw = omega - begin;
        float q1 = omega / (bw * scale_);
        float q2 = omega / (bw * carry_scale_);
        main_filters_[i - 1].MakeBandpass(omega, q1);
        side_filters_[i - 1].MakeBandpass(omega, q2);
        begin = omega;
    }
    float g1 = scale_ < 1.0f ? 1.0 / scale_ : 1.0f;
    float g2 = carry_scale_ < 1.0f ? 1.0f / carry_scale_ : carry_scale_;
    gain_ = g1 * g2 * 10.0f;
}

void ChannelVocoder::ProcessBlock(std::span<float> main_v, std::span<float> side_v) {
    int nsamples = static_cast<int>(main_v.size());
    for (int i = 0; i < nsamples; ++i) {
        float in = main_v[i];
        float side_in = side_v[i];
        float out = 0.0f;
        for (int j = 0; j < num_bans_; ++j) {
            float main_curr = main_filters_[j].Tick(in);
            main_curr = main_curr * main_curr;
            float main_latch = main_peaks_[j];
            if (main_curr > main_latch) {
                main_latch = main_latch * attack_ + (1.0f - attack_) * main_curr;
            }
            else {
                main_latch = main_latch * release_ + (1.0f - release_) * main_curr;
            }
            main_peaks_[j] = main_latch;

            float side_curr = side_filters_[j].Tick(side_in);
            out += side_curr * std::sqrt(main_latch + 1e-18f);
        }
        main_v[i] = out * gain_;
    }
}

}