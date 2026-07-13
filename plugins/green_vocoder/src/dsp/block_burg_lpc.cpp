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

void BlockBurgLPC::Init(float fs) {
    sample_rate_ = fs;
    update_rate_ = fs;
    SetBlockSize(1024);
}

void BlockBurgLPC::SetBlockSize(size_t size) {
    fft_size_ = size;
    hann_window_.resize(size);
    eb_.resize(size);
    ef_.resize(size);
    hop_size_ = size / 4;
    for (size_t i = 0; i < size; ++i) {
        hann_window_[i] =
            0.5f - 0.5f * std::cos(2.0f * std::numbers::pi_v<float> * static_cast<float>(i) / static_cast<float>(size));
    }
    update_rate_ = sample_rate_ / static_cast<float>(hop_size_);
    SetAttack(attack_ms_);
    SetSmear(smear_ms_);
}

void BlockBurgLPC::SetPoles(size_t num_poles) {
    num_poles_ = num_poles;
}

void BlockBurgLPC::SetSmear(float ms) {
    ms = std::max(ms, 10.0f);
    smear_ms_ = ms;
    smear_factor_ = qwqdsp_misc::ExpSmoother::ComputeSmoothFactor(ms, update_rate_);
}

void BlockBurgLPC::SetAttack(float ms) {
    attack_ms_ = ms;
    attack_factor_ = qwqdsp_misc::ExpSmoother::ComputeSmoothFactor(ms, update_rate_);
}

void BlockBurgLPC::SetFormantShift(float shift) {
    fir_allpass_coeff_ = std::clamp(-shift, -0.99f, 0.99f);
}

void BlockBurgLPC::SetUseV2(bool use) {
    use_v2_ = use;
}

void BlockBurgLPC::SetForget(float ms) {
    forget_factor_ = std::exp(-1.0f / ((sample_rate_)*ms / 1000.0f));
}

void BlockBurgLPC::Process(qwqdsp_simd_element::PackFloat<2>* main_ptr, qwqdsp_simd_element::PackFloat<2>* side_ptr,
                           size_t num_samples) {
    if (use_v2_) {
        ProcessV2(main_ptr, side_ptr, num_samples);
    }
    else {
        ProcessV1(main_ptr, side_ptr, num_samples);
    }
}

