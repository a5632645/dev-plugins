#pragma once
#include <cstddef>
#include <span>
#include <vector>
#include <complex>

namespace qwqdsp::spectral {
enum class ComplexFFTResultType {
    kNegPiToPosPi,
    kZeroToTwoPi
};

template<ComplexFFTResultType kResultType>
class ComplexFFT {
public:
    void Init(size_t fft_size);
    
    void FFT(std::span<const float> time, std::span<std::complex<float>> spectral);

    void FFT(std::span<const std::complex<float>> time, std::span<std::complex<float>> spectral);

    void IFFT(std::span<std::complex<float>> time, std::span<std::complex<float>> spectral);

    void Hilbert(std::span<const float> time, std::span<std::complex<float>> output);

    void Hilbert(std::span<const float> time, std::span<float> real, std::span<float> imag);

    size_t NumBins() const {
        return fft_size_;
    }

    size_t FFTSize() const {
        return fft_size_;
    }

    float FFTSizeFloat() const {
        return static_cast<float>(fft_size_);
    }
private:
    friend struct ComplexFFTHelper;

    size_t fft_size_{};
    std::vector<int> ip_;
    std::vector<float> w_;
    std::vector<float> buffer_;
};
}