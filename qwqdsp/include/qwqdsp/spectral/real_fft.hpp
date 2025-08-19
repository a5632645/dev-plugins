#pragma once
#include <cstddef>
#include <span>
#include <vector>
#include <complex>

namespace qwqdsp::spectral {
class RealFFT {
public:
    void Init(size_t fft_size);
    
    void FFT(std::span<const float> time, std::span<std::complex<float>> spectral);

    void FFT(std::span<const float> time, std::span<float> real, std::span<float> imag);

    void IFFT(std::span<float> time, std::span<const std::complex<float>> spectral);

    void IFFT(std::span<float> time, std::span<const float> real, std::span<const float> imag);

    size_t NumBins() const {
        return fft_size_ / 2 + 1;
    }
private:
    size_t fft_size_{};
    std::vector<int> ip_;
    std::vector<float> w_;
    std::vector<float> buffer_;
};
}