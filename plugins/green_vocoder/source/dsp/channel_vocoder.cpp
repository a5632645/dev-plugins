#include "channel_vocoder.hpp"
#include "param_ids.hpp"
#include <cassert>
#include <cstddef>
#include <numbers>
#include <qwqdsp/convert.hpp>

namespace green_vocoder::dsp {

void ChannelVocoder::Init(
    float sample_rate,
    size_t block_size
) {
    sample_rate_ = sample_rate;
    output_.resize(block_size);
    UpdateFilters();
}

void ChannelVocoder::SetNumBands(
    int bands
) {
    assert(bands % 4 == 0);
    num_bans_ = bands;
    num_filters_ = static_cast<size_t>(bands) / 4;
    UpdateFilters();
}

void ChannelVocoder::SetFreqBegin(
    float begin
) {
    freq_begin_ = begin;
    UpdateFilters();
}

void ChannelVocoder::SetFreqEnd(
    float end
) {
    freq_end_ = end;
    UpdateFilters();
}

void ChannelVocoder::SetAttack(
    float attack
) {
    attack_ = qwqdsp::convert::Ms2DecayDb(attack, sample_rate_, -60.0f);
}

void ChannelVocoder::SetRelease(
    float release
) {
    release_ = qwqdsp::convert::Ms2DecayDb(release, sample_rate_, -60.0f);
}

void ChannelVocoder::SetModulatorScale(
    float scale
) {
    scale_ = scale;
    UpdateFilters();
}

void ChannelVocoder::SetCarryScale(
    float scale
) {
    carry_scale_ = scale;
    UpdateFilters();
}

void ChannelVocoder::SetMap(
    eChannelVocoderMap map
) {
    map_ = map;
    UpdateFilters();
}

void ChannelVocoder::SetFlat(bool flat) {
    flat_ = flat;
    UpdateFilters();
}

void ChannelVocoder::SetHighOrder(bool high_order) {
    high_order_ = high_order;
    UpdateFilters();
}

struct LogMap {
    static float FromFreq(float freq) {
        return std::log(freq);
    }

    static qwqdsp_simd_element::PackFloat<4> ToFreq(qwqdsp_simd_element::PackFloat<4> log) {
        return qwqdsp_simd_element::PackOps::Exp(log);
    }

    static qwqdsp_simd_element::PackFloat<4> FromFreq(qwqdsp_simd_element::PackFloat<4> log) {
        return qwqdsp_simd_element::PackOps::Log(log);
    }
    
    static float ToFreq(float log) {
        return std::exp(log);
    }
};

struct MelMap {
    static qwqdsp_simd_element::PackFloat<4> FromFreq(qwqdsp_simd_element::PackFloat<4> freq) {
        return 1127.0f * qwqdsp_simd_element::PackOps::Log(1.0f + freq / 700.0f);
    }

    static float FromFreq(float freq) {
        return 1127.0f * std::log(1.0f + freq / 700.0f);
    }

    static qwqdsp_simd_element::PackFloat<4> ToFreq(qwqdsp_simd_element::PackFloat<4> mel) {
        return 700.0f * (qwqdsp_simd_element::PackOps::Exp(mel / 1127.0f) - 1.0f);
    }
    
    static float ToFreq(float mel) {
        return 700.0f * (std::exp(mel / 1127.0f) - 1.0f);
    }
};

struct LinearMap {
    static qwqdsp_simd_element::PackFloat<4> FromFreq(qwqdsp_simd_element::PackFloat<4> freq) {
        return freq;
    }

    static float FromFreq(float freq) {
        return freq;
    }

    static qwqdsp_simd_element::PackFloat<4> ToFreq(qwqdsp_simd_element::PackFloat<4> freq) {
        return freq;
    }
    
