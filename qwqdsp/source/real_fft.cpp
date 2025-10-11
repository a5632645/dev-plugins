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

#include "qwqdsp/spectral/ipp_real_fft.hpp"

namespace qwqdsp::spectral {

RealFFT::RealFFT() {
    fft_ = std::make_unique<IppRealFFT>();
}

RealFFT::~RealFFT() = default;

void RealFFT::Init(size_t fft_size) {
    fft_->Init(fft_size);
    buffer_.resize(fft_size + 2);
}

void RealFFT::FFT(std::span<const float> time, std::span<std::complex<float>> spectral) noexcept {
    assert(time.size() == fft_size_);
    assert(spectral.size() == NumBins());

    fft_->FFT(time.data(), buffer_.data());
    
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

    fft_->FFT(time.data(), buffer_.data());
    
    size_t complexCounter = 0;
    size_t const num_bins = NumBins();
    for (size_t i = 0; i < num_bins; ++i)
    {
        real[i] = buffer_[complexCounter++];
        imag[i] = buffer_[complexCounter++];
    }
}

/**
    * @param phase 可选的，不需要请传入{}
    */
void RealFFT::FFTGainPhase(std::span<const float> time, std::span<float> gain, std::span<float> phase) noexcept {
    assert(time.size() == fft_size_);
    assert(gain.size() == NumBins());
    if (!phase.empty()) {
        assert(phase.size() == NumBins());
    }

    fft_->FFT(time.data(), buffer_.data());
    
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

    fft_->IFFT(buffer_.data(), time.data());
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

    fft_->IFFT(buffer_.data(), time.data());
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

    fft_->IFFT(buffer_.data(), time.data());
}

void RealFFT::Hilbert(std::span<const float>input, std::span<float> shift90, bool clear_dc) noexcept {
    assert(input.size() == fft_size_);
    assert(shift90.size() == fft_size_);

    fft_->FFT(input.data(), buffer_.data());
    
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

    fft_->IFFT(buffer_.data(), shift90.data());
}


} // qwqdsp::spectral

#endif
