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

void ChannelVocoder::SetFilterBankMode(ChannelVocoder::FilterBankMode mode) {
    filter_bank_mode_ = mode;
    for (auto& filter : main_filters_) { filter.Reset(); }
    for (auto& filter :side_filters_) { filter.Reset(); }
    UpdateFilters();
}

void ChannelVocoder::SetGate(float db) {
    gate_peak_ = qwqdsp::convert::Db2Gain(db);
}

// -------------------- frequency maps --------------------
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

// -------------------- filter designs --------------------
struct StackButterworth12 {
    static void Design(
        CascadeBPSVF& svf,
        qwqdsp_simd_element::PackFloatCRef<4> w1,
        qwqdsp_simd_element::PackFloatCRef<4> w2
    ) noexcept {
        auto f1 = qwqdsp_simd_element::PackOps::Tan(w1 / 2);
        auto f2 = qwqdsp_simd_element::PackOps::Tan(w2 / 2);
        auto f0 = qwqdsp_simd_element::PackOps::Sqrt(f1 * f2);
        // this Q only works for a order4 bandpass to create -6dB gain
        auto Q = f0 / qwqdsp_simd_element::PackOps::Abs(f2 - f1);
        svf.svf_[0].MakeBandpass<false>(f0, Q);
        svf.svf_[1].MakeBandpass<false>(f0, Q);
    }
};

struct StackButterworth24 {
    static void Design(
        CascadeBPSVF& svf,
        qwqdsp_simd_element::PackFloatCRef<4> w1,
        qwqdsp_simd_element::PackFloatCRef<4> w2
    ) noexcept {
        auto f1 = qwqdsp_simd_element::PackOps::Tan(w1 / 2);
        auto f2 = qwqdsp_simd_element::PackOps::Tan(w2 / 2);
        auto f0 = qwqdsp_simd_element::PackOps::Sqrt(f1 * f2);
        // power of a order2 bandpass
        [[maybe_unused]] constexpr auto power = 0.5f;
        // sqrt it then we cascade 4 will make it at -6dB at gain response
        // using fomula of a power function of a normalized bandpass
        // Notice!: w and Q are all analog variable, w=w/wc
        //
        //                           w^2/Q^2                w^2
        // power(w) = |H(s)|^2 = ------------------ = -------------------
        //                       1-2w^2+w^2/Q^2+w^4    Q^2(w^2-1)^2+w^2
        //
        // to let f1(digital is w1)'s power match the power we want
        // take w = f1/f0 and solve Q
        constexpr auto half_power = std::numbers::sqrt2_v<float> / 2;
        auto w_pow_2 = qwqdsp_simd_element::PackOps::X2(f1 / f0);
        auto Q = qwqdsp_simd_element::PackOps::Sqrt(w_pow_2 / half_power - w_pow_2) / qwqdsp_simd_element::PackOps::Abs(w_pow_2 - 1.0f);
        svf.svf_[0].MakeBandpass<false>(f0, Q);
        svf.svf_[1].MakeBandpass<false>(f0, Q);
        svf.svf_[2].MakeBandpass<false>(f0, Q);
        svf.svf_[3].MakeBandpass<false>(f0, Q);
    }
};

struct FlatButterworth12 {
    static void Design(
        CascadeBPSVF& svf,
        qwqdsp_simd_element::PackFloatCRef<4> w1,
        qwqdsp_simd_element::PackFloatCRef<4> w2
    ) noexcept {
        auto w1_analog = qwqdsp_simd_element::PackOps::Tan(w1 / 2);
        auto w2_analog = qwqdsp_simd_element::PackOps::Tan(w2 / 2);
        // prototype is a 2pole butterworth
        qwqdsp_simd_element::PackFloat<4> w_analog1;
        qwqdsp_simd_element::PackFloat<4> w_analog2;
        qwqdsp_simd_element::PackFloat<4> Q1;
        qwqdsp_simd_element::PackFloat<4> Q2;
        for (size_t i = 0; i < 4; ++i) {
            std::array<qwqdsp_filter::IIRDesign::ZPK, 2> zpk_buffer;
            qwqdsp_filter::IIRDesignExtra::ButterworthAttenGain(zpk_buffer, 1, 0.5f);
            qwqdsp_filter::IIRDesign::ProtyleToBandpass2(zpk_buffer, 1, w1_analog[i], w2_analog[i]);
            auto analog_w_bp1 = std::abs(zpk_buffer[0].p);
            auto analog_w_bp2 = std::abs(zpk_buffer[1].p);
            auto analog_Q_bp1 = -analog_w_bp1 / (2 * std::real(zpk_buffer[0].p));
            auto analog_Q_bp2 = -analog_w_bp1 / (2 * std::real(zpk_buffer[1].p));
            w_analog1[i] = static_cast<float>(analog_w_bp1);
            w_analog2[i] = static_cast<float>(analog_w_bp2);
            Q1[i] = static_cast<float>(analog_Q_bp1);
            Q2[i] = static_cast<float>(analog_Q_bp2);
        }
        svf.svf_[0].MakeBandpass<false>(w_analog1, qwqdsp_simd_element::PackOps::Abs(Q1));
        svf.svf_[1].MakeBandpass<false>(w_analog2, qwqdsp_simd_element::PackOps::Abs(Q2));
    }
};

