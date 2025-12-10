#include "block_burg_lpc.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <span>
#include <qwqdsp/convert.hpp>

namespace green_vocoder::dsp {

void BlockBurgLPC::Init(float fs) {
    SetBlockSize(1024);
}

void BlockBurgLPC::SetBlockSize(size_t size) {
    fft_size_ = size;
    fft_.init(size);
    hann_window_.resize(size);
    eb_.resize(size);
    x_.resize(size);
    hop_size_ = size / 4;
    for (size_t i = 0; i < size; ++i) {
        hann_window_[i] = 0.5f - 0.5f * std::cos(2.0f* std::numbers::pi_v<float> * static_cast<float>(i) / static_cast<float>(size));
    }
}

void BlockBurgLPC::SetPoles(size_t num_poles) {
    num_poles_ = num_poles;
}

void BlockBurgLPC::Process(
    qwqdsp_simd_element::PackFloat<2>* main_ptr,
    qwqdsp_simd_element::PackFloat<2>* side_ptr,
    size_t num_samples
) {
    // -------------------- doing left --------------------
    std::copy_n(main_ptr, num_samples, main_inputBuffer_.begin() + static_cast<int>(numInput_));
    std::copy_n(side_ptr, num_samples, side_inputBuffer_.begin() + static_cast<int>(numInput_));
    numInput_ += num_samples;
    while (numInput_ >= fft_size_) {
        // get the block
        std::span<qwqdsp_simd_element::PackFloat<2> const> main{main_inputBuffer_.data(), fft_size_};
        std::span<qwqdsp_simd_element::PackFloat<2> const> side{side_inputBuffer_.data(), fft_size_};
        // -------------------- lpc --------------------
        // forward fir lattice
        std::copy(main.begin(), main.end(), x_.begin());
        std::copy(main.begin() + 1, main.end(), eb_.begin());
        for (size_t kidx = 0; kidx < num_poles_;) {
            auto& k = latticek[kidx];
            ++kidx;

            qwqdsp_simd_element::PackFloat<2> up{};
            qwqdsp_simd_element::PackFloat<2> down{};
            for (size_t i = 0; i < x_.size() - kidx; ++i) {
                up += x_[i] * eb_[i];
                down += x_[i] * x_[i];
                down += eb_[i] * eb_[i];
            }
            k = -2.0f * up / down;

            for (size_t i = 0; i < x_.size() - kidx - 1; ++i) {
                auto upgo = x_[i] + eb_[i] * k;
                auto downgo = eb_[i + 1] + x_[i + 1] * k;
                x_[i] = upgo;
                eb_[i] = downgo;
            }
        }
        // eval gain
        qwqdsp_simd_element::PackFloat<2> gain{};
        for (size_t i = 0; i < x_.size(); ++i) {
            gain += x_[i] * x_[i];
        }
        gain = qwqdsp_simd_element::PackOps::Sqrt(gain);
        qwqdsp_simd_element::PackFloat<2> gain_side{};
        gain_side.Broadcast(std::sqrt(static_cast<float>(fft_size_)));
        auto atten = gain / (gain_side);
        // iir lattice
        std::array<qwqdsp_simd_element::PackFloat<2>, kMaxPoles + 1> x_iir{};
        std::array<qwqdsp_simd_element::PackFloat<2>, kMaxPoles + 1> l_iir{};
        for (size_t j = 0; j < x_.size(); ++j) {
            x_iir[0] = side[j] * atten;
            for (size_t i = 0; i < num_poles_; ++i) {
                x_iir[i + 1] = x_iir[i] - latticek[num_poles_ - i - 1] * l_iir[i + 1];
            }
            for (size_t i = 0; i < num_poles_; ++i) {
                l_iir[i] = l_iir[i + 1] + latticek[num_poles_ - i - 1] * x_iir[i + 1];
            }
            l_iir[num_poles_] = x_iir[num_poles_];
            x_[j] = x_iir[num_poles_];
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
            main_outputBuffer_[i + writeAddBegin_] += x_[i] * hann_window_[i];
        }
        writeEnd_ = writeAddBegin_ + fft_size_;
        writeAddBegin_ += hop_size_;
    }
    // -------------------- output --------------------
    if (writeAddBegin_ >= num_samples) {
        // extract output
        size_t extractSize = num_samples;
        for (size_t i = 0; i < extractSize; ++i) {
            main_ptr[i] = main_outputBuffer_[i];
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

void BlockBurgLPC::CopyLatticeCoeffient(std::span<float> buffer) {
    auto backup = latticek;
    for (size_t i = 0; i < num_poles_; ++i) {
        size_t idx = static_cast<size_t>(i);
        buffer[idx] = backup[idx][0];
    }
}

}
