#pragma once
#include <cstddef>
#include <span>
#include <vector>
#include <complex>

namespace qwqdsp::spectral {
class ComplexFFT {
public:
    void Init(size_t fft_size);
    
    void FFT(std::span<const float> time, std::span<std::complex<float>> spectral);

    void FFT(std::span<const std::complex<float>> time, std::span<std::complex<float>> spectral);

    void IFFT(std::span<std::complex<float>> time, std::span<std::complex<float>> spectral);

    size_t NumBins() const {
        return fft_size_;
    }
private:
    size_t fft_size_{};
    std::vector<int> ip_;
    std::vector<float> w_;
    std::vector<float> buffer_;
};
}