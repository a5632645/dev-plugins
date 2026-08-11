#include "block_burg_lpc.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <span>

#include <qwqdsp/convert.hpp>
#include <qwqdsp/misc/smoother.hpp>

namespace green_vocoder::dsp {

using global::kMaxPoles;
using global::kRidge;

using PackFloat2 = qwqdsp_simd_element::PackFloat<2>;

void BlockBurgLPC::Init(float fs) {
    sample_rate_ = fs;
    SetParam(Params{.block_size = 1024});
}

void BlockBurgLPC::SetParam(const Params& p) {
    // 仅当 block_size 改变时才重建内部缓冲（分配开销大）
    if (p.block_size != fft_size_) {
        fft_size_ = p.block_size;
        eb_.resize(static_cast<size_t>(p.block_size));
        ef_.resize(static_cast<size_t>(p.block_size));
    }

    update_rate_ = sample_rate_ / (static_cast<float>(fft_size_) / 4.0f);

    num_poles_ = p.poles;

    smear_factor_ = qwqdsp_misc::ExpSmoother::ComputeSmoothFactor(std::max(p.smear, 10.0f), update_rate_);
    attack_factor_ = qwqdsp_misc::ExpSmoother::ComputeSmoothFactor(p.attack, update_rate_);

    fir_allpass_coeff_ = std::clamp(-p.formant_shift, -0.99f, 0.99f);
}

std::span<PackFloat2 const> BlockBurgLPC::operator()(std::span<PackFloat2 const> main,
                                                     std::span<PackFloat2 const> side) {
    // 岭回归常数：防止 down≈0 时反射系数 k = -2*up/(down+λ) 病态
    PackFloat2 ridge{};
    ridge.Broadcast(kRidge);

    // forward fir lattice
    std::copy(main.begin(), main.end(), ef_.begin());
    std::copy(main.begin(), main.end(), eb_.begin());
    std::array<PackFloat2, kMaxPoles> latticek{};
    for (int kidx = 0; kidx < num_poles_; ++kidx) {
        auto& k = latticek[static_cast<size_t>(kidx)];

        PackFloat2 up{};
        PackFloat2 down{};
        PackFloat2 s_iir{};
        for (int i = 0; i < static_cast<int>(ef_.size()); ++i) {
            auto y = fir_allpass_coeff_ * eb_[static_cast<size_t>(i)] + s_iir;
            s_iir = eb_[static_cast<size_t>(i)] - fir_allpass_coeff_ * y;
            eb_[static_cast<size_t>(i)] = y;
            up += ef_[static_cast<size_t>(i)] * y;
            down += ef_[static_cast<size_t>(i)] * ef_[static_cast<size_t>(i)];
            down += y * y;
        }
        k = -2.0f * up / (down + ridge);

        for (int i = 0; i < static_cast<int>(ef_.size()); ++i) {
            auto upgo = ef_[static_cast<size_t>(i)] + eb_[static_cast<size_t>(i)] * k;
            auto downgo = eb_[static_cast<size_t>(i)] + ef_[static_cast<size_t>(i)] * k;
            ef_[static_cast<size_t>(i)] = upgo;
            eb_[static_cast<size_t>(i)] = downgo;
        }
    }
    // smear
    // the FIR and IIR lattice coeffient are reversed
    auto reverse_fir_k = latticek.begin() + num_poles_;
    auto iir_k = latticek_.begin();
    for (int order = 0; order < num_poles_; order += 2) {
        auto const& fir_1 = *(--reverse_fir_k);
        auto const& fir_2 = *(--reverse_fir_k);
        auto& iir_1 = *(iir_k++);
        auto& iir_2 = *(iir_k++);
        iir_1 *= smear_factor_;
        iir_2 *= smear_factor_;
        iir_1 += fir_1 * (1.0f - smear_factor_);
        iir_2 += fir_2 * (1.0f - smear_factor_);
    }
    // eval gain
    PackFloat2 gain{};
    for (int i = 0; i < static_cast<int>(ef_.size()); ++i) {
        gain += ef_[static_cast<size_t>(i)] * ef_[static_cast<size_t>(i)];
    }
    gain = qwqdsp_simd_element::PackOps::Sqrt(gain);
    PackFloat2 gain_side{};
    gain_side.Broadcast(std::sqrt(static_cast<float>(fft_size_)));
    auto atten = gain / (gain_side);
    // atten smooth
    gain_lag_ *= attack_factor_;
    gain_lag_ += (1.0f - attack_factor_) * atten;
    // iir lattice
    std::array<PackFloat2, kMaxPoles + 1> l_iir{};
    for (int j = 0; j < static_cast<int>(ef_.size()); ++j) {
        auto x0 = side[static_cast<size_t>(j)] * gain_lag_;
        for (int idx = 0; idx < num_poles_; idx += 2) {
            auto x1 = x0 - latticek_[static_cast<size_t>(idx)] * l_iir[static_cast<size_t>(idx) + 1];
            auto x2 = x1 - latticek_[static_cast<size_t>(idx) + 1] * l_iir[static_cast<size_t>(idx) + 2];
            auto l0 = l_iir[static_cast<size_t>(idx) + 1] + latticek_[static_cast<size_t>(idx)] * x1;
            auto l1 = l_iir[static_cast<size_t>(idx) + 2] + latticek_[static_cast<size_t>(idx) + 1] * x2;
            x0 = x2;
            l_iir[static_cast<size_t>(idx)] = l0;
            l_iir[static_cast<size_t>(idx) + 1] = l1;
        }
        l_iir[static_cast<size_t>(num_poles_)] = x0;
        ef_[static_cast<size_t>(j)] = x0;
    }
    return std::span<PackFloat2 const>{ef_};
}

void BlockBurgLPC::CopyLatticeCoeffient(std::span<float> buffer, int order) {
    auto const backup = latticek_;
    auto reverse_iir_it = backup.begin() + order;
    for (int i = 0; i < order; ++i) {
        buffer[static_cast<size_t>(i)] = (*(--reverse_iir_it))[0];
    }
}

} // namespace green_vocoder::dsp
