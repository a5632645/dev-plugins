#pragma once
#include <array>
#include <vector>
#include <span>
#include <qwqdsp/simd_element/simd_pack.hpp>
#include "AudioFFT/AudioFFT.h"

namespace green_vocoder::dsp {

class BlockBurgLPC {
public:
    static constexpr size_t kMaxPoles = 80;

    void Init(float fs);
    void Process(
        qwqdsp_simd_element::PackFloat<2>* main,
        qwqdsp_simd_element::PackFloat<2>* side,
        size_t num_samples
    );
    void SetBlockSize(size_t size);
    void SetPoles(size_t poles);

    int GetOrder() const { return static_cast<int>(num_poles_); }
    void CopyLatticeCoeffient(std::span<float> buffer);
private:
    float Blend(float x);

    audiofft::AudioFFT fft_;
    std::vector<float> hann_window_{};
    std::array<qwqdsp_simd_element::PackFloat<2>, 32768> main_inputBuffer_{};
    std::array<qwqdsp_simd_element::PackFloat<2>, 32768> side_inputBuffer_{};
    std::array<qwqdsp_simd_element::PackFloat<2>, 32768> main_outputBuffer_{};
    std::vector<qwqdsp_simd_element::PackFloat<2>> eb_;
    std::vector<qwqdsp_simd_element::PackFloat<2>> x_;
    std::array<qwqdsp_simd_element::PackFloat<2>, kMaxPoles> latticek{};
    size_t fft_size_{};    
    size_t hop_size_{};
    size_t numInput_{};
    size_t writeEnd_{};
    size_t writeAddBegin_{};
    size_t num_poles_{};
};

}
