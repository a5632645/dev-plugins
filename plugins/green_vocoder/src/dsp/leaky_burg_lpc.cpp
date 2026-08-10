#include "leaky_burg_lpc.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <qwqdsp/filter/rbj.hpp>

namespace green_vocoder::dsp {
void LeakyBurgLPC::Init(float sample_rate, [[maybe_unused]] int block_size) {
    sample_rate_ = sample_rate;
}

void LeakyBurgLPC::Process(std::span<qwqdsp_simd_element::PackFloat<2>> main,
                           std::span<qwqdsp_simd_element::PackFloat<2>> side) {
    for (int i = 0; i < static_cast<int>(main.size()); ++i) {
        // adding some noise to prevent ill filter coefficient
        float small_noise = noise_.Next() * global::kNoiseGain;
        qwqdsp_simd_element::PackFloat<2> main_x = main[static_cast<size_t>(i)];
        main_x += small_noise;
        // forward fir lattice
        qwqdsp_simd_element::PackFloat<2> ef = main_x;
        qwqdsp_simd_element::PackFloat<2> eb = main_x;
        for (int order = 0; order < lpc_order_; ++order) {
            int const order_idx = order;

            auto y = fir_allpass_coeff_ * eb + fir_allpass_s_[static_cast<size_t>(order_idx)];
            fir_allpass_s_[static_cast<size_t>(order_idx)] = eb - fir_allpass_coeff_ * y;

            efsum_[static_cast<size_t>(order_idx)] *= forget_;
            ebsum_[static_cast<size_t>(order_idx)] *= forget_;
            efsum_[static_cast<size_t>(order_idx)] += ef * y;
            ebsum_[static_cast<size_t>(order_idx)] += ef * ef;
            ebsum_[static_cast<size_t>(order_idx)] += y * y;
            lattice_k_[static_cast<size_t>(order_idx)] =
                -2.0f * efsum_[static_cast<size_t>(order_idx)] / (ebsum_[static_cast<size_t>(order_idx)]);
            auto const k = lattice_k_[static_cast<size_t>(order_idx)];
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
        auto gain = gain_smooth_.Tick(qwqdsp_simd_element::PackOps::Abs(residual));
        auto x0 = side[static_cast<size_t>(i)] * gain;
        int const lpc_order = lpc_order_;
        for (int idx = 0; idx < lpc_order; idx += 2) {
            auto x1 = x0 - iir_k_[static_cast<size_t>(idx)] * iir_s_[static_cast<size_t>(idx) + 1];
            auto x2 = x1 - iir_k_[static_cast<size_t>(idx) + 1] * iir_s_[static_cast<size_t>(idx) + 2];
            auto l0 = iir_s_[static_cast<size_t>(idx) + 1] + iir_k_[static_cast<size_t>(idx)] * x1;
            auto l1 = iir_s_[static_cast<size_t>(idx) + 2] + iir_k_[static_cast<size_t>(idx) + 1] * x2;
            x0 = x2;
            iir_s_[static_cast<size_t>(idx)] = l0;
            iir_s_[static_cast<size_t>(idx) + 1] = l1;
        }
        iir_s_[static_cast<size_t>(lpc_order)] = x0;
        main[static_cast<size_t>(i)] = x0 * 0.1f;
    }
}

void LeakyBurgLPC::SetParam(const Params& p) {
    assert(p.order % 4 == 0);
    int const order = p.order;
    lpc_order_ = p.order;
    std::fill_n(lattice_k_.begin(), order, qwqdsp_simd_element::PackFloat<2>{});
    std::fill_n(iir_k_.begin(), order, qwqdsp_simd_element::PackFloat<2>{});
    std::fill_n(ebsum_.begin(), order, qwqdsp_simd_element::PackFloat<2>{});
    std::fill_n(efsum_.begin(), order, qwqdsp_simd_element::PackFloat<2>{});
    iir_s_.fill({});

    forget_ms_ = p.forget;
    forget_ = qwqdsp_misc::ExpSmoother::ComputeSmoothFactor(p.forget, sample_rate_);

    smooth_ms_ = p.smooth;
    smooth_ = qwqdsp_misc::ExpSmoother::ComputeSmoothFactor(p.smooth, sample_rate_);

    gain_attack_ = p.gain_attack;
    gain_release_ = p.gain_release;
    gain_smooth_.SetAttackTime(p.gain_attack, sample_rate_);
    gain_smooth_.SetReleaseTime(p.gain_release + p.gain_attack, sample_rate_);
    gain_smooth_.SetHoldTime(p.gain_hold, sample_rate_);

    fir_allpass_coeff_ = std::clamp(-p.formant_shift, -0.99f, 0.99f);
}

void LeakyBurgLPC::CopyLatticeCoeffient(std::span<float> buffer, int order) {
    auto const backup = iir_k_;
    auto reverse_iir_it = backup.begin() + order;
    for (int i = 0; i < order; ++i) {
        buffer[static_cast<size_t>(i)] = (*(--reverse_iir_it))[0];
    }
}

} // namespace green_vocoder::dsp
