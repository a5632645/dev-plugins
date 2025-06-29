#include "channel_vocoder.hpp"
#include "utli.hpp"
#include <numbers>

namespace dsp {

void ChannelVocoder::Init(float sample_rate) {
    sample_rate_ = sample_rate;
    UpdateFilters();
}

void ChannelVocoder::SetNumBands(int bands) {
    if (bands > num_bans_) {
        for (int i = num_bans_; i < bands; ++i) {
            main_filters_[i].ClearLatch();
            side_filters_[i].ClearLatch();
        }
    }
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

void ChannelVocoder::SetQ(float q) {
    q_ = q;
    UpdateFilters();
}

void ChannelVocoder::UpdateFilters() {
    float pitch_begin = std::log(freq_begin_);
    float pitch_end = std::log(freq_end_);
    float pitch_interval = (pitch_end - pitch_begin) / (num_bans_ - 1.0f);
    for (int i = 0; i < num_bans_; ++i) {
        float pitch = pitch_begin + pitch_interval * i;
        float freq = std::exp(pitch);
        float omega = std::numbers::pi_v<float> * 2.0f * freq / sample_rate_;
        main_filters_[i].Set(omega, q_);
        side_filters_[i].CopyCoeffients(main_filters_[i]);
    }
}

void ChannelVocoder::ProcessBlock(std::span<float> main_v, std::span<float> side_v) {
    int nsamples = static_cast<int>(main_v.size());
    for (int i = 0; i < nsamples; ++i) {
        float in = main_v[i];
        float side_in = side_v[i];
        float out = 0.0f;
        for (int j = 0; j < num_bans_; ++j) {
            float main_curr = main_filters_[j].ProcessSingle(in);
            main_curr = main_curr * main_curr;
            float main_latch = main_peaks_[j];
            if (main_curr > main_latch) {
                main_latch = main_latch * attack_ + (1.0f - attack_) * main_curr;
            }
            else {
                main_latch = main_latch * release_ + (1.0f - release_) * main_curr;
            }
            main_peaks_[j] = main_latch;

            float side_curr = side_filters_[j].ProcessSingle(side_in);
            out += side_curr * std::sqrt(main_latch + 1e-18f);
        }
        main_v[i] = out / num_bans_;
    }
}

}