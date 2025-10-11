#pragma once
#include <cstddef>
#include <span>
#include <vector>
#include <complex>
#include <cassert>

namespace qwqdsp::spectral {
#ifdef QWQDSP_HAVE_IPP
class IppRealFFT;
#endif

class RealFFT {
public:
    #ifdef QWQDSP_HAVE_IPP
    RealFFT();
    ~RealFFT();
    #endif

    void Init(size_t fft_size);

    void FFT(std::span<const float> time, std::span<std::complex<float>> spectral) noexcept;

    void FFT(std::span<const float> time, std::span<float> real, std::span<float> imag) noexcept;

    /**
     * @param phase 可选的，不需要请传入{}
     */
    void FFTGainPhase(std::span<const float> time, std::span<float> gain, std::span<float> phase = {}) noexcept;

    void IFFT(std::span<float> time, std::span<const std::complex<float>> spectral) noexcept;

    void IFFT(std::span<float> time, std::span<const float> real, std::span<const float> imag) noexcept;

    void IFFTGainPhase(std::span<float> time, std::span<const float> gain, std::span<const float> phase) noexcept;

    void Hilbert(std::span<const float> input, std::span<float> shift90, bool clear_dc) noexcept;

    /**
     * @brief 0 ~ N ---> -N/2 ~ N/2
     */
    void TimeDomainShift(std::span<float> block) noexcept {
        assert(block.size() == fft_size_);
        std::copy_n(block.begin(), fft_size_ / 2, buffer_.begin());
        for (size_t i = 0; i < fft_size_ / 2; ++i) {
            block[i] = block[i + fft_size_ / 2];
        }
        std::copy_n(buffer_.begin(), fft_size_ / 2, block.begin() + fft_size_ / 2);
    }

    size_t NumBins() const {
        return fft_size_ / 2 + 1;
    }

    static constexpr size_t NumBins(size_t fft_size) {
        return fft_size / 2 + 1;
    }

    size_t FFTSize() const {
        return fft_size_;
    }

private:
    size_t fft_size_{};
#ifndef QWQDSP_HAVE_IPP
    std::vector<int> ip_;
    std::vector<float> w_;
    std::vector<float> buffer_;
#else
    std::unique_ptr<IppRealFFT> fft_;
    std::vector<float> buffer_;
#endif
};
}