    static float ToFreq(float freq) {
        return freq;
    }
};

void ChannelVocoder::UpdateFilters() {
    switch (map_) {
    case eChannelVocoderMap_Log:
        this->template _UpdateFilters<LogMap>();
        break;
    case eChannelVocoderMap_Linear:
        this->template _UpdateFilters<LinearMap>();
        break;
    case eChannelVocoderMap_Mel:
        this->template _UpdateFilters<MelMap>();
        break;
    default:
        assert(false);
        break;
    }
}

template<class AssignMap>
void ChannelVocoder::_UpdateFilters() {
    if (flat_) {
        this->template _UpdateFilters2<AssignMap, true>();
    }
    else {
        this->template _UpdateFilters2<AssignMap, false>();
    }
}

template<class AssignMap, bool kFlat>
void ChannelVocoder::_UpdateFilters2() {
    float pitch_begin = AssignMap::FromFreq(freq_begin_);
    float pitch_end = AssignMap::FromFreq(freq_end_);
    float pitch_interval = (pitch_end - pitch_begin) / static_cast<float>(num_bans_);
    float begin = qwqdsp::convert::Freq2W(freq_begin_, sample_rate_);

    size_t filter_idx = 0;
    auto const min_w = qwqdsp_simd_element::PackFloat<4>::vBroadcast(qwqdsp::convert::Freq2W(10.0f, sample_rate_));
    auto const max_w = qwqdsp_simd_element::PackFloat<4>::vBroadcast(qwqdsp::convert::Freq2W(sample_rate_ / 2 - 100.0f, sample_rate_));
    for (int i = 0; i < num_bans_; i += 4) {
        qwqdsp_simd_element::PackFloat<4> end_omega{
            pitch_begin + pitch_interval * (i + 1),
            pitch_begin + pitch_interval * (i + 2),
            pitch_begin + pitch_interval * (i + 3),
            pitch_begin + pitch_interval * (i + 4)
        };
        end_omega = AssignMap::ToFreq(end_omega);
        end_omega = std::numbers::pi_v<float> * 2.0f * end_omega / sample_rate_;
        qwqdsp_simd_element::PackFloat<4> prev_omega{
            begin,
            end_omega[0],
            end_omega[1],
            end_omega[2]
        };
        begin = end_omega[3];

        if constexpr (kFlat) {
            auto center = (prev_omega + end_omega) * 0.5f;
            auto half_bw = end_omega - center;
            auto main_half_bw = half_bw * scale_;
            auto main_w1 = center - main_half_bw;
            auto main_w2 = center + main_half_bw;
            auto side_half_bw = half_bw * carry_scale_;
            auto side_w1 = center - side_half_bw;
            auto side_w2 = center + side_half_bw;
            main_w1 = qwqdsp_simd_element::PackOps::Clamp(main_w1, min_w, max_w);
            main_w2 = qwqdsp_simd_element::PackOps::Clamp(main_w2, min_w, max_w);
            side_w1 = qwqdsp_simd_element::PackOps::Clamp(side_w1, min_w, max_w);
            side_w2 = qwqdsp_simd_element::PackOps::Clamp(side_w2, min_w, max_w);
            if (high_order_) {
                main_filters_[filter_idx].MakeBandpassFlat<true>(main_w1, main_w2);
                side_filters_[filter_idx].MakeBandpassFlat<true>(side_w1, side_w2);
            }
            else {
                main_filters_[filter_idx].MakeBandpassFlat<false>(main_w1, main_w2);
                side_filters_[filter_idx].MakeBandpassFlat<false>(side_w1, side_w2);
            }
        }
        else {
            auto bw = end_omega - prev_omega;
            auto cutoff = qwqdsp_simd_element::PackOps::Sqrt(end_omega * prev_omega);
            if (high_order_) {
                main_filters_[filter_idx].MakeBandpass<true>(cutoff, bw * scale_);
                side_filters_[filter_idx].MakeBandpass<true>(cutoff, bw * carry_scale_);
            }
            else {
                main_filters_[filter_idx].MakeBandpass<false>(cutoff, bw * scale_);
                side_filters_[filter_idx].MakeBandpass<false>(cutoff, bw * carry_scale_);
            }
        }
        ++filter_idx;
    }
    float g1 = scale_ < 1.0f ? 1.0f / scale_ : 1.0f;
    float g2 = carry_scale_ < 1.0f ? 1.0f / carry_scale_ : carry_scale_;
    gain_ = g1 * g2 * (num_bans_ > 8 ? 10.0f : 2.0f);
}

void ChannelVocoder::ProcessBlock(
    qwqdsp_simd_element::PackFloat<2>* main,
    qwqdsp_simd_element::PackFloat<2>* side,
    size_t num_samples
) {
    if (high_order_) {
        this->template _ProcessBlock<true>(main, side, num_samples);
    }
    else {
        this->template _ProcessBlock<false>(main, side, num_samples);
    }
}

template<bool kHigherOrder>
void ChannelVocoder::_ProcessBlock(
    qwqdsp_simd_element::PackFloat<2>* main,
    qwqdsp_simd_element::PackFloat<2>* side,
    size_t num_samples
) {
    std::fill_n(output_.data(), num_samples, qwqdsp_simd_element::PackFloat<2>{});
    for (size_t filter_idx = 0; filter_idx < num_filters_; ++filter_idx) {
        for (size_t sample_idx = 0; sample_idx < num_samples; ++sample_idx) {
            // filtering
            qwqdsp_simd_element::PackFloat<4> main_l;
            qwqdsp_simd_element::PackFloat<4> main_r;
            qwqdsp_simd_element::PackFloat<4> side_l;
            qwqdsp_simd_element::PackFloat<4> side_r;
            main_l.Broadcast(main[sample_idx][0]);
            main_r.Broadcast(main[sample_idx][1]);
            side_l.Broadcast(side[sample_idx][0]);
            side_r.Broadcast(side[sample_idx][1]);
            main_filters_[filter_idx].Tick<kHigherOrder>(main_l, main_r);
            side_filters_[filter_idx].Tick<kHigherOrder>(side_l, side_r);
            // envelope follower
            auto curr = main_peaks_[filter_idx];
            main_l = qwqdsp_simd_element::PackOps::Abs(main_l);
            main_r = qwqdsp_simd_element::PackOps::Abs(main_r);
            auto lag_l = curr[0];
            auto lag_r = curr[1];
            auto coeff_mask_l = main_l > lag_l;
            auto coeff_mask_r = main_r > lag_r;
            auto coeff_l = qwqdsp_simd_element::PackOps::Select(coeff_mask_l, qwqdsp_simd_element::PackFloat<4>::vBroadcast(attack_), qwqdsp_simd_element::PackFloat<4>::vBroadcast(release_));
            auto coeff_r = qwqdsp_simd_element::PackOps::Select(coeff_mask_r, qwqdsp_simd_element::PackFloat<4>::vBroadcast(attack_), qwqdsp_simd_element::PackFloat<4>::vBroadcast(release_));
            lag_l *= coeff_l;
            lag_r *= coeff_r;
            lag_l += (1.0f - coeff_l) * main_l;
            lag_r += (1.0f - coeff_r) * main_r;
            main_peaks_[filter_idx][0] = lag_l;
            main_peaks_[filter_idx][1] = lag_r;
            // output
            float l = qwqdsp_simd_element::PackOps::ReduceAdd(lag_l * side_l);
            float r = qwqdsp_simd_element::PackOps::ReduceAdd(lag_r * side_r);
            output_[sample_idx][0] += l;
            output_[sample_idx][1] += r;
        }
    }
    for (size_t i = 0; i < num_samples; ++i) {
        main[i] = output_[i] * gain_;
    }
}

}
