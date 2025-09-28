#pragma once
#include <cstddef>
#include <span>
#include <vector>
#include <complex>
#include <cassert>

namespace qwqdsp::spectral {
/**
 * 默认输出 [0, 2pi]
 *
 *           奈奎斯特     负频率        零            正频率
 *   -pi ~ pi   0      1 ~ n/2-1      n/2        n/2+1 ~ n-1
 *              零       正频率      奈奎斯特         负频率
 *   0 ~ 2pi    0      1 ~ n/2-1      n/2        n/2+1 ~ n  
 */
class ComplexFFT {
public:
    #ifdef QWQDSP_HAVE_IPP
    ComplexFFT();
    ~ComplexFFT();
    #endif

    void Init(size_t fft_size);
    
    void FFT(std::span<const float> time, std::span<std::complex<float>> spectral) noexcept;

    void FFT(std::span<const std::complex<float>> time, std::span<std::complex<float>> spectral) noexcept;

    void FFT(std::span<const float> time, std::span<float> real, std::span<float> imag) noexcept;

    void FFT(std::span<const std::complex<float>> time, std::span<float> real, std::span<float> imag) noexcept;

    /**
     * @param phase 可选的，不需要请传入{}
     */
    void FFTGainPhase(std::span<const float> time, std::span<float> gain, std::span<float> phase = {}) noexcept;

    void FFTGainPhase(std::span<const std::complex<float>> time, std::span<float> gain, std::span<float> phase = {}) noexcept;

    void IFFT(std::span<float> time, std::span<const std::complex<float>> spectral) noexcept;

    void IFFT(std::span<std::complex<float>> time, std::span<const std::complex<float>> spectral) noexcept;

    void IFFT(std::span<float> time, std::span<const float> real, std::span<const float> imag) noexcept;

    void IFFT(std::span<std::complex<float>> time, std::span<const float> real, std::span<const float> imag) noexcept;
    
    void IFFTGainPhase(std::span<float> time, std::span<const float> gain, std::span<const float> phase) noexcept;

    void IFFTGainPhase(std::span<std::complex<float>> time, std::span<const float> gain, std::span<const float> phase) noexcept;

    void Hilbert(std::span<const float> time, std::span<std::complex<float>> output, bool clear_dc) noexcept;
    
    void Hilbert(std::span<const float> input, std::span<float> output90, bool clear_dc) noexcept;

    size_t NumBins() const noexcept {
        return fft_size_;
    }

    static constexpr size_t NumBins(size_t fft_size) noexcept {
        return fft_size;
    }

    size_t FFTSize() const noexcept {
        return fft_size_;
    }

    /**
     * [0, 2pi) ---> [-pi, pi) or [-pi, pi) -> [0, 2pi)
     */
    template<class TYPE>
    static void ShuffleFrequency(std::span<TYPE> buffer) {
        size_t const fft_size = buffer.size();
        size_t neg_idx = fft_size / 2;
        size_t pos_idx = 0;
        for (size_t i = 0; i < fft_size / 2; ++i) {
            std::swap(buffer[pos_idx], buffer[neg_idx]);
            ++pos_idx;
            ++neg_idx;
        }
    }

private:
    size_t fft_size_{};
    #ifndef QWQDSP_HAVE_IPP
    std::vector<int> ip_;
    std::vector<float> w_;
    std::vector<float> buffer_;
    #else
    unsigned char* p_spec_{};
    unsigned char* p_spec_buffer_{};
    unsigned char* p_buffer_{};
    unsigned char* p_fft_spec_{};
    std::vector<float> real_buffer_;
    std::vector<float> imag_buffer_;
    std::vector<float> src_imag_buffer_;
    std::vector<float> src_real_buffer_;
    #endif
};
}