void BlockBurgLPC::ProcessV1(qwqdsp_simd_element::PackFloat<2>* main_ptr, qwqdsp_simd_element::PackFloat<2>* side_ptr,
                             size_t num_samples) {
    // adding some noise
    for (size_t i = 0; i < num_samples; ++i) {
        main_ptr[i] += noise_.Next() * kNoiseGain;
    }
    // -------------------- copy buffer --------------------
    std::copy_n(main_ptr, num_samples, main_inputBuffer_.begin() + static_cast<int>(numInput_));
    std::copy_n(side_ptr, num_samples, side_inputBuffer_.begin() + static_cast<int>(numInput_));
    numInput_ += num_samples;
    while (numInput_ >= fft_size_) {
        // get the block
        std::span<qwqdsp_simd_element::PackFloat<2> const> main{main_inputBuffer_.data(), fft_size_};
        std::span<qwqdsp_simd_element::PackFloat<2> const> side{side_inputBuffer_.data(), fft_size_};
        // -------------------- lpc --------------------
        // forward fir lattice
        std::copy(main.begin(), main.end(), ef_.begin());
        std::copy(main.begin(), main.end(), eb_.begin());
        std::array<qwqdsp_simd_element::PackFloat<2>, kMaxPoles> latticek{};
        for (size_t kidx = 0; kidx < num_poles_; ++kidx) {
            auto& k = latticek[kidx];

            qwqdsp_simd_element::PackFloat<2> up{};
            qwqdsp_simd_element::PackFloat<2> down{};
            qwqdsp_simd_element::PackFloat<2> s_iir{};
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
        qwqdsp_simd_element::PackFloat<2> gain{};
        for (size_t i = 0; i < ef_.size(); ++i) {
            gain += ef_[i] * ef_[i];
        }
        gain = qwqdsp_simd_element::PackOps::Sqrt(gain);
        qwqdsp_simd_element::PackFloat<2> gain_side{};
        gain_side.Broadcast(std::sqrt(static_cast<float>(fft_size_)));
        auto atten = gain / (gain_side);
        // atten smooth
        gain_lag_ *= attack_factor_;
        gain_lag_ += (1.0f - attack_factor_) * atten;
        // iir lattice
        std::array<qwqdsp_simd_element::PackFloat<2>, kMaxPoles + 1> l_iir{};
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
        // pull input buffer a hop size
        numInput_ -= hop_size_;
        for (size_t i = 0; i < numInput_; i++) {
            main_inputBuffer_[i] = main_inputBuffer_[i + hop_size_];
        }
        for (size_t i = 0; i < numInput_; i++) {
            side_inputBuffer_[i] = side_inputBuffer_[i + hop_size_];
        }
        // overlay add
        for (size_t i = 0; i < fft_size_; i++) {
            main_outputBuffer_[i + writeAddBegin_] += ef_[i] * hann_window_[i];
        }
        writeEnd_ = writeAddBegin_ + fft_size_;
        writeAddBegin_ += hop_size_;
    }
    // -------------------- output --------------------
    if (writeAddBegin_ >= num_samples) {
        // extract output
        size_t extractSize = num_samples;
        for (size_t i = 0; i < extractSize; ++i) {
            main_ptr[i] = main_outputBuffer_[i] * 0.25f;
        }
        // shift output buffer
        size_t shiftSize = writeEnd_ - extractSize;
        for (size_t i = 0; i < shiftSize; i++) {
            main_outputBuffer_[i] = main_outputBuffer_[i + extractSize];
        }
        writeAddBegin_ -= extractSize;
        size_t newWriteEnd = writeEnd_ - extractSize;
        // zero shifed buffer
        for (size_t i = newWriteEnd; i < writeEnd_; ++i) {
            main_outputBuffer_[i].Broadcast(0);
        }
        writeEnd_ = newWriteEnd;
    }
    else {
        // zero buffer
        std::fill_n(main_ptr, num_samples, qwqdsp_simd_element::PackFloat<2>{});
    }
}

void BlockBurgLPC::ProcessV2(qwqdsp_simd_element::PackFloat<2>* main_ptr, qwqdsp_simd_element::PackFloat<2>* side_ptr,
                             size_t num_samples) {
    // adding some noise
    for (size_t i = 0; i < num_samples; ++i) {
        main_ptr[i] += noise_.Next() * kNoiseGain;
    }
    // -------------------- copy buffer --------------------
    std::copy_n(main_ptr, num_samples, main_inputBuffer_.begin() + static_cast<int>(numInput_));
    std::copy_n(side_ptr, num_samples, side_inputBuffer_.begin() + static_cast<int>(numInput_));
    numInput_ += num_samples;
    while (numInput_ >= fft_size_) {
        // get the block
        std::span<qwqdsp_simd_element::PackFloat<2> const> main{main_inputBuffer_.data(), fft_size_};
        std::span<qwqdsp_simd_element::PackFloat<2> const> side{side_inputBuffer_.data(), fft_size_};
        // -------------------- lpc --------------------
        // forward fir lattice
        std::copy(main.begin(), main.end(), ef_.begin());
        std::copy(main.begin(), main.end(), eb_.begin());
        std::array<qwqdsp_simd_element::PackFloat<2>, kMaxPoles> latticek{};
        for (size_t kidx = 0; kidx < num_poles_; ++kidx) {
            auto& k = latticek[kidx];

            qwqdsp_simd_element::PackFloat<2> up{};
            qwqdsp_simd_element::PackFloat<2> down{};
            qwqdsp_simd_element::PackFloat<2> s_iir{};
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
        qwqdsp_simd_element::PackFloat<2> gain{};
        for (size_t i = 0; i < ef_.size(); ++i) {
            gain += ef_[i] * ef_[i];
        }
        gain = qwqdsp_simd_element::PackOps::Sqrt(gain);
        qwqdsp_simd_element::PackFloat<2> gain_side{};
        gain_side.Broadcast(std::sqrt(static_cast<float>(fft_size_)));
        auto atten = gain / (gain_side);
        // atten smooth
        gain_lag_ *= attack_factor_;
        gain_lag_ += (1.0f - attack_factor_) * atten;
        // iir lattice
        std::array<qwqdsp_simd_element::PackFloat<2>, kMaxPoles + 1> l_iir{};
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
        // pull input buffer a hop size
        numInput_ -= hop_size_;
        for (size_t i = 0; i < numInput_; i++) {
            main_inputBuffer_[i] = main_inputBuffer_[i + hop_size_];
        }
        for (size_t i = 0; i < numInput_; i++) {
            side_inputBuffer_[i] = side_inputBuffer_[i + hop_size_];
        }
        // overlay add
        for (size_t i = 0; i < fft_size_; i++) {
            main_outputBuffer_[i + writeAddBegin_] += ef_[i] * hann_window_[i];
        }
        writeEnd_ = writeAddBegin_ + fft_size_;
        writeAddBegin_ += hop_size_;
    }
    // -------------------- output --------------------
    if (writeAddBegin_ >= num_samples) {
        // extract output
        size_t extractSize = num_samples;
        for (size_t i = 0; i < extractSize; ++i) {
            main_ptr[i] = main_outputBuffer_[i] * 0.25f;
        }
        // shift output buffer
        size_t shiftSize = writeEnd_ - extractSize;
        for (size_t i = 0; i < shiftSize; i++) {
            main_outputBuffer_[i] = main_outputBuffer_[i + extractSize];
        }
        writeAddBegin_ -= extractSize;
        size_t newWriteEnd = writeEnd_ - extractSize;
        // zero shifed buffer
        for (size_t i = newWriteEnd; i < writeEnd_; ++i) {
            main_outputBuffer_[i].Broadcast(0);
        }
        writeEnd_ = newWriteEnd;
    }
    else {
        // zero buffer
        std::fill_n(main_ptr, num_samples, qwqdsp_simd_element::PackFloat<2>{});
    }
}

void BlockBurgLPC::CopyLatticeCoeffient(std::span<float> buffer, size_t order) {
    auto const backup = latticek_;
    auto reverse_iir_it = backup.begin() + static_cast<int>(order);
    for (size_t i = 0; i < order; ++i) {
        buffer[i] = (*(--reverse_iir_it))[0];
    }
}

} // namespace green_vocoder::dsp
