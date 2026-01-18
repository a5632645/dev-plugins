#pragma once
#include <array>
#include <vector>

#include <qwqdsp/simd_element/simd_pack.hpp>
#include <qwqdsp/spectral/complex_fft.hpp>
#include "AudioFFT/AudioFFT.h"

namespace green_vocoder::dsp {

class STFTVocoder {
public:
    static constexpr size_t kExtraGainSize = 1;

    void Init(float fs);
    void Process(qwqdsp_simd_element::PackFloat<2>* main, qwqdsp_simd_element::PackFloat<2>* side, size_t num_samples);

    void SetBandwidth(float bw);
    void SetRelease(float ms);
    void SetAttack(float ms);
    void SetBlend(float blend);
    void SetFFTSize(size_t size);
    void SetFormantShift(float formant_shift);

    void SetUseV2(bool use);
    void SetDetail(float detail);

    size_t GetFFTSize() const {
        return fft_size_;
    }

    std::vector<float> gains_{};
    std::vector<float> gains2_{};
private:
    float Blend(float x);
    void SpectralProcess(std::vector<float>& real_in, std::vector<float>& imag_in,
                         std::vector<float>& real_out, std::vector<float>& imag_out,
                         std::vector<float>& gains);
    void SpectralProcess2(std::vector<float>& real_in, std::vector<float>& imag_in,
                          std::vector<float>& real_out, std::vector<float>& imag_out,
                          std::vector<float>& gains);

    audiofft::AudioFFT fft_;
    std::vector<float> window_{};
    std::vector<float> hann_window_{};
    std::vector<float> temp_main_{};
    std::vector<float> temp_side_{};
    std::vector<float> real_main_{};
    std::vector<float> real_side_{};
    std::vector<float> imag_main_{};
    std::vector<float> imag_side_{};
    
    std::array<qwqdsp_simd_element::PackFloat<2>, 32768> main_inputBuffer_{};
    std::array<qwqdsp_simd_element::PackFloat<2>, 32768> side_inputBuffer_{};
    std::array<qwqdsp_simd_element::PackFloat<2>, 32768> main_outputBuffer_{};
    size_t fft_size_{};
    size_t hop_size_{};
    size_t numInput_{};
    size_t writeEnd_{};
    size_t writeAddBegin_{};
    float bandwidth_{};
    float decay_{};
    float attck_{};
    float sample_rate_{};
    float blend_{};
    float window_gain_{};
    float release_ms_{};
    float attack_ms_{};
    float formant_mul_{};

    // v2 cepstrum processing
    bool use_v2_{};
    float detail_{};
    std::vector<float> temp_;
    std::vector<float> re1_;
    std::vector<float> phase_;
    qwqdsp_spectral::ComplexFFT cep_fft_;
    std::vector<float> cep_window_{};
    std::vector<float> cep_window_fft_{};
};

} // namespace green_vocoder::dsp
