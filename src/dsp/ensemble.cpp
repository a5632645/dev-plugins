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
    int min_delay_len = static_cast<int>(4.5f + kMaxTime / 1000.0f * sample_rate);

    int power = 1;
    while (power <= min_delay_len) {
        power *= 2;
    }
    buffer_.resize(power);
    buffer_len_mask_ = power - 1;

    for (auto& n : noises_) {
        n.Init(sample_rate);
        n.Reset();
    }
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
    // lfo_freq_ = rate_ / sample_rate_;

    // for (auto& n : noises_) {
    //     n.SetRate(rate_ * 5.0f);
    // }
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
            buffer_[buffer_wpos_] = in;
    
            float wet_left = 0.0f;
            float wet_right = 0.0f;
            for (int voice = 0; voice < num_voices_; ++voice) {
                float voice_phase = voice * phase_add + lfo_phase_;
                voice_phase -= std::floor(voice_phase);
    
                float sin = std::cos(voice_phase * std::numbers::pi_v<float> * 2.0f);
                sin = sin * 0.5f + 0.5f;
                float delay = sin * current_delay_len_ + 2; // give some delay to avoid read future value
                
                float rpos = buffer_wpos_ - delay;
                int irpos = static_cast<int>(rpos) & buffer_len_mask_;
                int inext1 = (irpos + 1) & buffer_len_mask_;
                int inext2 = (irpos + 2) & buffer_len_mask_;
                int iprev = (irpos - 1) & buffer_len_mask_;
                float t = rpos - static_cast<int>(rpos);
    
                // Cubic interpolation using four points
                float p0 = buffer_[iprev];
                float p1 = buffer_[irpos];
                float p2 = buffer_[inext1];
                float p3 = buffer_[inext2];
    
                float a = -p0 / 2.0f + 3.0f * p1 / 2.0f - 3.0f * p2 / 2.0f + p3 / 2.0f;
                float b = p0 - 5.0f * p1 / 2.0f + 2.0f * p2 - p3 / 2.0f;
                float c = -p0 / 2.0f + p2 / 2.0f;
                float d = p1;
                float v = a * t * t * t + b * t * t + c * t + d;
    
                float pan = pans[voice] * spread_;
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
    else if (mode_ == Mode::Noise) {
        int size = static_cast<int>(block.size());
        for (int i = 0; i < size; ++i) {
            float in = block[i];
            buffer_[buffer_wpos_] = in;
    
            float wet_left = 0.0f;
            float wet_right = 0.0f;
            for (int voice = 0; voice < num_voices_; ++voice) {
                float norm_len = noises_[voice].Tick();
                float delay = norm_len * current_delay_len_ + 2; // give some delay to avoid read future value
                
                float rpos = buffer_wpos_ - delay;
                int irpos = static_cast<int>(rpos) & buffer_len_mask_;
                int inext1 = (irpos + 1) & buffer_len_mask_;
                int inext2 = (irpos + 2) & buffer_len_mask_;
                int iprev = (irpos - 1) & buffer_len_mask_;
                float t = rpos - static_cast<int>(rpos);
    
                // Cubic interpolation using four points
                float p0 = buffer_[iprev];
                float p1 = buffer_[irpos];
                float p2 = buffer_[inext1];
                float p3 = buffer_[inext2];
    
                float a = -p0 / 2.0f + 3.0f * p1 / 2.0f - 3.0f * p2 / 2.0f + p3 / 2.0f;
                float b = p0 - 5.0f * p1 / 2.0f + 2.0f * p2 - p3 / 2.0f;
                float c = -p0 / 2.0f + p2 / 2.0f;
                float d = p1;
                float v = a * t * t * t + b * t * t + c * t + d;
    
                float pan = pans[voice] * spread_;
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
}

void Ensemble::CalcCurrDelayLen() {
    float mul = std::exp2(detune_ / 12.0f);
    float delay_s = (mul - 1.0f) / rate_;
    float rate = rate_;
    if (delay_s * 1000.0f > kMaxTime) {
        rate = (mul - 1.0f) / (kMaxTime / 1000.0f);
        delay_s = kMaxTime / 1000.0f;
    }
    current_delay_len_ = delay_s * sample_rate_;
    lfo_freq_ = rate / sample_rate_;
    for (auto& n : noises_) {
        n.SetRate(rate * 5.0f);
    }
}

}