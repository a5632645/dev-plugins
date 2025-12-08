#include "qwqdsp/spectral/complex_fft.hpp"

#include <bit>

#ifndef QWQDSP_HAVE_IPP

namespace qwqdsp_spectral {
namespace internal {

// --------------------------------------------------------------------------------
// complex fft
//
// oouras的复数FFT返回为
// 0 -1 -2 ...... -N/2(N/2) ....... 2 1
// idx
// 0  1  2 ...... N/2 ............N-2 N-1
// --------------------------------------------------------------------------------

void cdft(int, int, float *, int *, float *) noexcept;
void makewt(int nw, int *ip, float *w) noexcept;
void makect(int nc, int *ip, float *c) noexcept;
} // qwq::spectral::internal


void ComplexFFT::Init(size_t fft_size) {
    assert(std::has_single_bit(fft_size));
    fft_size_ = fft_size;
    ip_.resize(2 + std::ceil(std::sqrt(fft_size / 2.0f)));
    w_.resize(fft_size / 2);
    buffer_.resize(fft_size * 2);
    const size_t size4 = fft_size / 2;
    internal::makewt(size4, ip_.data(), w_.data());
}


void ComplexFFT::FFT(std::span<const float> time, std::span<std::complex<float>> spectral) noexcept {
    assert(time.size() == fft_size_);
    assert(spectral.size() == NumBins());

    for (size_t i = 0; i < fft_size_; ++i) {
        buffer_[2 * i] = time[i];
        buffer_[2 * i + 1] = 0.0f;
    }
    internal::cdft(fft_size_ * 2, 1, buffer_.data(), ip_.data(), w_.data());

    spectral[0].real(buffer_[0]);
    spectral[0].imag(buffer_[1]);
    for (size_t i = 1; i < fft_size_; ++i) {
        spectral[fft_size_ - i].real(buffer_[i * 2]);
        spectral[fft_size_ - i].imag(buffer_[i * 2 + 1]);
    }
}


void ComplexFFT::FFT(std::span<const std::complex<float>> time, std::span<std::complex<float>> spectral) noexcept {
    assert(time.size() == fft_size_);
    assert(spectral.size() == NumBins());

    for (size_t i = 0; i < fft_size_; ++i) {
        buffer_[2 * i] = time[i].real();
        buffer_[2 * i + 1] = time[i].imag();
    }
    internal::cdft(fft_size_ * 2, 1, buffer_.data(), ip_.data(), w_.data());

    spectral[0].real(buffer_[0]);
    spectral[0].imag(buffer_[1]);
    for (size_t i = 1; i < fft_size_; ++i) {
        spectral[fft_size_ - i].real(buffer_[i * 2]);
        spectral[fft_size_ - i].imag(buffer_[i * 2 + 1]);
    }
}


void ComplexFFT::FFT(std::span<const float> time, std::span<float> real, std::span<float> imag) noexcept {
    assert(time.size() == fft_size_);
    assert(real.size() == NumBins());
    assert(imag.size() == NumBins());

    for (size_t i = 0; i < fft_size_; ++i) {
        buffer_[2 * i] = time[i];
        buffer_[2 * i + 1] = 0.0f;
    }
    internal::cdft(fft_size_ * 2, 1, buffer_.data(), ip_.data(), w_.data());

    real[0] = (buffer_[0]);
    imag[0] = (buffer_[1]);
    for (size_t i = 1; i < fft_size_; ++i) {
        real[fft_size_ - i] = (buffer_[i * 2]);
        imag[fft_size_ - i] = (buffer_[i * 2 + 1]);
    }
}


void ComplexFFT::FFT(std::span<const std::complex<float>> time, std::span<float> real, std::span<float> imag) noexcept {
    assert(time.size() == fft_size_);
    assert(real.size() == NumBins());
    assert(imag.size() == NumBins());

    for (size_t i = 0; i < fft_size_; ++i) {
        buffer_[2 * i] = time[i].real();
        buffer_[2 * i + 1] = time[i].imag();
    }
    internal::cdft(fft_size_ * 2, 1, buffer_.data(), ip_.data(), w_.data());

    real[0] = (buffer_[0]);
    imag[0] = (buffer_[1]);
    for (size_t i = 1; i < fft_size_; ++i) {
        real[fft_size_ - i] = (buffer_[i * 2]);
        imag[fft_size_ - i] = (buffer_[i * 2 + 1]);
    }
}


void ComplexFFT::FFTGainPhase(std::span<const float> time, std::span<float> gain, std::span<float> phase) noexcept {
    assert(time.size() == fft_size_);
    assert(gain.size() == NumBins());
    if (!phase.empty()) {
        assert(phase.size() == NumBins());
    }

    for (size_t i = 0; i < fft_size_; ++i) {
        buffer_[2 * i] = time[i];
        buffer_[2 * i + 1] = 0;
    }
    internal::cdft(fft_size_ * 2, 1, buffer_.data(), ip_.data(), w_.data());

    {
        float real = (buffer_[0]);
        float imag = (buffer_[1]);
        gain[0] = std::sqrt(real * real + imag * imag);
        if (!phase.empty()) phase[0] = std::atan2(imag, real);
    }
    for (size_t i = 1; i < fft_size_; ++i) {
        float real = (buffer_[i * 2]);
        float imag = (buffer_[i * 2 + 1]);
        gain[fft_size_ - i] = std::sqrt(real * real + imag * imag);
        if (!phase.empty()) phase[fft_size_ - i] = std::atan2(imag, real);
    }
}


void ComplexFFT::IFFT(std::span<float> time, std::span<const std::complex<float>> spectral) noexcept {
    buffer_[0] = spectral[0].real();
    buffer_[1] = spectral[1].imag();
    for (size_t i = 1; i < fft_size_; ++i) {
        buffer_[2 * i] = spectral[fft_size_ - i].real();
        buffer_[2 * i + 1] = spectral[fft_size_ - i].imag();
    }
    internal::cdft(fft_size_ * 2, -1, buffer_.data(), ip_.data(), w_.data());
    const float gain = 1.0f / fft_size_;
    for (size_t i = 0; i < fft_size_; ++i) {
        time[i] = buffer_[i * 2] * gain;
    }
}


void ComplexFFT::IFFT(std::span<std::complex<float>> time, std::span<const std::complex<float>> spectral) noexcept {
    buffer_[0] = spectral[0].real();
    buffer_[1] = spectral[1].imag();
    for (size_t i = 1; i < fft_size_; ++i) {
        buffer_[2 * i] = spectral[fft_size_ - i].real();
        buffer_[2 * i + 1] = spectral[fft_size_ - i].imag();
    }
    internal::cdft(fft_size_ * 2, -1, buffer_.data(), ip_.data(), w_.data());
    const float gain = 1.0f / fft_size_;
    for (size_t i = 0; i < fft_size_; ++i) {
        time[i].real(buffer_[i * 2] * gain);
        time[i].imag(buffer_[i * 2 + 1] * gain);
    }
}


void ComplexFFT::IFFT(std::span<float> time, std::span<const float> real, std::span<const float> imag) noexcept {
    buffer_[0] = real[0];
    buffer_[1] = imag[1];
    for (size_t i = 1; i < fft_size_; ++i) {
        buffer_[2 * i] = real[fft_size_ - i];
        buffer_[2 * i + 1] = imag[fft_size_ - i];
    }
    internal::cdft(fft_size_ * 2, -1, buffer_.data(), ip_.data(), w_.data());
    const float gain = 1.0f / fft_size_;
    for (size_t i = 0; i < fft_size_; ++i) {
        time[i] = buffer_[i * 2] * gain;
    }
}


void ComplexFFT::IFFT(std::span<std::complex<float>> time, std::span<const float> real, std::span<const float> imag) noexcept {
    buffer_[0] = real[0];
    buffer_[1] = imag[1];
    for (size_t i = 1; i < fft_size_; ++i) {
        buffer_[2 * i] = real[fft_size_ - i];
        buffer_[2 * i + 1] = imag[fft_size_ - i];
    }
    internal::cdft(fft_size_ * 2, -1, buffer_.data(), ip_.data(), w_.data());
    const float gain = 1.0f / fft_size_;
    for (size_t i = 0; i < fft_size_; ++i) {
        time[i].real(buffer_[i * 2] * gain);
        time[i].imag(buffer_[i * 2 + 1] * gain);
    }
}


void ComplexFFT::IFFTGainPhase(std::span<float> time, std::span<const float> gain, std::span<const float> phase) noexcept {
    buffer_[0] = gain[0];
    buffer_[1] = gain[1];
    for (size_t i = 1; i < fft_size_; ++i) {
        buffer_[2 * i] = gain[fft_size_ - i] * std::cos(phase[fft_size_ - i]);
        buffer_[2 * i + 1] = gain[fft_size_ - i] * std::sin(phase[fft_size_ - i]);
    }
    internal::cdft(fft_size_ * 2, -1, buffer_.data(), ip_.data(), w_.data());
    const float g = 1.0f / fft_size_;
    for (size_t i = 0; i < fft_size_; ++i) {
        time[i] = buffer_[i * 2] * g;
    }
}


void ComplexFFT::IFFTGainPhase(std::span<std::complex<float>> time, std::span<const float> gain, std::span<const float> phase) noexcept {
    buffer_[0] = gain[0];
    buffer_[1] = gain[1];
    for (size_t i = 1; i < fft_size_; ++i) {
        buffer_[2 * i] = gain[fft_size_ - i] * std::cos(phase[fft_size_ - i]);
        buffer_[2 * i + 1] = gain[fft_size_ - i] * std::sin(phase[fft_size_ - i]);
    }
    internal::cdft(fft_size_ * 2, -1, buffer_.data(), ip_.data(), w_.data());
    const float g = 1.0f / fft_size_;
    for (size_t i = 0; i < fft_size_; ++i) {
        time[i].real(buffer_[i * 2] * g);
        time[i].imag(buffer_[i * 2 + 1] * g);
    }
}


void ComplexFFT::Hilbert(std::span<const float> time, std::span<std::complex<float>> output, bool clear_dc) noexcept {
    assert(time.size() == fft_size_);
    assert(output.size() == fft_size_);

    for (size_t i = 0; i < fft_size_; ++i) {
        buffer_[2 * i] = time[i];
        buffer_[2 * i + 1] = 0.0f;
    }
    internal::cdft(fft_size_ * 2, 1, buffer_.data(), ip_.data(), w_.data());
    if (clear_dc) {
        // Z[0] = X[0]
        buffer_[0] = 0.0f;
        buffer_[1] = 0.0f;
        // Z[N/2] = x[N/2]
        buffer_[fft_size_] = 0.0f;
        buffer_[fft_size_ + 1] = 0.0f;
    }
    else {
        // Z[0] = X[0]
        buffer_[0] *= 0.5f;
        buffer_[1] *= 0.5f;
        // Z[N/2] = x[N/2]
        buffer_[fft_size_] *= 0.5f;
        buffer_[fft_size_ + 1] *= 0.5f;
    }
    // Z[negative frequency] = 0
    for (size_t i = 1; i < fft_size_ / 2; ++i) {
        buffer_[2 * i] = 0.0f;
        buffer_[2 * i + 1] = 0.0f;
    }
    internal::cdft(fft_size_ * 2, -1, buffer_.data(), ip_.data(), w_.data());
    // Z[n] = 2 * X[n]
    const float gain = 2.0f / fft_size_;
    for (size_t i = 0; i < fft_size_; ++i) {
        output[i].real(buffer_[i * 2] * gain);
        output[i].imag(buffer_[i * 2 + 1] * gain);
    }
}


void ComplexFFT::Hilbert(std::span<const float> input, std::span<float> output90, bool clear_dc) noexcept {
    assert(input.size() == fft_size_);
    assert(output90.size() == fft_size_);

    for (size_t i = 0; i < fft_size_; ++i) {
        buffer_[2 * i] = input[i];
        buffer_[2 * i + 1] = 0.0f;
    }
    internal::cdft(fft_size_ * 2, 1, buffer_.data(), ip_.data(), w_.data());
    if (clear_dc) {
        // Z[0] = X[0]
        buffer_[0] = 0.0f;
        buffer_[1] = 0.0f;
        // Z[N/2] = x[N/2]
        buffer_[fft_size_] = 0.0f;
        buffer_[fft_size_ + 1] = 0.0f;
    }
    // Z[negative frequency] -> -b + ai
    for (size_t i = 1; i < fft_size_ / 2; ++i) {
        float re = buffer_[2 * i];
        float im = buffer_[2 * i + 1];
        buffer_[2 * i] = -im;
        buffer_[2 * i + 1] = re;
    }
    // Z[n] -> b - ai
    for (size_t i = fft_size_ / 2 + 1; i < fft_size_; ++i) {
        float re = buffer_[2 * i];
        float im = buffer_[2 * i + 1];
        buffer_[2 * i] = im;
        buffer_[2 * i + 1] = -re;
    }
    internal::cdft(fft_size_ * 2, -1, buffer_.data(), ip_.data(), w_.data());
    const float gain = 1.0f / fft_size_;
    for (size_t i = 0; i < fft_size_; ++i) {
        output90[i] = buffer_[i * 2] * gain;
    }
}

} // qwq::spectral

