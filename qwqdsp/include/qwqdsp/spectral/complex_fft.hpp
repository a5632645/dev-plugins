#pragma once
#include <cmath>
#include <cstddef>
#include <span>
#include <vector>
#include <complex>
#include <cassert>

namespace qwqdsp::spectral {
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
}

/**
 * @tparam kUseNegPiFirst true: 结果为-pi ~ pi，否则为0 ~ 2pi
 *           奈奎斯特     负频率        零            正频率
 *   true       0      1 ~ n/2-1      n/2        n/2+1 ~ n-1
 *   false     n/2     n/2+1 ~ n       0          1 ~ n/2-1
 */
template<bool kUseNegPiFirst>
class ComplexFFT {
public:
    void Init(size_t fft_size) {
        fft_size_ = fft_size;
        ip_.resize(2 + std::ceil(std::sqrt(fft_size / 2.0f)));
        w_.resize(fft_size / 2);
        buffer_.resize(fft_size * 2);
        const size_t size4 = fft_size / 2;
        internal::makewt(size4, ip_.data(), w_.data());
    }
    
    void FFT(std::span<const float> time, std::span<std::complex<float>> spectral) noexcept {
        assert(time.size() == fft_size_);
        assert(spectral.size() == NumBins());

        for (size_t i = 0; i < fft_size_; ++i) {
            buffer_[2 * i] = time[i];
            buffer_[2 * i + 1] = 0.0f;
        }
        internal::cdft(fft_size_ * 2, 1, buffer_.data(), ip_.data(), w_.data());
        if constexpr (kUseNegPiFirst) {
            for (size_t i = 0; i <= fft_size_ / 2; ++i) {
                size_t e = fft_size_ / 2 - i;
                spectral[i].real(buffer_[e * 2]);
                spectral[i].imag(buffer_[e * 2 + 1]);
            }
            for (size_t i = 0; i < fft_size_ / 2 - 1; ++i) {
                size_t e = fft_size_ - 1 - i;
                size_t a = fft_size_ / 2 + 1 + i;
                spectral[a].real(buffer_[e * 2]);
                spectral[a].imag(buffer_[e * 2 + 1]);
            }
        }
        else {
            spectral[0].real(buffer_[0]);
            spectral[0].imag(buffer_[1]);
            for (size_t i = 1; i < fft_size_; ++i) {
                spectral[fft_size_ - i].real(buffer_[i * 2]);
                spectral[fft_size_ - i].imag(buffer_[i * 2 + 1]);
            }
        }
    }

    void FFT(std::span<const std::complex<float>> time, std::span<std::complex<float>> spectral) noexcept {
        assert(time.size() == fft_size_);
        assert(spectral.size() == NumBins());

        for (size_t i = 0; i < fft_size_; ++i) {
            buffer_[2 * i] = time[i].real();
            buffer_[2 * i + 1] = time[i].imag();
        }
        internal::cdft(fft_size_ * 2, 1, buffer_.data(), ip_.data(), w_.data());
        if constexpr (kUseNegPiFirst) {
            for (size_t i = 0; i <= fft_size_ / 2; ++i) {
                size_t e = fft_size_ / 2 - i;
                spectral[i].real(buffer_[e * 2]);
                spectral[i].imag(buffer_[e * 2 + 1]);
            }
            for (size_t i = 0; i < fft_size_ / 2 - 1; ++i) {
                size_t e = fft_size_ - 1 - i;
                size_t a = fft_size_ / 2 + 1 + i;
                spectral[a].real(buffer_[e * 2]);
                spectral[a].imag(buffer_[e * 2 + 1]);
            }
        }
        else {
            spectral[0].real(buffer_[0]);
            spectral[0].imag(buffer_[1]);
            for (size_t i = 1; i < fft_size_; ++i) {
                spectral[fft_size_ - i].real(buffer_[i * 2]);
                spectral[fft_size_ - i].imag(buffer_[i * 2 + 1]);
            }
        }
    }