struct FlatButterworth24 {
    static void Design(
        CascadeBPSVF& svf,
        qwqdsp_simd_element::PackFloatCRef<4> w1,
        qwqdsp_simd_element::PackFloatCRef<4> w2
    ) noexcept {
        auto w1_analog = qwqdsp_simd_element::PackOps::Tan(w1 / 2);
        auto w2_analog = qwqdsp_simd_element::PackOps::Tan(w2 / 2);
        // prototype is a 4pole butterworth
        qwqdsp_simd_element::PackFloat<4> w_analog1;
        qwqdsp_simd_element::PackFloat<4> w_analog2;
        qwqdsp_simd_element::PackFloat<4> w_analog3;
        qwqdsp_simd_element::PackFloat<4> w_analog4;
        qwqdsp_simd_element::PackFloat<4> Q1;
        qwqdsp_simd_element::PackFloat<4> Q2;
        qwqdsp_simd_element::PackFloat<4> Q3;
        qwqdsp_simd_element::PackFloat<4> Q4;
        for (size_t i = 0; i < 4; ++i) {
            std::array<qwqdsp_filter::IIRDesign::ZPK, 4> zpk_buffer;
            qwqdsp_filter::IIRDesignExtra::ButterworthAttenGain(zpk_buffer, 2, 0.5f);
            qwqdsp_filter::IIRDesign::ProtyleToBandpass2(zpk_buffer, 2, w1_analog[i], w2_analog[i]);
            auto analog_w_bp1 = std::abs(zpk_buffer[0].p);
            auto analog_w_bp2 = std::abs(zpk_buffer[1].p);
            auto analog_w_bp3 = std::abs(zpk_buffer[2].p);
            auto analog_w_bp4 = std::abs(zpk_buffer[3].p);
            auto analog_Q_bp1 = -analog_w_bp1 / (2 * std::real(zpk_buffer[0].p));
            auto analog_Q_bp2 = -analog_w_bp1 / (2 * std::real(zpk_buffer[1].p));
            auto analog_Q_bp3 = -analog_w_bp1 / (2 * std::real(zpk_buffer[2].p));
            auto analog_Q_bp4 = -analog_w_bp1 / (2 * std::real(zpk_buffer[3].p));
            w_analog1[i] = static_cast<float>(analog_w_bp1);
            w_analog2[i] = static_cast<float>(analog_w_bp2);
            w_analog3[i] = static_cast<float>(analog_w_bp3);
            w_analog4[i] = static_cast<float>(analog_w_bp4);
            Q1[i] = static_cast<float>(analog_Q_bp1);
            Q2[i] = static_cast<float>(analog_Q_bp2);
            Q3[i] = static_cast<float>(analog_Q_bp3);
            Q4[i] = static_cast<float>(analog_Q_bp4);
        }
        svf.svf_[0].MakeBandpass<false>(w_analog1, qwqdsp_simd_element::PackOps::Abs(Q1));
        svf.svf_[1].MakeBandpass<false>(w_analog2, qwqdsp_simd_element::PackOps::Abs(Q2));
        svf.svf_[2].MakeBandpass<false>(w_analog3, qwqdsp_simd_element::PackOps::Abs(Q3));
        svf.svf_[3].MakeBandpass<false>(w_analog4, qwqdsp_simd_element::PackOps::Abs(Q4));
    }
};

