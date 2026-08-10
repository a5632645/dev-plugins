#include "block_burg_lpc.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <span>

#include <qwqdsp/convert.hpp>
#include <qwqdsp/misc/smoother.hpp>

namespace green_vocoder::dsp {

using global::kMaxPoles;
using global::kNoiseGain;

using PackFloat2 = qwqdsp_simd_element::PackFloat<2>;

void BlockBurgLPC::Init(float fs) {
    sample_rate_ = fs;
    SetParam(Params{.block_size = 1024});
}

void BlockBurgLPC::SetParam(const Params& p) {
    // block size（含窗口与内部缓冲重建）
    fft_size_ = p.block_size;
    size_t const hop_size = p.block_size / 4;
    eb_.resize(p.block_size);
    ef_.resize(p.block_size);

    // hann 合成窗
    std::vector<float> hann_window(p.block_size);
    for (size_t i = 0; i < p.block_size; ++i) {
        hann_window[i] =
            0.5f - 0.5f * std::cos(2.0f * std::numbers::pi_v<float> * static_cast<float>(i) / static_cast<float>(p.block_size));
    }
    ola_.Init(p.block_size, hop_size, hann_window);
    ola_.SetOutputGain(0.25f);

    update_rate_ = sample_rate_ / static_cast<float>(hop_size);

    num_poles_ = p.poles;

    smear_factor_ = qwqdsp_misc::ExpSmoother::ComputeSmoothFactor(std::max(p.smear, 10.0f), update_rate_);
    attack_factor_ = qwqdsp_misc::ExpSmoother::ComputeSmoothFactor(p.attack, update_rate_);

    fir_allpass_coeff_ = std::clamp(-p.formant_shift, -0.99f, 0.99f);
}

void BlockBurgLPC::Process(qwqdsp_simd_element::PackFloat<2>* main_ptr, qwqdsp_simd_element::PackFloat<2>* side_ptr,
                           size_t num_samples) {
    // adding some noise
    for (size_t i = 0; i < num_samples; ++i) {
        main_ptr[i] += noise_.Next() * kNoiseGain;
    }
    ola_.Process(main_ptr, side_ptr, num_samples, *this);
}

std::span<PackFloat2 const> BlockBurgLPC::operator()(std::span<PackFloat2 const> main,
                                                     std::span<PackFloat2 const> side) {
    // forward fir lattice
    std::copy(main.begin(), main.end(), ef_.begin());
    std::copy(main.begin(), main.end(), eb_.begin());
    std::array<PackFloat2, kMaxPoles> latticek{};
    for (size_t kidx = 0; kidx < num_poles_; ++kidx) {
        auto& k = latticek[kidx];

        PackFloat2 up{};
        PackFloat2 down{};
        PackFloat2 s_iir{};
        for (size_t i = 0; i < ef_.size(); ++i) {
            auto y = fir_allpass_coeff_ * eb_[i] + s_iir;
            s_iir = eb_[i] - fir_allpass_coeff_ * y;
            eb_[i] = y;
            up += ef_[i] * y;
            down += ef_[i] * ef_[i];
            down += y * y;
        }
        k = -2.0f * up / down;

        for (size_t i = 0; i < ef_.size(); ++i) {
            auto upgo = ef_[i] + eb_[i] * k;
            auto downgo = eb_[i] + ef_[i] * k;
            ef_[i] = upgo;
            eb_[i] = downgo;
        }
    }
    // smear
    // the FIR and IIR lattice coeffient are reversed
    auto reverse_fir_k = latticek.begin() + static_cast<int>(num_poles_);
    auto iir_k = latticek_.begin();
    for (size_t order = 0; order < num_poles_; order += 2) {
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
    for (size_t i = 0; i < ef_.size(); ++i) {
        gain += ef_[i] * ef_[i];
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
    for (size_t j = 0; j < ef_.size(); ++j) {
        auto x0 = side[j] * gain_lag_;
        for (size_t idx = 0; idx < num_poles_; idx += 2) {
            auto x1 = x0 - latticek_[idx] * l_iir[idx + 1];
            auto x2 = x1 - latticek_[idx + 1] * l_iir[idx + 2];
            auto l0 = l_iir[idx + 1] + latticek_[idx] * x1;
            auto l1 = l_iir[idx + 2] + latticek_[idx + 1] * x2;
            x0 = x2;
            l_iir[idx] = l0;
            l_iir[idx + 1] = l1;
        }
        l_iir[num_poles_] = x0;
        ef_[j] = x0;
    }
    return std::span<PackFloat2 const>{ef_};
}

void BlockBurgLPC::CopyLatticeCoeffient(std::span<float> buffer, size_t order) {
    auto const backup = latticek_;
    auto reverse_iir_it = backup.begin() + static_cast<int>(order);
    for (size_t i = 0; i < order; ++i) {
        buffer[i] = (*(--reverse_iir_it))[0];
    }
}

} // namespace green_vocoder::dsp
