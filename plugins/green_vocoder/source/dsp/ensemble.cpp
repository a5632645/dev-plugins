#include "ensemble.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <cassert>

namespace green_vocoder::dsp {
template<size_t N>
static constexpr std::array<float, N> MakePanTable(size_t nvocice) {
    std::array<float, Ensemble::kMaxVoices> out{};
    if (nvocice == 2) {
        out[0] = -1.0f;
        out[1] = 1.0f;
    }
    else {
        float interval = 2.0f / (static_cast<float>(nvocice) - 1);
        float begin = -1.0f;
        for (size_t i = 0; i < nvocice; ++i) {
            out[i] = begin + static_cast<float>(i) * interval;
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
    size_t min_delay_len = static_cast<size_t>(4.0f + kMaxTime / 1000.0f * sample_rate);
    delay_.Init(min_delay_len);
    for (auto& n : noises_) {
        n.GetNoise().SetSeed(static_cast<uint32_t>(rand()));
        n.Reset();
    }
    delay_samples_smoother_.MakeFilter(sample_rate * 100.0f / 1000.0f);
    delay_samples_smoother_.Reset();
}

void Ensemble::SetNumVoices(int num_voices) {
    assert(num_voices % 4 == 0);
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

void Ensemble::Process(qwqdsp_simd_element::PackFloat<2>* main, size_t num_samples) {
    if (num_voices_ == 0) return;

    const auto& pans = kPanTable[static_cast<size_t>(num_voices_) - 2];
    delay_.WarpBuffer();

    if (mode_ == Mode::Sine) {
        const float phase_add = 1.0f / static_cast<float>(num_voices_);
        for (size_t i = 0; i < num_samples; ++i) {
            auto x = main[i];
            delay_.Push(x[0], x[1]);

            float wet_left = 0.0f;
            float wet_right = 0.0f;
            float current_delay_line = delay_samples_smoother_.Tick(current_delay_len_);
            for (int voice = 0; voice < num_voices_; voice += 4) {
                qwqdsp_simd_element::PackInt32<4> mul{
                    voice, voice + 1, voice + 2, voice + 3
                };
                auto voice_phase = mul.ToFloat() * phase_add + lfo_phase_;
                voice_phase = qwqdsp_simd_element::PackOps::Frac(voice_phase);
                auto sin = qwqdsp_simd_element::PackOps::Cos(voice_phase * std::numbers::pi_v<float> * 2.0f);
                sin = sin * 0.5f + 0.5f;
                auto delay = sin * current_delay_line;
                auto v = delay_.GetAfterPush(delay);
                qwqdsp_simd_element::PackFloat<4> pan{
                    pans[static_cast<uint32_t>(voice)], pans[static_cast<uint32_t>(voice + 1)], pans[static_cast<uint32_t>(voice + 2)], pans[static_cast<uint32_t>(voice + 3)]
                };
                pan *= spread_;
                wet_left += qwqdsp_simd_element::PackOps::ReduceAdd(v * (1.0f - pan) / 2.0f);
                wet_right += qwqdsp_simd_element::PackOps::ReduceAdd(v * (1.0f + pan) / 2.0f);
            }
            qwqdsp_simd_element::PackFloat<2> wet{wet_left, wet_right};
            main[i] = qwqdsp_simd_element::PackOps::Lerp(main[i], wet, mix_);

            lfo_phase_ += lfo_freq_;
            lfo_phase_ -= std::floor(lfo_phase_);
        }
    }
    else if (mode_ == Mode::Noise) {
        for (size_t i = 0; i < num_samples; ++i) {
            auto x = main[i];
            delay_.Push(x[0], x[1]);

            float wet_left = 0.0f;
            float wet_right = 0.0f;
            float current_delay_line = delay_samples_smoother_.Tick(current_delay_len_);
            size_t noise_idx = 0;
            for (int voice = 0; voice < num_voices_; voice += 4) {
                qwqdsp_simd_element::PackFloat<4> norm_len{
                    noises_[noise_idx].Tick(),
                    noises_[noise_idx].Tick(),
                    noises_[noise_idx + 1].Tick(),
                    noises_[noise_idx + 1].Tick()
                };
                noise_idx += 2;
                norm_len = norm_len * 0.5f + 0.5f;
                auto delay = norm_len * current_delay_line;
                auto v = delay_.GetAfterPush(delay);
                qwqdsp_simd_element::PackFloat<4> pan{
                    pans[static_cast<uint32_t>(voice)], pans[static_cast<uint32_t>(voice + 1)], pans[static_cast<uint32_t>(voice + 2)], pans[static_cast<uint32_t>(voice + 3)]
                };
                pan *= spread_;
                wet_left += qwqdsp_simd_element::PackOps::ReduceAdd(v * (1.0f - pan) / 2.0f);
                wet_right += qwqdsp_simd_element::PackOps::ReduceAdd(v * (1.0f + pan) / 2.0f);
            }
            qwqdsp_simd_element::PackFloat<2> wet{wet_left, wet_right};
            main[i] = qwqdsp_simd_element::PackOps::Lerp(main[i], wet, mix_);
    
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
        n.SetRate(rate * 2.0f, sample_rate_);
    }
}

}
