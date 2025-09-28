#include "qwqdsp/spectral/real_fft.hpp"

#include <bit>

#ifndef QWQDSP_HAVE_IPP

namespace qwqdsp::spectral {
namespace internal {
void rdft(int, int, float *, int *, float *) noexcept;
void makewt(int nw, int *ip, float *w) noexcept;
void makect(int nc, int *ip, float *c) noexcept;
} // qwqdsp::spectral::internal

void RealFFT::Init(size_t fft_size) {
    assert(std::has_single_bit(fft_size));
    fft_size_ = fft_size;
    ip_.resize(2 + std::ceil(std::sqrt(fft_size / 2.0f)));
    w_.resize(fft_size / 2);
    buffer_.resize(fft_size);
    const size_t size4 = fft_size / 4;
    internal::makewt(size4, ip_.data(), w_.data());
    internal::makect(size4, ip_.data(), w_.data() + size4);
}

void RealFFT::FFT(std::span<const float> time, std::span<std::complex<float>> spectral) noexcept {
    assert(time.size() == fft_size_);
    assert(spectral.size() == NumBins());

    std::copy(time.begin(), time.end(), buffer_.begin());
    internal::rdft(fft_size_, 1, buffer_.data(), ip_.data(), w_.data());
    spectral.front().real(buffer_[0]);
    spectral.front().imag(0.0f);
    spectral[fft_size_ / 2].real(-buffer_[1]);
    spectral[fft_size_ / 2].imag(0.0f);
    const size_t n = fft_size_ / 2;
    for (size_t i = 1; i < n; ++i) {
        spectral[i].real(buffer_[i * 2]);
        spectral[i].imag(-buffer_[i * 2 + 1]);
    }
}

void RealFFT::FFT(std::span<const float> time, std::span<float> real, std::span<float> imag) noexcept {
    assert(time.size() == fft_size_);
    assert(real.size() == NumBins());
    assert(imag.size() == NumBins());

    std::copy(time.begin(), time.end(), buffer_.begin());
    internal::rdft(fft_size_, 1, buffer_.data(), ip_.data(), w_.data());
    real.front() = buffer_[0];
    imag.front() = 0.0f;
    real[fft_size_ / 2] = -buffer_[1];
    imag[fft_size_ / 2] = 0.0f;
    const size_t n = fft_size_ / 2;
    for (size_t i = 1; i < n; ++i) {
        real[i] = buffer_[i * 2];
        imag[i] = -buffer_[i * 2 + 1];
    }
}

void RealFFT::IFFT(std::span<float> time, std::span<const std::complex<float>> spectral) noexcept {
    assert(time.size() == fft_size_);
    assert(spectral.size() == NumBins());

    buffer_[0] = spectral.front().real();
    buffer_[1] = -spectral[fft_size_ / 2].real();
    const size_t n = fft_size_ / 2;
    for (size_t i = 1; i < n; ++i) {
        buffer_[2 * i] = spectral[i].real();
        buffer_[2 * i + 1] = -spectral[i].imag();
    }
    internal::rdft(fft_size_, -1, buffer_.data(), ip_.data(), w_.data());
    float gain = 2.0f / fft_size_;
    for (size_t i = 0; i < fft_size_; ++i) {
        time[i] = buffer_[i] * gain;
    }
}

void RealFFT::IFFT(std::span<float> time, std::span<const float> real, std::span<const float> imag) noexcept {
    assert(time.size() == fft_size_);
    assert(real.size() == NumBins());
    assert(imag.size() == NumBins());

    buffer_[0] = real.front();
    buffer_[1] = -real[fft_size_ / 2];
    const size_t n = fft_size_ / 2;
    for (size_t i = 1; i < n; ++i) {
        buffer_[2 * i] = real[i];
        buffer_[2 * i + 1] = -imag[i];
    }
    internal::rdft(fft_size_, -1, buffer_.data(), ip_.data(), w_.data());
    float gain = 2.0f / fft_size_;
    for (size_t i = 0; i < fft_size_; ++i) {
        time[i] = buffer_[i] * gain;
    }
}

void RealFFT::FFTGainPhase(std::span<const float> time, std::span<float> gain, std::span<float> phase) noexcept {
    assert(time.size() == fft_size_);
    assert(gain.size() == NumBins());
    if (!phase.empty()) {
        assert(phase.size() == NumBins());
    }

    std::copy(time.begin(), time.end(), buffer_.begin());
    internal::rdft(fft_size_, 1, buffer_.data(), ip_.data(), w_.data());
    if (phase.empty()) {
        gain.front() = std::abs(buffer_[0]);
        gain[fft_size_ / 2] = std::abs(buffer_[1]);
        const size_t n = fft_size_ / 2;
        for (size_t i = 1; i < n; ++i) {
            float real = buffer_[i * 2];
            float imag = -buffer_[i * 2 + 1];
            gain[i] = std::sqrt(real * real + imag * imag);
        }
    }
    else {
        gain.front() = std::abs(buffer_[0]);
        phase.front() = 0.0f;
        gain[fft_size_ / 2] = std::abs(buffer_[1]);
        phase[fft_size_ / 2] = 0.0f;
        const size_t n = fft_size_ / 2;
        for (size_t i = 1; i < n; ++i) {
            float real = buffer_[i * 2];
            float imag = -buffer_[i * 2 + 1];
            gain[i] = std::sqrt(real * real + imag + imag);
            phase[i] = std::atan2(imag, real);
        }
    }
}

void RealFFT::IFFTGainPhase(std::span<float> time, std::span<const float> gain, std::span<const float> phase) noexcept {
    assert(time.size() == fft_size_);
    assert(gain.size() == NumBins());
    assert(phase.size() == NumBins());

    buffer_[0] = gain[0];
    buffer_[1] = -gain[fft_size_ / 2];
    const size_t n = fft_size_ / 2;
    for (size_t i = 1; i < n; ++i) {
        buffer_[2 * i] = gain[i] * std::cos(phase[i]);
        buffer_[2 * i + 1] = -gain[i] * std::sin(phase[i]);
    }
    internal::rdft(fft_size_, -1, buffer_.data(), ip_.data(), w_.data());
    float g = 2.0f / fft_size_;
    for (size_t i = 0; i < fft_size_; ++i) {
        time[i] = buffer_[i] * g;
    }
}

void RealFFT::Hilbert(std::span<const float> input, std::span<float> shift90, bool clear_dc) noexcept {
    assert(input.size() == fft_size_);
    assert(shift90.size() == fft_size_);
    std::copy(input.begin(), input.end(), buffer_.begin());
    internal::rdft(fft_size_, 1, buffer_.data(), ip_.data(), w_.data());
    const size_t n = fft_size_ / 2;
    for (size_t i = 1; i < n; ++i) {
        float real = buffer_[i * 2];
        float imag = -buffer_[i * 2 + 1];
        float re = imag;
        float im = -real;
        buffer_[i * 2] = re;
        buffer_[i * 2 + 1] = -im;
    }
    if (clear_dc) {
        buffer_[0] = 0;
        buffer_[1] = 0;
    }
    internal::rdft(fft_size_, -1, buffer_.data(), ip_.data(), w_.data());
    float gain = 2.0f / fft_size_;
    for (size_t i = 0; i < fft_size_; ++i) {
        shift90[i] = buffer_[i] * gain;
    }
}

} // qwqdsp::spectral