struct Chebyshev12 {
    static void Design(
        CascadeBPSVF& svf,
        qwqdsp_simd_element::PackFloatCRef<4> w1,
        qwqdsp_simd_element::PackFloatCRef<4> w2
    ) noexcept {
        auto w1_analog = qwqdsp_simd_element::PackOps::Tan(w1 / 2);
        auto w2_analog = qwqdsp_simd_element::PackOps::Tan(w2 / 2);
        qwqdsp_simd_element::PackFloat<4> w_analog1;
        qwqdsp_simd_element::PackFloat<4> w_analog2;
        qwqdsp_simd_element::PackFloat<4> Q1;
        qwqdsp_simd_element::PackFloat<4> Q2;
        for (size_t i = 0; i < 4; ++i) {
            std::array<qwqdsp_filter::IIRDesign::ZPK, 2> zpk_buffer;
            qwqdsp_filter::IIRDesign::Chebyshev1(zpk_buffer, 1, 6.02059991f, true);
            qwqdsp_filter::IIRDesign::ProtyleToBandpass2(zpk_buffer, 1, w1_analog[i], w2_analog[i]);
            auto analog_w_bp1 = std::abs(zpk_buffer[0].p);
            auto analog_w_bp2 = std::abs(zpk_buffer[1].p);
            auto analog_Q_bp1 = -analog_w_bp1 / (2 * std::real(zpk_buffer[0].p));
            auto analog_Q_bp2 = -analog_w_bp1 / (2 * std::real(zpk_buffer[1].p));
            w_analog1[i] = static_cast<float>(analog_w_bp1);
            w_analog2[i] = static_cast<float>(analog_w_bp2);
            Q1[i] = static_cast<float>(analog_Q_bp1);
            Q2[i] = static_cast<float>(analog_Q_bp2);
        }
        svf.svf_[0].MakeBandpass<false>(w_analog1, qwqdsp_simd_element::PackOps::Abs(Q1));
        svf.svf_[1].MakeBandpass<false>(w_analog2, qwqdsp_simd_element::PackOps::Abs(Q2));
    }
};

struct Chebyshev24 {
    static void Design(
        CascadeBPSVF& svf,
        qwqdsp_simd_element::PackFloatCRef<4> w1,
        qwqdsp_simd_element::PackFloatCRef<4> w2
    ) noexcept {
        auto w1_analog = qwqdsp_simd_element::PackOps::Tan(w1 / 2);
        auto w2_analog = qwqdsp_simd_element::PackOps::Tan(w2 / 2);
        qwqdsp_simd_element::PackFloat<4> w_analog1;
        qwqdsp_simd_element::PackFloat<4> w_analog2;
        qwqdsp_simd_element::PackFloat<4> w_analog3;
        qwqdsp_simd_element::PackFloat<4> w_analog4;
        qwqdsp_simd_element::PackFloat<4> Q1;
        qwqdsp_simd_element::PackFloat<4> Q2;
        qwqdsp_simd_element::PackFloat<4> Q3;
        qwqdsp_simd_element::PackFloat<4> Q4;
        for (size_t i = 0; i < 4; ++i) {
            std::array<qwqdsp_filter::IIRDesign::ZPK, 4> zpk_buffer;
            qwqdsp_filter::IIRDesign::Chebyshev1(zpk_buffer, 2, 6.02059991f, true);
            qwqdsp_filter::IIRDesign::ProtyleToBandpass2(zpk_buffer, 2, w1_analog[i], w2_analog[i]);
            auto analog_w_bp1 = std::abs(zpk_buffer[0].p);
            auto analog_w_bp2 = std::abs(zpk_buffer[1].p);
            auto analog_w_bp3 = std::abs(zpk_buffer[2].p);
            auto analog_w_bp4 = std::abs(zpk_buffer[3].p);
            auto analog_Q_bp1 = -analog_w_bp1 / (2 * std::real(zpk_buffer[0].p));
            auto analog_Q_bp2 = -analog_w_bp1 / (2 * std::real(zpk_buffer[1].p));
            auto analog_Q_bp3 = -analog_w_bp1 / (2 * std::real(zpk_buffer[2].p));
            auto analog_Q_bp4 = -analog_w_bp1 / (2 * std::real(zpk_buffer[3].p));
            w_analog1[i] = static_cast<float>(analog_w_bp1);
            w_analog2[i] = static_cast<float>(analog_w_bp2);
            w_analog3[i] = static_cast<float>(analog_w_bp3);
            w_analog4[i] = static_cast<float>(analog_w_bp4);
            Q1[i] = static_cast<float>(analog_Q_bp1);
            Q2[i] = static_cast<float>(analog_Q_bp2);
            Q3[i] = static_cast<float>(analog_Q_bp3);
            Q4[i] = static_cast<float>(analog_Q_bp4);
        }
        svf.svf_[0].MakeBandpass<false>(w_analog1, qwqdsp_simd_element::PackOps::Abs(Q1));
        svf.svf_[1].MakeBandpass<false>(w_analog2, qwqdsp_simd_element::PackOps::Abs(Q2));
        svf.svf_[2].MakeBandpass<false>(w_analog3, qwqdsp_simd_element::PackOps::Abs(Q3));
        svf.svf_[3].MakeBandpass<false>(w_analog4, qwqdsp_simd_element::PackOps::Abs(Q4));
    }
};

