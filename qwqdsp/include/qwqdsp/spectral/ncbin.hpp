#pragma once
#include "real_fft.hpp"
#include <cstddef>

namespace qwqdsp::spectral {
class NcBin {
public:
    void Init(size_t fft_size) {
        fft_.Init(fft_size);
        spectral_.resize(fft_.NumBins());
    }

    void Process(std::span<const float> time, std::span<float> output_gain) noexcept {
        fft_.FFT(time, spectral_);
        const float gain = 2.0f / fft_.FFTSizeFloat();
        for (int bin = 0; bin < fft_.FFTSize() / 2; ++bin) {
            auto& thisBin = spectral_[bin];
            auto& nextBin = spectral_[bin + 1];
            float ncSum = -(thisBin.real() * nextBin.real() + thisBin.imag() * nextBin.imag());
            if (ncSum < 0) {
                output_gain[bin] = 0.0f;
            }
            else {
                output_gain[bin] = std::sqrt(ncSum) * gain;
            }
        }
    }

    struct FrequencyInfo {
        float begin_bin;
        float end_bin;
        float center_bin;
    };
    FrequencyInfo GetFrequencyInfo(size_t idx) const noexcept {
        FrequencyInfo info{};
        info.begin_bin = idx;
        info.end_bin = idx + 1;
        info.center_bin = idx + 0.5f;
        return info;
    }

    size_t NumData() const noexcept {
        return fft_.NumBins() - 1;
    }

    static constexpr size_t NumData(size_t size) noexcept {
        return RealFFT::NumBins(size) - 1;
    }
private:
    RealFFT fft_;
    std::vector<std::complex<float>> spectral_;
};
}