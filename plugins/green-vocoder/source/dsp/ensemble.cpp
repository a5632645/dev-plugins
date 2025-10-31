#include "ensemble.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <numbers>

namespace dsp {
template<size_t N>
static constexpr std::array<float, N> MakePanTable(int nvocice) {
    std::array<float, Ensemble::kMaxVoices> out{};
    if (nvocice == 2) {
        out[0] = -1.0f;
        out[1] = 1.0f;
    }
    else {
        float interval = 2.0f / (nvocice - 1);
        float begin = -1.0f;
        for (int i = 0; i < nvocice; ++i) {
            out[i] = begin + i * interval;
        }
    }
    return out;
}

static constexpr std::array<std::array<float, Ensemble::kMaxVoices>, Ensemble::kMaxVoices>
kPanTable{
    MakePanTable<Ensemble::kMaxVoices>(2),
    MakePanTable<Ensemble::kMaxVoices>(3),
    MakePanTable<Ensemble::kMaxVoices>(4),
    MakePanTable<Ensemble::kMaxVoices>(5),
    MakePanTable<Ensemble::kMaxVoices>(6),
    MakePanTable<Ensemble::kMaxVoices>(7),
    MakePanTable<Ensemble::kMaxVoices>(8),
    MakePanTable<Ensemble::kMaxVoices>(9),
    MakePanTable<Ensemble::kMaxVoices>(10),
    MakePanTable<Ensemble::kMaxVoices>(11),
    MakePanTable<Ensemble::kMaxVoices>(12),
    MakePanTable<Ensemble::kMaxVoices>(13),
    MakePanTable<Ensemble::kMaxVoices>(14),
    MakePanTable<Ensemble::kMaxVoices>(15),
    MakePanTable<Ensemble::kMaxVoices>(16),
};

void Ensemble::Init(float sample_rate) {
    sample_rate_ = sample_rate;
    int min_delay_len = static_cast<int>(4.0f + kMaxTime / 1000.0f * sample_rate);
    delay_.Init(min_delay_len);
    for (auto& n : noises_) {
        n.GetNoise().SetSeed(rand());
        n.Reset();
    }
    delay_samples_smoother_.MakeFilter(sample_rate * 100.0f / 1000.0f);
    delay_samples_smoother_.Reset();
}

void Ensemble::SetNumVoices(int num_voices) {
    num_voices_ = num_voices;
    gain_ = 2.0f / std::sqrt(static_cast<float>(num_voices));
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
}

void Ensemble::SetMode(Ensemble::Mode mode) {
    mode_ = mode;
}

void Ensemble::Process(std::span<float> block, std::span<float> right) {
    const auto& pans = kPanTable[num_voices_ - 2];
    if (mode_ == Mode::Sine) {
        const float phase_add = 1.0f / num_voices_;
        int size = static_cast<int>(block.size());
        for (int i = 0; i < size; ++i) {
            float in = block[i];
            delay_.Push(in);
    
            float wet_left = 0.0f;
            float wet_right = 0.0f;
            float current_delay_line = delay_samples_smoother_.Tick(current_delay_len_);
            for (int voice = 0; voice < num_voices_; ++voice) {
                float voice_phase = voice * phase_add + lfo_phase_;
                voice_phase -= std::floor(voice_phase);
    
                float sin = std::cos(voice_phase * std::numbers::pi_v<float> * 2.0f);
                sin = sin * 0.5f + 0.5f;
                float delay = sin * current_delay_line;
            
                float v = delay_.GetAfterPush(delay);
    
                float pan = pans[voice] * spread_;
                wet_left += v * (1.0f - pan) / 2.0f;
                wet_right += v * (1.0f + pan) / 2.0f;
            }
            block[i] = std::lerp(in, wet_left * gain_, mix_);
            right[i] = std::lerp(in, wet_right * gain_, mix_);
    
            lfo_phase_ += lfo_freq_;
            lfo_phase_ -= std::floor(lfo_phase_);
        }
    }
    else if (mode_ == Mode::Noise) {
        int size = static_cast<int>(block.size());
        for (int i = 0; i < size; ++i) {
            float in = block[i];
            delay_.Push(in);
    
            float wet_left = 0.0f;
            float wet_right = 0.0f;
            float current_delay_line = delay_samples_smoother_.Tick(current_delay_len_);
            for (int voice = 0; voice < num_voices_; ++voice) {
                float norm_len = noises_[voice].Tick() * 0.5f + 0.5f;
                float delay = norm_len * current_delay_line;
                float v = delay_.GetAfterPush(delay);
    
                float pan = pans[voice] * spread_;
                wet_left += v * (1.0f - pan) / 2.0f;
                wet_right += v * (1.0f + pan) / 2.0f;
            }
            block[i] = std::lerp(in, wet_left * gain_, mix_);
            right[i] = std::lerp(in, wet_right * gain_, mix_);
    
            lfo_phase_ += lfo_freq_;
            lfo_phase_ -= std::floor(lfo_phase_);
        }
    }
}

void Ensemble::CalcCurrDelayLen() {
    float mul = std::exp2(detune_ / 12.0f);
    float delay_s = (mul - 1.0f) / rate_;
    float rate = rate_;
    if (delay_s * 1000.0f > kMaxTime) {
        rate = (mul - 1.0f) / (kMaxTime / 1000.0f);
        delay_s = kMaxTime / 1000.0f;
    }
    if (delay_s * 1000.0f < kMinTime) {
        rate = (mul - 1.0f) / (kMinTime / 1000.0f);
        delay_s = kMinTime / 1000.0f;
    }
    if (rate < kMinFrequency) {
        rate = kMinFrequency;
    }
    current_delay_len_ = delay_s * sample_rate_;
    lfo_freq_ = rate / sample_rate_;
    for (auto& n : noises_) {
        n.SetRate(rate * 5.0f, sample_rate_);
    }
}

}