template<class AssignMap>
void ChannelVocoder::_UpdateFilters() {
    gain_ = 0.5;
    switch (filter_bank_mode_) {
        case FilterBankMode::StackButterworth12:
            _UpdateFilters2<AssignMap, StackButterworth12>();
            gain_ *= qwqdsp::convert::Db2Gain(20.0f);
            break;
        case FilterBankMode::StackButterworth24:
            _UpdateFilters2<AssignMap, StackButterworth24>();
            gain_ *= qwqdsp::convert::Db2Gain(19.0f);
            break;
        case FilterBankMode::FlatButterworth12:
            _UpdateFilters2<AssignMap, FlatButterworth12>();
            gain_ *= qwqdsp::convert::Db2Gain(25.0f);
        break;
            case FilterBankMode::FlatButterworth24:
            _UpdateFilters2<AssignMap, FlatButterworth24>();
            gain_ *= qwqdsp::convert::Db2Gain(33.0f);
            break;
        case FilterBankMode::Chebyshev12:
            _UpdateFilters2<AssignMap, Chebyshev12>();
            gain_ *= qwqdsp::convert::Db2Gain(22.5f);
            break;
        case FilterBankMode::Chebyshev24:
            _UpdateFilters2<AssignMap, Chebyshev24>();
            gain_ *= qwqdsp::convert::Db2Gain(71.5f);
            break;
    }
}

template<class AssignMap, class Designer>
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
        auto& main_filter = main_filters_[filter_idx];
        auto& side_filter = side_filters_[filter_idx];
        Designer::Design(main_filter, main_w1, main_w2);
        Designer::Design(side_filter, side_w1, side_w2);
        ++filter_idx;
    }
}

void ChannelVocoder::ProcessBlock(
    qwqdsp_simd_element::PackFloat<2>* main,
    qwqdsp_simd_element::PackFloat<2>* side,
    size_t num_samples
) {
    switch (filter_bank_mode_) {
        case FilterBankMode::StackButterworth12:
        case FilterBankMode::FlatButterworth12:
        case FilterBankMode::Chebyshev12:
            _ProcessBlock<2>(main, side, num_samples);
            break;
        case FilterBankMode::StackButterworth24:
        case FilterBankMode::FlatButterworth24:
        case FilterBankMode::Chebyshev24:
            _ProcessBlock<4>(main, side, num_samples);
            break;
    }
}

template<size_t kFilterNumbers>
void ChannelVocoder::_ProcessBlock(
    qwqdsp_simd_element::PackFloat<2>* main,
    qwqdsp_simd_element::PackFloat<2>* side,
    size_t num_samples
) {
    std::fill_n(output_.begin(), num_samples, qwqdsp_simd_element::PackFloat<2>{});
    auto vgate_peak = qwqdsp_simd_element::PackFloat<4>::vBroadcast(gate_peak_);
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
            main_l *= gain_;
            main_r *= gain_;
            side_l *= gain_;
            side_r *= gain_;
            main_filters_[filter_idx].Tick<kFilterNumbers, true>(main_l, main_r);
            side_filters_[filter_idx].Tick<kFilterNumbers, false>(side_l, side_r);
            // envelope follower
            auto curr = main_peaks_[filter_idx];
            main_l = qwqdsp_simd_element::PackOps::Abs(main_l);
            main_r = qwqdsp_simd_element::PackOps::Abs(main_r);
            main_l = qwqdsp_simd_element::PackOps::Select(main_l > vgate_peak, main_l, qwqdsp_simd_element::PackFloat<4>::vBroadcast(0.0f));
            main_r = qwqdsp_simd_element::PackOps::Select(main_r > vgate_peak, main_r, qwqdsp_simd_element::PackFloat<4>::vBroadcast(0.0f));
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
    std::copy_n(output_.begin(), num_samples, main);
}

}