#else

#include <ipp.h>
#include "ipp_init.hpp"

namespace qwqdsp::spectral {

RealFFT::RealFFT() {
    internal::InitIPPOnce();
}

RealFFT::~RealFFT() {
    if (p_spec_) {
        ippFree(p_spec_);
    }
    if (p_buffer_) {
        ippFree(p_buffer_);
    }
}

void RealFFT::Init(size_t fft_size) {
    assert(std::has_single_bit(fft_size));
    fft_size_ = fft_size;

    int order = std::countr_zero(fft_size);
    int p_spec_size{};
    int p_spec_buffer_size{};
    int p_buffer_size{};
    int const flag = IPP_FFT_DIV_INV_BY_N;
    ippsFFTGetSize_R_32f(order, flag, ippAlgHintFast, &p_spec_size, &p_spec_buffer_size, &p_buffer_size);

    if (p_spec_) {
        ippFree(p_spec_);
    }
    p_spec_ = ippsMalloc_8u(p_spec_size);
    if (p_spec_buffer_) {
        ippFree(p_spec_buffer_);
    }
    p_spec_buffer_ = p_spec_buffer_size > 0 ? ippsMalloc_8u(p_spec_buffer_size) : nullptr;
    if (p_buffer_) {
        ippFree(p_buffer_);
    }
    p_buffer_ = p_buffer_size > 0 ? ippsMalloc_8u(p_buffer_size) : nullptr;

    ippsFFTInit_R_32f((IppsFFTSpec_R_32f**)&p_fft_spec_, order, flag, ippAlgHintFast, p_spec_, p_spec_buffer_);
    buffer_.resize(fft_size + 2);
    if (p_spec_buffer_) {
        ippFree(p_spec_buffer_);
    }
}

void RealFFT::FFT(std::span<const float> time, std::span<std::complex<float>> spectral) noexcept {
    assert(time.size() == fft_size_);
    assert(spectral.size() == NumBins());

    ippsFFTFwd_RToCCS_32f(time.data(), buffer_.data(), (IppsFFTSpec_R_32f*)p_fft_spec_, p_buffer_);
    
    size_t complexCounter = 0;
    size_t const num_bins = NumBins();
    for (size_t i = 0; i < num_bins; ++i)
    {
        float re = buffer_[complexCounter++];
        float im = buffer_[complexCounter++];
        spectral[i] = {re, im};
    }
}

void RealFFT::FFT(std::span<const float> time, std::span<float> real, std::span<float> imag) noexcept {
    assert(time.size() == fft_size_);
    assert(real.size() == NumBins());
    assert(imag.size() == NumBins());

    ippsFFTFwd_RToCCS_32f(time.data(), buffer_.data(), (IppsFFTSpec_R_32f*)p_fft_spec_, p_buffer_);
    
    size_t complexCounter = 0;
    size_t const num_bins = NumBins();
    for (size_t i = 0; i < num_bins; ++i)
    {
        real[i] = buffer_[complexCounter++];
        imag[i] = buffer_[complexCounter++];
    }
}

void RealFFT::IFFT(std::span<float> time, std::span<const std::complex<float>> spectral) noexcept {
    assert(time.size() == fft_size_);
    assert(spectral.size() == NumBins());

    size_t complexCounter = 0;
    size_t const num_bins = NumBins();
    for (size_t i = 0; i < num_bins; ++i)
    {
        buffer_[complexCounter++] = spectral[i].real();
        buffer_[complexCounter++] = spectral[i].imag();
    }

    ippsFFTInv_CCSToR_32f(buffer_.data(), time.data(), (IppsFFTSpec_R_32f*)p_fft_spec_, p_buffer_);
}

void RealFFT::IFFT(std::span<float> time, std::span<const float> real, std::span<const float> imag) noexcept {
    assert(time.size() == fft_size_);
    assert(real.size() == NumBins());
    assert(imag.size() == NumBins());

    size_t complexCounter = 0;
    size_t const num_bins = NumBins();
    for (size_t i = 0; i < num_bins; ++i)
    {
        buffer_[complexCounter++] = real[i];
        buffer_[complexCounter++] = imag[i];
    }

    ippsFFTInv_CCSToR_32f(buffer_.data(), time.data(), (IppsFFTSpec_R_32f*)p_fft_spec_, p_buffer_);
}

void RealFFT::FFTGainPhase(std::span<const float> time, std::span<float> gain, std::span<float> phase) noexcept {
    assert(time.size() == fft_size_);
    assert(gain.size() == NumBins());
    if (!phase.empty()) {
        assert(phase.size() == NumBins());
    }

    ippsFFTFwd_RToCCS_32f(time.data(), buffer_.data(), (IppsFFTSpec_R_32f*)p_fft_spec_, p_buffer_);
    
    size_t complexCounter = 0;
    size_t const num_bins = NumBins();
    if (phase.empty()) {
        for (size_t i = 0; i < num_bins; ++i)
        {
            float re = buffer_[complexCounter++];
            float im = buffer_[complexCounter++];
            gain[i] = std::sqrt(re * re + im * im);
        }
    }
    else {
        for (size_t i = 0; i < num_bins; ++i)
        {
            float re = buffer_[complexCounter++];
            float im = buffer_[complexCounter++];
            gain[i] = std::sqrt(re * re + im * im);
            phase[i] = std::atan2(im, re);
        }
    }
}

void RealFFT::IFFTGainPhase(std::span<float> time, std::span<const float> gain, std::span<const float> phase) noexcept {
    assert(time.size() == fft_size_);
    assert(gain.size() == NumBins());
    assert(phase.size() == NumBins());

    size_t const num_bins = NumBins();
    size_t idx = 0;
    for (size_t i = 0; i < num_bins; ++i) {
        auto const a = std::polar(gain[i], phase[i]);
        buffer_[idx++] = a.real();
        buffer_[idx++] = a.imag();
    }
    ippsFFTInv_CCSToR_32f(buffer_.data(), time.data(), (IppsFFTSpec_R_32f*)p_fft_spec_, p_buffer_);
}

void RealFFT::Hilbert(std::span<const float> input, std::span<float> shift90, bool clear_dc) noexcept {
    assert(input.size() == fft_size_);
    assert(shift90.size() == fft_size_);
    
    ippsFFTFwd_RToCCS_32f(input.data(), buffer_.data(), (IppsFFTSpec_R_32f*)p_fft_spec_, p_buffer_);
    
    const size_t n = fft_size_ / 2;
    for (size_t i = 1; i < n; ++i) {
        float real = buffer_[i * 2];
        float imag = -buffer_[i * 2 + 1];
        float re = imag;
        float im = -real;
        buffer_[i * 2] = re;
        buffer_[i * 2 + 1] = -im;
    }
    if (clear_dc) {
        buffer_[0] = 0;
        buffer_[1] = 0;
        buffer_[fft_size_] = 0;
        buffer_[fft_size_ + 1] = 0;
    }

    ippsFFTInv_CCSToR_32f(buffer_.data(), shift90.data(), (IppsFFTSpec_R_32f*)p_fft_spec_, p_buffer_);
}

} // qwqdsp::spectral

#endif
