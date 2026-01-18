#pragma once
#include <array>
#include <vector>
#include <qwqdsp/simd_element/simd_pack.hpp>
#include "AudioFFT/AudioFFT.h"

namespace green_vocoder::dsp {

class MFCCVocoder {
public:
    static constexpr size_t kMaxNumMfcc = 80;
    static constexpr size_t kMinNumMfcc = 8;

    void Init(float fs);
    void Process(
        qwqdsp_simd_element::PackFloat<2>* main,
        qwqdsp_simd_element::PackFloat<2>* side,
        size_t num_samples
    );

    void SetRelease(float ms);
    void SetAttack(float ms);
    void SetFFTSize(size_t size);
    void SetNumMfcc(size_t num_mfcc);
    void SetFormantShift(float formant_shift);

    size_t GetFFTSize() const { return fft_size_; }

    std::array<float, kMaxNumMfcc> gains_{};
    std::array<float, kMaxNumMfcc> gains2_{};
private:
    void SpectralProcess(std::vector<float>& real_in, std::vector<float>& imag_in,
                         std::vector<float>& real_out, std::vector<float>& imag_out,
                         std::array<float, kMaxNumMfcc>& gains);
                         
    audiofft::AudioFFT fft_;
    std::vector<float> hann_window_{};
    std::vector<float> temp_main_{};
    std::vector<float> temp_side_{};
    std::vector<float> real_main_{};
    std::vector<float> real_side_{};
    std::vector<float> imag_main_{};
    std::vector<float> imag_side_{};
    std::vector<float> fill_gains_{};
    std::array<qwqdsp_simd_element::PackFloat<2>, 32768> main_inputBuffer_{};
    std::array<qwqdsp_simd_element::PackFloat<2>, 32768> side_inputBuffer_{};
    std::array<qwqdsp_simd_element::PackFloat<2>, 32768> main_outputBuffer_{};
    std::array<size_t, kMaxNumMfcc + 1> mfcc_indexs_{};
    size_t num_mfcc_{};
    size_t fft_size_{};
    size_t hop_size_{};
    size_t numInput_{};
    size_t writeEnd_{};
    size_t writeAddBegin_{};
    float decay_{};
    float attck_{};
    float sample_rate_{};
    float window_gain_{};
    float release_ms_{};
    float attack_ms_{};
    float formant_mul_{1};
};

}