#else

#include "qwqdsp/spectral/ipp_complex_fft.hpp"

// Intel IPP 默认正频率起步
namespace qwqdsp_spectral {

ComplexFFT::ComplexFFT() {
    fft_ = std::make_unique<IppComplexFFT>();
}

ComplexFFT::~ComplexFFT() = default;

void ComplexFFT::Init(size_t fft_size) {
    fft_->Init(fft_size);
    real_buffer_.resize(fft_size);
    imag_buffer_.resize(fft_size);
    src_imag_buffer_.resize(fft_size);
    src_real_buffer_.resize(fft_size);
    fft_size_ = fft_size;
}


void ComplexFFT::FFT(std::span<const float> time, std::span<std::complex<float>> spectral) noexcept {
    assert(time.size() == fft_size_);
    assert(spectral.size() == NumBins());

    std::fill(src_imag_buffer_.begin(), src_imag_buffer_.end(), float{});
    fft_->FFT(time.data(), src_imag_buffer_.data(), real_buffer_.data(), imag_buffer_.data());
    
    for (size_t i = 0; i < fft_size_; ++i)
    {
        float const re = real_buffer_[i];
        float const im = imag_buffer_[i];
        spectral[i] = {re, im};
    }
}


void ComplexFFT::FFT(std::span<const std::complex<float>> time, std::span<std::complex<float>> spectral) noexcept {
    assert(time.size() == fft_size_);
    assert(spectral.size() == NumBins());

    for (size_t i = 0; i < fft_size_; ++i) {
        src_real_buffer_[i] = time[i].real();
        src_imag_buffer_[i] = time[i].imag();
    }
    fft_->FFT(src_real_buffer_.data(), src_imag_buffer_.data(), real_buffer_.data(), imag_buffer_.data());
    for (size_t i = 0; i < fft_size_; ++i) {
        spectral[i].real(real_buffer_[i]);
        spectral[i].imag(imag_buffer_[i]);
    }
}


void ComplexFFT::FFT(std::span<const float> time, std::span<float> real, std::span<float> imag) noexcept {
    assert(time.size() == fft_size_);
    assert(real.size() == NumBins());
    assert(imag.size() == NumBins());

    std::fill(src_imag_buffer_.begin(), src_imag_buffer_.end(), float{});
    fft_->FFT(time.data(), src_imag_buffer_.data(), real.data(), imag.data());
}


void ComplexFFT::FFT(std::span<const std::complex<float>> time, std::span<float> real, std::span<float> imag) noexcept {
    assert(time.size() == fft_size_);
    assert(real.size() == NumBins());
    assert(imag.size() == NumBins());

    for (size_t i = 0; i < fft_size_; ++i) {
        src_real_buffer_[i] = time[i].real();
        src_imag_buffer_[i] = time[i].imag();
    }
    fft_->FFT(src_real_buffer_.data(), src_imag_buffer_.data(), real.data(), imag.data());
}


void ComplexFFT::FFTGainPhase(std::span<const float> time, std::span<float> gain, std::span<float> phase) noexcept {
    assert(time.size() == fft_size_);
    assert(gain.size() == NumBins());
    if (!phase.empty()) {
        assert(phase.size() == NumBins());
    }

    std::fill(src_imag_buffer_.begin(), src_imag_buffer_.end(), float{});
    fft_->FFT(time.data(), src_imag_buffer_.data(), real_buffer_.data(), imag_buffer_.data());

    if (phase.empty()) {
        for (size_t i = 0; i < fft_size_; ++i) {
            float const re = real_buffer_[i];
            float const im = imag_buffer_[i];
            gain[i] = std::sqrt(re * re + im * im);
        }
    }
    else {
        for (size_t i = 0; i < fft_size_; ++i) {
            float const re = real_buffer_[i];
            float const im = imag_buffer_[i];
            gain[i] = std::sqrt(re * re + im * im);
            phase[i] = std::atan2(im, re);
        }
    }
}


void ComplexFFT::FFTGainPhase(std::span<const std::complex<float>> time, std::span<float> gain, std::span<float> phase) noexcept {
    assert(time.size() == fft_size_);
    assert(gain.size() == NumBins());
    if (!phase.empty()) {
        assert(phase.size() == NumBins());
    }

    for (size_t i = 0; i < fft_size_; ++i) {
        src_real_buffer_[i] = time[i].real();
        src_imag_buffer_[i] = time[i].imag();
    }
    fft_->FFT(src_real_buffer_.data(), src_imag_buffer_.data(), real_buffer_.data(), imag_buffer_.data());

    if (phase.empty()) {
        for (size_t i = 0; i < fft_size_; ++i) {
            float const re = real_buffer_[i];
            float const im = imag_buffer_[i];
            gain[i] = std::sqrt(re * re + im * im);
        }
    }
    else {
        for (size_t i = 0; i < fft_size_; ++i) {
            float const re = real_buffer_[i];
            float const im = imag_buffer_[i];
            gain[i] = std::sqrt(re * re + im * im);
            phase[i] = std::atan2(im, re);
        }
    }
}


void ComplexFFT::IFFT(std::span<float> time, std::span<const std::complex<float>> spectral) noexcept {
    for (size_t i = 0; i < fft_size_; ++i) {
        src_real_buffer_[i] = spectral[i].real();
        src_imag_buffer_[i] = spectral[i].imag();
    }

    fft_->IFFT(src_real_buffer_.data(), src_imag_buffer_.data(), time.data(), imag_buffer_.data());
}


void ComplexFFT::IFFT(std::span<std::complex<float>> time, std::span<const std::complex<float>> spectral) noexcept {
    for (size_t i = 0; i < fft_size_; ++i) {
        src_real_buffer_[i] = spectral[i].real();
        src_imag_buffer_[i] = spectral[i].imag();
    }

    fft_->IFFT(src_real_buffer_.data(), src_imag_buffer_.data(), real_buffer_.data(), imag_buffer_.data());

    for (size_t i = 0; i < fft_size_; ++i) {
        time[i] = {real_buffer_[i], imag_buffer_[i]};
    }
}


void ComplexFFT::IFFT(std::span<float> time, std::span<const float> real, std::span<const float> imag) noexcept {
    fft_->IFFT(real.data(), imag.data(), time.data(), imag_buffer_.data());
}


void ComplexFFT::IFFT(std::span<std::complex<float>> time, std::span<const float> real, std::span<const float> imag) noexcept {
    fft_->IFFT(real.data(), imag.data(), real_buffer_.data(), imag_buffer_.data());

    for (size_t i = 0; i < fft_size_; ++i) {
        time[i] = {real_buffer_[i], imag_buffer_[i]};
    }
}


void ComplexFFT::IFFTGainPhase(std::span<float> time, std::span<const float> gain, std::span<const float> phase) noexcept {
    for (size_t i = 0; i < fft_size_; ++i) {
        auto const a = std::polar(gain[i], phase[i]);
        src_real_buffer_[i] = a.real();
        src_imag_buffer_[i] = a.imag();
    }

    fft_->IFFT(src_real_buffer_.data(), src_imag_buffer_.data(), time.data(), imag_buffer_.data());
}


void ComplexFFT::IFFTGainPhase(std::span<std::complex<float>> time, std::span<const float> gain, std::span<const float> phase) noexcept {
    for (size_t i = 0; i < fft_size_; ++i) {
        auto const a = std::polar(gain[i], phase[i]);
        src_real_buffer_[i] = a.real();
        src_imag_buffer_[i] = a.imag();
    }

    fft_->IFFT(src_real_buffer_.data(), src_imag_buffer_.data(), real_buffer_.data(), imag_buffer_.data());

    for (size_t i = 0; i < fft_size_; ++i) {
        time[i] = {real_buffer_[i], imag_buffer_[i]};
    }
}


void ComplexFFT::Hilbert(std::span<const float> time, std::span<std::complex<float>> output, bool clear_dc) noexcept {
    assert(time.size() == fft_size_);
    assert(output.size() == fft_size_);

    std::fill(src_imag_buffer_.begin(), src_imag_buffer_.end(), float{});
    fft_->FFT(time.data(), src_imag_buffer_.data(), real_buffer_.data(), imag_buffer_.data());

    size_t const nyquist_idx = fft_size_ / 2;
    for (size_t i = nyquist_idx + 1; i < fft_size_; ++i) {
        real_buffer_[i] = 0;
        imag_buffer_[i] = 0;
    }
    if (clear_dc) {
        real_buffer_[0] = 0;
        imag_buffer_[0] = 0;
        real_buffer_[nyquist_idx] = 0;
        imag_buffer_[nyquist_idx] = 0;
    }

    fft_->IFFT(real_buffer_.data(), imag_buffer_.data(), src_real_buffer_.data(), src_imag_buffer_.data());
    for (size_t i = 0; i < fft_size_; ++i) {
        output[i] = {src_real_buffer_[i], src_imag_buffer_[i]};
    }
}


void ComplexFFT::Hilbert(std::span<const float> input, std::span<float> output90, bool clear_dc) noexcept {
    assert(input.size() == fft_size_);
    assert(output90.size() == fft_size_);

    std::fill(src_imag_buffer_.begin(), src_imag_buffer_.end(), float{});
    fft_->FFT(input.data(), src_imag_buffer_.data(), real_buffer_.data(), imag_buffer_.data());

    size_t const nyquist_idx = fft_size_ / 2;
    for (size_t i = nyquist_idx + 1; i < fft_size_; ++i) {
        real_buffer_[i] = 0;
        imag_buffer_[i] = 0;
    }
    if (clear_dc) {
        real_buffer_[0] = 0;
        imag_buffer_[0] = 0;
        real_buffer_[nyquist_idx] = 0;
        imag_buffer_[nyquist_idx] = 0;
    }

    fft_->IFFT(real_buffer_.data(), imag_buffer_.data(), src_real_buffer_.data(), output90.data());
}

} // qwq::spectral

#endif
