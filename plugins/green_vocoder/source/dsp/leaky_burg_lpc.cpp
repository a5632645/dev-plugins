#include "leaky_burg_lpc.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <qwqdsp/filter/rbj.hpp>

namespace green_vocoder::dsp {
void LeakyBurgLPC::Init(float sample_rate, size_t block_size) {
    sample_rate_ = sample_rate;
    true_sample_rate_ = sample_rate;
}

void LeakyBurgLPC::Process(
    std::span<qwqdsp_simd_element::PackFloat<2>> main,
    std::span<qwqdsp_simd_element::PackFloat<2>> side
) {
    switch (quality_) {
        case Quality::Modern:
            ProcessWithDicimate<kDicimateTable[0]>(main, side);
            break;
        case Quality::Legacy:
            ProcessWithDicimate<kDicimateTable[1]>(main, side);
            break;
        case Quality::Telephone:
            ProcessWithDicimate<kDicimateTable[2]>(main, side);
            break;
    }
}

template<size_t kDicimate>
void LeakyBurgLPC::ProcessWithDicimate(
    std::span<qwqdsp_simd_element::PackFloat<2>> main,
    std::span<qwqdsp_simd_element::PackFloat<2>> side
) {
    if constexpr (kDicimate == 1) {
        for (size_t i = 0; i < main.size(); ++i) {
            // adding some noise to prevent ill filter coefficient
            float small_noise = noise_.Next() * kNoiseGain;
            qwqdsp_simd_element::PackFloat<2> main_x = main[i];
            main_x += small_noise;
            // forward fir lattice
            qwqdsp_simd_element::PackFloat<2> ef = main_x;
            qwqdsp_simd_element::PackFloat<2> eb = main_x;
            for (int order = 0; order < lpc_order_; ++order) {
                size_t order_idx = static_cast<size_t>(order);

                auto y = fir_allpass_coeff_ * eb + fir_allpass_s_[order_idx];
                fir_allpass_s_[order_idx] = eb - fir_allpass_coeff_ * y;

                efsum_[order_idx] *= forget_;
                ebsum_[order_idx] *= forget_;
                efsum_[order_idx] += ef * y;
                ebsum_[order_idx] += ef * ef;
                ebsum_[order_idx] += y * y;
                lattice_k_[order_idx] = -2.0f * efsum_[order_idx] / (ebsum_[order_idx]);
                auto const k = lattice_k_[order_idx];
                auto const upgo = ef + k * y;
                auto const downgo = y + k * ef;
                ef = upgo;
                eb = downgo;
            }
            // wired lattice coeffient smooth
            // the FIR and IIR lattice coeffient are reversed
            auto reverse_fir_k = lattice_k_.begin() + lpc_order_;
            auto iir_k = iir_k_.begin();
            for (int order = 0; order < lpc_order_; order += 2) {
                auto const& fir_1 = *(--reverse_fir_k);
                auto const& fir_2 = *(--reverse_fir_k);
                auto& iir_1 = *(iir_k++);
                auto& iir_2 = *(iir_k++);
                iir_1 *= smooth_;
                iir_2 *= smooth_;
                iir_1 += fir_1 * (1.0f - smooth_);
                iir_2 += fir_2 * (1.0f - smooth_);
            }
            // iir lattice
            auto const& residual = ef;
            auto& s_iir = s_iir_.front();
            auto gain = gain_smooth_.Tick(qwqdsp_simd_element::PackOps::Abs(residual));
            auto x0 = side[i] * gain;
            size_t const size_lpc_order = static_cast<size_t>(lpc_order_);
            for (size_t idx = 0; idx < size_lpc_order; idx += 2) {
                auto x1 = x0 - iir_k_[idx] * s_iir[idx + 1];
                auto x2 = x1 - iir_k_[idx + 1] * s_iir[idx + 2];
                auto l0 = s_iir[idx + 1] + iir_k_[idx] * x1;
                auto l1 = s_iir[idx + 2] + iir_k_[idx + 1] * x2;
                x0 = x2;
                s_iir[idx] = l0;
                s_iir[idx + 1] = l1;
            }
            s_iir[size_lpc_order] = x0;
            main[i] = x0;
        }
    }
    else {
        for (size_t i = 0; i < main.size(); ++i) {
            qwqdsp_simd_element::PackFloat<4> x{
                main[i][0], main[i][1]
            };
            dicimate_filter_.TickMultiChannel(x);
            main[i][0] = x[0];
            main[i][1] = x[1];
            // FIR lattice working in dicimate sample rate
            ++dicimate_counter_;
            if (dicimate_counter_ > kDicimate) {
                dicimate_counter_ = 0;
                float small_noise = noise_.Next() * kNoiseGain;
                qwqdsp_simd_element::PackFloat<2> main_x = main[i];
                main_x += small_noise;
                // forward fir lattice
                qwqdsp_simd_element::PackFloat<2> ef = main_x;
                qwqdsp_simd_element::PackFloat<2> eb = main_x;
                for (int order = 0; order < lpc_order_; ++order) {
                    size_t order_idx = static_cast<size_t>(order);

                    auto y = fir_allpass_coeff_ * eb + fir_allpass_s_[order_idx];
                    fir_allpass_s_[order_idx] = eb - fir_allpass_coeff_ * y;

                    efsum_[order_idx] *= forget_;
                    ebsum_[order_idx] *= forget_;
                    efsum_[order_idx] += ef * y;
                    ebsum_[order_idx] += ef * ef;
                    ebsum_[order_idx] += y * y;
                    lattice_k_[order_idx] = -2.0f * efsum_[order_idx] / (ebsum_[order_idx]);
                    auto const k = lattice_k_[order_idx];
                    auto const upgo = ef + k * y;
                    auto const downgo = y + k * ef;
                    ef = upgo;
                    eb = downgo;
                }
                auto const& residual = ef;
                residual_gain_ = qwqdsp_simd_element::PackOps::Abs(residual);
            }
            // IIR lattice working in normal sample rate
            // wired lattice coeffient smooth
            // the FIR and IIR lattice coeffient are reversed
            auto reverse_fir_k = lattice_k_.begin() + lpc_order_;
            auto iir_k = iir_k_.begin();
            for (int order = 0; order < lpc_order_; order += 2) {
                auto const& fir_1 = *(--reverse_fir_k);
                auto const& fir_2 = *(--reverse_fir_k);
                auto& iir_1 = *(iir_k++);
                auto& iir_2 = *(iir_k++);
                iir_1 *= smooth_;
                iir_2 *= smooth_;
                iir_1 += fir_1 * (1.0f - smooth_);
                iir_2 += fir_2 * (1.0f - smooth_);
            }
            // iir lattice
            auto gain = gain_smooth_.Tick(qwqdsp_simd_element::PackOps::Abs(residual_gain_));
            auto& s_iir_write = s_iir_[iir_s_wpos_++];
            auto& s_iir_read = s_iir_[iir_s_rpos_++];
            iir_s_wpos_ &= (2 * kDicimate - 1);
            iir_s_rpos_ &= (2 * kDicimate - 1);
            auto x0 = side[i] * gain;
            size_t const size_lpc_order = static_cast<size_t>(lpc_order_);
            for (size_t idx = 0; idx < size_lpc_order; idx += 2) {
                auto x1 = x0 - iir_k_[idx] * s_iir_read[idx + 1];
                auto x2 = x1 - iir_k_[idx + 1] * s_iir_read[idx + 2];
                auto l0 = s_iir_read[idx + 1] + iir_k_[idx] * x1;
                auto l1 = s_iir_read[idx + 2] + iir_k_[idx + 1] * x2;
                x0 = x2;
                s_iir_write[idx] = l0;
                s_iir_write[idx + 1] = l1;
            }
            s_iir_write[size_lpc_order] = x0;
            main[i] = x0;
        }
    }
}

void LeakyBurgLPC::SetSmooth(float smooth) {
    smooth_ms_ = smooth;
    smooth_ = std::exp(-1.0f / ((sample_rate_) * smooth / 1000.0f));
}

void LeakyBurgLPC::SetForget(float forget_ms) {
    forget_ms_ = forget_ms;
    forget_ = std::exp(-1.0f / ((true_sample_rate_) * forget_ms / 1000.0f));
}

void LeakyBurgLPC::SetLPCOrder(int order) {
    assert(order % 4 == 0);
    lpc_order_ = order;
    std::fill_n(lattice_k_.begin(), order, qwqdsp_simd_element::PackFloat<2>{});
    std::fill_n(iir_k_.begin(), order, qwqdsp_simd_element::PackFloat<2>{});
    std::fill_n(ebsum_.begin(), order, qwqdsp_simd_element::PackFloat<2>{});
    std::fill_n(efsum_.begin(), order, qwqdsp_simd_element::PackFloat<2>{});
    s_iir_.fill({});
}

void LeakyBurgLPC::SetGainAttack(float ms) {
    gain_attack_ = ms;
    gain_smooth_.SetAttackTime(ms, sample_rate_);
    gain_smooth_.SetReleaseTime((ms + gain_attack_), sample_rate_);
}

void LeakyBurgLPC::SetGainRelease(float ms) {
    gain_release_ = ms;
    gain_smooth_.SetReleaseTime((ms + gain_attack_), sample_rate_);
}

void LeakyBurgLPC::SetGainHold(float ms) {
    gain_smooth_.SetHoldTime(ms, sample_rate_);
}

void LeakyBurgLPC::CopyLatticeCoeffient(std::span<float> buffer, size_t order) {
    auto const backup = iir_k_;
    auto reverse_iir_it = backup.begin() + static_cast<int>(order);
    for (size_t i = 0; i < order; ++i) {
        buffer[i] = (*(--reverse_iir_it))[0];
    }
}

void LeakyBurgLPC::SetQuality(LeakyBurgLPC::Quality quality) {
    quality_ = quality;
    size_t dicimate = 1;
    switch (quality_) {
        case Quality::Legacy:
            dicimate = kDicimateTable[1];
            // iir_s_wpos_ = 3;
            break;
        case Quality::Telephone:
            dicimate = kDicimateTable[2];
            // iir_s_wpos_ = 9;
            break;
        case Quality::Modern:
            dicimate = kDicimateTable[0];
            // iir_s_wpos_ = 0;
            break;
    }
    // I don't know why iir lattice delay unit is (dicimate + 2) not (dicimate)
    // This is wried.
    iir_s_rpos_ = 0;
    iir_s_wpos_ = dicimate + 1;
    s_iir_.fill({});

    true_sample_rate_ = sample_rate_ / static_cast<float>(dicimate);
    dicimate_counter_ = dicimate;
    residual_gain_.Broadcast(0);
    SetForget(forget_ms_);

    qwqdsp_filter::RBJ designer;
    dicimate_filter_.SetAll(designer.Dicimate(dicimate));
}

void LeakyBurgLPC::SetFormantShift(float shift) {
    fir_allpass_coeff_ = std::clamp(-shift, -0.99f, 0.99f);
}

}