    void FFT(std::span<const float> time, std::span<float> real, std::span<float> imag) noexcept {
        assert(time.size() == fft_size_);
        assert(real.size() == NumBins());
        assert(imag.size() == NumBins());

        for (size_t i = 0; i < fft_size_; ++i) {
            buffer_[2 * i] = time[i];
            buffer_[2 * i + 1] = 0.0f;
        }
        internal::cdft(fft_size_ * 2, 1, buffer_.data(), ip_.data(), w_.data());
        if constexpr (kUseNegPiFirst) {
            for (size_t i = 0; i <= fft_size_ / 2; ++i) {
                size_t e = fft_size_ / 2 - i;
                real[i] = (buffer_[e * 2]);
                imag[i] = (buffer_[e * 2 + 1]);
            }
            for (size_t i = 0; i < fft_size_ / 2 - 1; ++i) {
                size_t e = fft_size_ - 1 - i;
                size_t a = fft_size_ / 2 + 1 + i;
                real[a] = (buffer_[e * 2]);
                imag[a] = (buffer_[e * 2 + 1]);
            }
        }
        else {
            real[0] = (buffer_[0]);
            imag[0] = (buffer_[1]);
            for (size_t i = 1; i < fft_size_; ++i) {
                real[fft_size_ - i] = (buffer_[i * 2]);
                imag[fft_size_ - i] = (buffer_[i * 2 + 1]);
            }
        }
    }

    void FFT(std::span<const std::complex<float>> time, std::span<float> real, std::span<float> imag) noexcept {
        assert(time.size() == fft_size_);
        assert(real.size() == NumBins());
        assert(imag.size() == NumBins());

        for (size_t i = 0; i < fft_size_; ++i) {
            buffer_[2 * i] = time[i].real();
            buffer_[2 * i + 1] = time[i].imag();
        }
        internal::cdft(fft_size_ * 2, 1, buffer_.data(), ip_.data(), w_.data());
        if constexpr (kUseNegPiFirst) {
            for (size_t i = 0; i <= fft_size_ / 2; ++i) {
                size_t e = fft_size_ / 2 - i;
                real[i] = (buffer_[e * 2]);
                imag[i] = (buffer_[e * 2 + 1]);
            }
            for (size_t i = 0; i < fft_size_ / 2 - 1; ++i) {
                size_t e = fft_size_ - 1 - i;
                size_t a = fft_size_ / 2 + 1 + i;
                real[a] = (buffer_[e * 2]);
                imag[a] = (buffer_[e * 2 + 1]);
            }
        }
        else {
            real[0] = (buffer_[0]);
            imag[0] = (buffer_[1]);
            for (size_t i = 1; i < fft_size_; ++i) {
                real[fft_size_ - i] = (buffer_[i * 2]);
                imag[fft_size_ - i] = (buffer_[i * 2 + 1]);
            }
        }
    }

