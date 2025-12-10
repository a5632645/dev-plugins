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
                efsum_[order_idx] *= forget_;
                ebsum_[order_idx] *= forget_;
                efsum_[order_idx] += ef * eb_lag_[order_idx];
                ebsum_[order_idx] += ef * ef;
                ebsum_[order_idx] += eb_lag_[order_idx] * eb_lag_[order_idx];
                lattice_k_[order_idx] = -2.0f * efsum_[order_idx] / (ebsum_[order_idx]);
                auto const k = lattice_k_[order_idx];
                auto const upgo = ef + k * eb_lag_[order_idx];
                auto const downgo = eb_lag_[order_idx] + k * ef;
                eb_lag_[order_idx] = eb;
                ef = upgo;
                eb = downgo;
            }
            // wired lattice coeffient smooth
            for (int order = 0; order < lpc_order_; ++order) {
                size_t order_idx = static_cast<size_t>(order);
                iir_k_[order_idx] *= smooth_;
                iir_k_[order_idx] += lattice_k_[order_idx] * (1.0f - smooth_);
            }
            // iir lattice
            qwqdsp_simd_element::PackFloat<2> residual = ef;
            auto gain = gain_smooth_.Process(qwqdsp_simd_element::PackOps::Abs(residual));
            x_iir_[0] = side[i] * gain;
            for (int idx = 0; idx < lpc_order_; ++idx) {
                x_iir_[static_cast<size_t>(idx + 1)] = x_iir_[static_cast<size_t>(idx)] - iir_k_[static_cast<size_t>(lpc_order_ - idx - 1)] * l_iir[static_cast<size_t>(idx + 1)];
            }
            for (int idx = 0; idx < lpc_order_; ++idx) {
                l_iir[static_cast<size_t>(idx)] = l_iir[static_cast<size_t>(idx + 1)] + iir_k_[static_cast<size_t>(lpc_order_ - idx - 1)] * x_iir_[static_cast<size_t>(idx + 1)];
            }
            l_iir[static_cast<size_t>(lpc_order_)] = x_iir_[static_cast<size_t>(lpc_order_)];
            main[i] = x_iir_[static_cast<size_t>(lpc_order_)];
        }
    }
    else {
        for (size_t i = 0; i < main.size(); ++i) {
            qwqdsp_simd_element::PackFloat<4> x{
                main[i][0], main[i][1], side[i][0], side[i][1]
            };
            dicimate_filter_.TickMultiChannel(x);
            main[i][0] = x[0];
            main[i][1] = x[1];
            side[i][0] = x[2];
            side[i][1] = x[3];

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
                    efsum_[order_idx] *= forget_;
                    ebsum_[order_idx] *= forget_;
                    efsum_[order_idx] += ef * eb_lag_[order_idx];
                    ebsum_[order_idx] += ef * ef;
                    ebsum_[order_idx] += eb_lag_[order_idx] * eb_lag_[order_idx];
                    lattice_k_[order_idx] = -2.0f * efsum_[order_idx] / (ebsum_[order_idx]);
                    auto const k = lattice_k_[order_idx];
                    auto const upgo = ef + k * eb_lag_[order_idx];
                    auto const downgo = eb_lag_[order_idx] + k * ef;
                    eb_lag_[order_idx] = eb;
                    ef = upgo;
                    eb = downgo;
                }
                // wired lattice coeffient smooth
                for (int order = 0; order < lpc_order_; ++order) {
                    size_t order_idx = static_cast<size_t>(order);
                    iir_k_[order_idx] *= smooth_;
                    iir_k_[order_idx] += lattice_k_[order_idx] * (1.0f - smooth_);
                }
                // iir lattice
                qwqdsp_simd_element::PackFloat<2> residual = ef;
                auto gain = gain_smooth_.Process(qwqdsp_simd_element::PackOps::Abs(residual));
                x_iir_[0] = side[i] * gain;
                for (int j = 0; j < lpc_order_; ++j) {
                    x_iir_[static_cast<size_t>(j + 1)] = x_iir_[static_cast<size_t>(j)] - iir_k_[static_cast<size_t>(lpc_order_ - j - 1)] * l_iir[static_cast<size_t>(j + 1)];
                }
                for (int j = 0; j < lpc_order_; ++j) {
                    l_iir[static_cast<size_t>(j)] = l_iir[static_cast<size_t>(j + 1)] + iir_k_[static_cast<size_t>(lpc_order_ - j - 1)] * x_iir_[static_cast<size_t>(j + 1)];
                }
                l_iir[static_cast<size_t>(lpc_order_)] = x_iir_[static_cast<size_t>(lpc_order_)];
                upsample_latch_ = x_iir_[static_cast<size_t>(lpc_order_)];
            }
            main[i] = upsample_latch_;
        }
    }
}

void LeakyBurgLPC::SetSmooth(float smooth) {
    smooth_ms_ = smooth;
    smooth_ = std::exp(-1.0f / ((true_sample_rate_) * smooth / 1000.0f));
}

void LeakyBurgLPC::SetForget(float forget_ms) {
    forget_ms_ = forget_ms;
    forget_ = std::exp(-1.0f / ((true_sample_rate_) * forget_ms / 1000.0f));
}

void LeakyBurgLPC::SetLPCOrder(int order) {
    lpc_order_ = order;
    std::fill_n(lattice_k_.begin(), order, qwqdsp_simd_element::PackFloat<2>{});
    std::fill_n(iir_k_.begin(), order, qwqdsp_simd_element::PackFloat<2>{});
    std::fill_n(ebsum_.begin(), order, qwqdsp_simd_element::PackFloat<2>{});
    std::fill_n(efsum_.begin(), order, qwqdsp_simd_element::PackFloat<2>{});
    std::fill_n(x_iir_.begin(), order + 1, qwqdsp_simd_element::PackFloat<2>{});
    std::fill_n(l_iir.begin(), order + 1, qwqdsp_simd_element::PackFloat<2>{});
}

void LeakyBurgLPC::SetGainAttack(float ms) {
    gain_attack_ = ms;
    gain_smooth_.SetAttackTime(ms, true_sample_rate_);
}

void LeakyBurgLPC::SetGainRelease(float ms) {
    gain_release_ = ms;
    gain_smooth_.SetReleaseTime(ms, true_sample_rate_);
}

void LeakyBurgLPC::CopyLatticeCoeffient(std::span<float> buffer) {
    auto backup = iir_k_;
    for (int i = 0; i < lpc_order_; ++i) {
        size_t idx = static_cast<size_t>(i);
        buffer[idx] = backup[idx][0];
    }
}

void LeakyBurgLPC::SetQuality(LeakyBurgLPC::Quality quality) {
    quality_ = quality;
    size_t dicimate = 1;
    switch (quality_) {
        case Quality::Legacy:
            dicimate = kDicimateTable[1];
            break;
        case Quality::Telephone:
            dicimate = kDicimateTable[2];
            break;
        case Quality::Modern:
            dicimate = kDicimateTable[0];
            break;
    }
    true_sample_rate_ = sample_rate_ / static_cast<float>(dicimate);
    dicimate_counter_ = dicimate;
    upsample_latch_.Broadcast(0);
    SetForget(forget_ms_);
    SetSmooth(smooth_ms_);
    gain_smooth_.SetAttackTime(gain_attack_, true_sample_rate_);
    gain_smooth_.SetReleaseTime(gain_release_, true_sample_rate_);

    qwqdsp_filter::RBJ designer;
    dicimate_filter_.SetAll(designer.Dicimate(dicimate));
}

}