    /**
     * @param phase 可选的，不需要请传入{}
     */
    void FFTGainPhase(std::span<const float> time, std::span<float> gain, std::span<float> phase = {}) noexcept {
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
        if constexpr (kUseNegPiFirst) {
            for (size_t i = 0; i <= fft_size_ / 2; ++i) {
                size_t e = fft_size_ / 2 - i;
                float real = (buffer_[e * 2]);
                float imag = (buffer_[e * 2 + 1]);
                gain[i] = std::sqrt(real * real + imag * imag);
                if (!phase.empty()) phase[i] = std::atan2(imag, real);
            }
            for (size_t i = 0; i < fft_size_ / 2 - 1; ++i) {
                size_t e = fft_size_ - 1 - i;
                size_t a = fft_size_ / 2 + 1 + i;
                float real = (buffer_[e * 2]);
                float imag = (buffer_[e * 2 + 1]);
                gain[a] = std::sqrt(real * real + imag * imag);
                if (!phase.empty()) phase[a] = std::atan2(imag, real);
            }
        }
        else {
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
    }

    template<class SPAN_TYPE>
    void IFFT(std::span<SPAN_TYPE> time, std::span<std::complex<float>> spectral) noexcept {
        if constexpr (kUseNegPiFirst) {
            for (size_t i = 0; i <= fft_size_ / 2; ++i) {
            size_t a = fft_size_ / 2 - i;
                buffer_[2 * a] = spectral[i].real();
                buffer_[2 * a + 1] = spectral[i].imag();
            }
            for (size_t i = 0; i < fft_size_ / 2 - 1; ++i) {
                size_t a = fft_size_ / 2 + 1 + i;
                size_t e = fft_size_ - 1 - i;
                buffer_[2 * a] = spectral[e].real();
                buffer_[2 * a + 1] = spectral[e].imag();
            }
        }
        else {
            buffer_[0] = spectral[0].real();
            buffer_[1] = spectral[1].imag();
            for (size_t i = 1; i < fft_size_; ++i) {
                buffer_[2 * i] = spectral[fft_size_ - i].real();
                buffer_[2 * i + 1] = spectral[fft_size_ - i].imag();
            }
        }
        internal::cdft(fft_size_ * 2, -1, buffer_.data(), ip_.data(), w_.data());
        const float gain = 1.0f / fft_size_;
        for (size_t i = 0; i < fft_size_; ++i) {
            if constexpr (std::is_same_v<SPAN_TYPE, std::complex<float>>) {
                time[i].real(buffer_[i * 2] * gain);
                time[i].imag(buffer_[i * 2 + 1] * gain);
            }
            else {
                time[i] = buffer_[i * 2] * gain;
            }
        }
    }


    template<class SPAN_TYPE>
    void IFFT(std::span<SPAN_TYPE> time, std::span<float> real, std::span<float> imag) noexcept {
        if constexpr (kUseNegPiFirst) {
            for (size_t i = 0; i <= fft_size_ / 2; ++i) {
            size_t a = fft_size_ / 2 - i;
                buffer_[2 * a] = real[i];
                buffer_[2 * a + 1] = imag[i];
            }
            for (size_t i = 0; i < fft_size_ / 2 - 1; ++i) {
                size_t a = fft_size_ / 2 + 1 + i;
                size_t e = fft_size_ - 1 - i;
                buffer_[2 * a] = real[e];
                buffer_[2 * a + 1] = imag[e];
            }
        }
        else {
            buffer_[0] = real[0];
            buffer_[1] = imag[1];
            for (size_t i = 1; i < fft_size_; ++i) {
                buffer_[2 * i] = real[fft_size_ - i];
                buffer_[2 * i + 1] = imag[fft_size_ - i];
            }
        }
        internal::cdft(fft_size_ * 2, -1, buffer_.data(), ip_.data(), w_.data());
        const float gain = 1.0f / fft_size_;
        for (size_t i = 0; i < fft_size_; ++i) {
            if constexpr (std::is_same_v<SPAN_TYPE, std::complex<float>>) {
                time[i].real(buffer_[i * 2] * gain);
                time[i].imag(buffer_[i * 2 + 1] * gain);
            }
            else {
                time[i] = buffer_[i * 2] * gain;
            }
        }
    }

    void IFFTGainPhase(std::span<float> time, std::span<float> gain, std::span<float> phase) noexcept {
        if constexpr (kUseNegPiFirst) {
            for (size_t i = 0; i <= fft_size_ / 2; ++i) {
            size_t a = fft_size_ / 2 - i;
                buffer_[2 * a] = gain[i] * std::cos(phase[i]);
                buffer_[2 * a + 1] = gain[i] * std::sin(phase[i]);
            }
            for (size_t i = 0; i < fft_size_ / 2 - 1; ++i) {
                size_t a = fft_size_ / 2 + 1 + i;
                size_t e = fft_size_ - 1 - i;
                buffer_[2 * a] = gain[e] * std::cos(phase[e]);
                buffer_[2 * a + 1] = gain[e] * std::cos(phase[e]);
            }
        }
        else {
            buffer_[0] = gain[0];
            buffer_[1] = gain[1];
            for (size_t i = 1; i < fft_size_; ++i) {
                buffer_[2 * i] = gain[fft_size_ - i] * std::cos(phase[fft_size_ - i]);
                buffer_[2 * i + 1] = gain[fft_size_ - i] * std::sin(phase[fft_size_ - i]);
            }
        }
        internal::cdft(fft_size_ * 2, -1, buffer_.data(), ip_.data(), w_.data());
        const float g = 1.0f / fft_size_;
        for (size_t i = 0; i < fft_size_; ++i) {
            time[i] = buffer_[i * 2] * g;
        }
    }

    void IFFTGainPhase(std::span<std::complex<float>> time, std::span<float> gain, std::span<float> phase) noexcept {
        if constexpr (kUseNegPiFirst) {
            for (size_t i = 0; i <= fft_size_ / 2; ++i) {
            size_t a = fft_size_ / 2 - i;
                buffer_[2 * a] = gain[i] * std::cos(phase[i]);
                buffer_[2 * a + 1] = gain[i] * std::sin(phase[i]);
            }
            for (size_t i = 0; i < fft_size_ / 2 - 1; ++i) {
                size_t a = fft_size_ / 2 + 1 + i;
                size_t e = fft_size_ - 1 - i;
                buffer_[2 * a] = gain[e] * std::cos(phase[e]);
                buffer_[2 * a + 1] = gain[e] * std::cos(phase[e]);
            }
        }
        else {
            buffer_[0] = gain[0];
            buffer_[1] = gain[1];
            for (size_t i = 1; i < fft_size_; ++i) {
                buffer_[2 * i] = gain[fft_size_ - i] * std::cos(phase[fft_size_ - i]);
                buffer_[2 * i + 1] = gain[fft_size_ - i] * std::sin(phase[fft_size_ - i]);
            }
        }
        internal::cdft(fft_size_ * 2, -1, buffer_.data(), ip_.data(), w_.data());
        const float g = 1.0f / fft_size_;
        for (size_t i = 0; i < fft_size_; ++i) {
            time[i].real(buffer_[i * 2] * g);
            time[i].imag(buffer_[i * 2 + 1] * g);
        }
    }

    void Hilbert(std::span<const float> time, std::span<std::complex<float>> output, bool clear_dc) noexcept {
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

    void Hilbert(std::span<const float> time, std::span<float> real, std::span<float> imag) noexcept {
        assert(time.size() == fft_size_);
        assert(real.size() == fft_size_);
        assert(imag.size() == fft_size_);

        for (size_t i = 0; i < fft_size_; ++i) {
            buffer_[2 * i] = time[i];
            buffer_[2 * i + 1] = 0.0f;
        }
        internal::cdft(fft_size_ * 2, 1, buffer_.data(), ip_.data(), w_.data());
        // Z[0] = X[0]
        buffer_[0] *= 0.5f;
        buffer_[1] *= 0.5f;
        // Z[N/2] = x[N/2]
        buffer_[fft_size_] *= 0.5f;
        buffer_[fft_size_ + 1] *= 0.5f;
        // Z[negative frequency] = 0
        for (size_t i = 1; i < fft_size_ / 2; ++i) {
            buffer_[2 * i] = 0.0f;
            buffer_[2 * i + 1] = 0.0f;
        }
        internal::cdft(fft_size_ * 2, -1, buffer_.data(), ip_.data(), w_.data());
        // Z[n] = 2 * X[n]
        const float gain = 2.0f / fft_size_;
        for (size_t i = 0; i < fft_size_; ++i) {
            real[i] = buffer_[i * 2] * gain;
            imag[i] = buffer_[i * 2 + 1] * gain;
        }
    }

    void Hilbert(std::span<const float> input, std::span<float> output90, bool clear_dc) noexcept {
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

    /**
     * @brief 0 ~ N ---> -N/2 ~ N/2
     */
    template<class SPAN_TYPE>
    void TimeDomainShift(std::span<SPAN_TYPE> block) noexcept {
        assert(block.size() == fft_size_);
        std::copy_n(block.begin(), fft_size_ / 2, buffer_.begin());
        for (size_t i = 0; i < fft_size_ / 2; ++i) {
            block[i] = block[i + fft_size_ / 2];
        }
        std::copy_n(buffer_.begin(), fft_size_ / 2, block.begin() + fft_size_ / 2);
    }

    size_t NumBins() const noexcept {
        return fft_size_;
    }

    static constexpr size_t NumBins(size_t fft_size) noexcept {
        return fft_size;
    }

    size_t FFTSize() const noexcept {
        return fft_size_;
    }

    float FFTSizeFloat() const noexcept {
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