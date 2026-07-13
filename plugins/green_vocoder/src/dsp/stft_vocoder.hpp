#pragma once
#include <array>
#include <vector>

#include <qwqdsp/simd_element/simd_pack.hpp>
#include <qwqdsp/spectral/complex_fft.hpp>
#include "AudioFFT.h"

namespace green_vocoder::dsp {

class STFTVocoder {
public:
    static constexpr int kExtraGainSize = 1;
    static constexpr int kMaxNumMfcc = 80;
    static constexpr int kMinNumMfcc = 8;

    enum class Mode {
        Standard,
        Cepstrum,
        MFCC,
    };

    void Init(float fs);
    void Reset();
    void Process(qwqdsp_simd_element::PackFloat<2>* main, qwqdsp_simd_element::PackFloat<2>* side, int num_samples);

    void SetAttack(float ms);
    void SetRelease(float ms);
    void SetBlend(float blend);
    void SetFFTSize(int size);
    void SetFormantShift(float formant_shift);
    void SetMode(Mode mode);

    int GetFFTSize() const {
        return fft_size_;
    }

    // standard
    void SetBandwidth(float bw);

    // cepstrum
    void SetDetail(float detail);

    // mfcc
    void SetNumMfcc(int num_mfcc);

    std::vector<float> gains_{};
    std::vector<float> gains2_{};

    std::array<float, kMaxNumMfcc> mfcc_gains_{};
    std::array<float, kMaxNumMfcc> mfcc_gains2_{};
private:
    float Blend(float x);
    void SpectralProcess_Standard(std::vector<float>& real_in, std::vector<float>& imag_in,
                                  std::vector<float>& real_out, std::vector<float>& imag_out,
                                  std::vector<float>& gains);
    void SpectralProcess_Cepstrum(std::vector<float>& real_in, std::vector<float>& imag_in,
                                  std::vector<float>& real_out, std::vector<float>& imag_out,
                                  std::vector<float>& gains);
    void SpectralProcess_MFCC(std::vector<float>& real_in, std::vector<float>& imag_in, std::vector<float>& real_out,
                              std::vector<float>& imag_out, std::array<float, kMaxNumMfcc>& gains);

    // common fft
    audiofft::AudioFFT fft_;
    std::vector<float> hann_window_{};
    std::vector<float> temp_main_{};
    std::vector<float> temp_side_{};
    std::vector<float> real_main_{};
    std::vector<float> real_side_{};
    std::vector<float> imag_main_{};
    std::vector<float> imag_side_{};
    std::vector<qwqdsp_simd_element::PackFloat<2>> main_inputBuffer_{};
    std::vector<qwqdsp_simd_element::PackFloat<2>> side_inputBuffer_{};
    std::vector<qwqdsp_simd_element::PackFloat<2>> main_outputBuffer_{};
    int numInput_{};
    int fft_size_{};
    int hop_size_{};
    int writeEnd_{};
    int writeAddBegin_{};
    Mode mode_{};

    // standard
    std::vector<float> window_{};
    float bandwidth_{};
    float decay_{};
    float attck_{};
    float sample_rate_{};
    float blend_{};
    float window_gain_{};
    float release_ms_{};
    float attack_ms_{};
    float formant_mul_{};

    // cepstrum processing
    float detail_{};
    float norm_detail_{};
    std::vector<float> temp_;
    std::vector<float> re1_;
    std::vector<float> phase_;
    qwqdsp_spectral::ComplexFFT cep_fft_;
    std::vector<float> cep_window_{};
    std::vector<float> cep_window_fft_{};

    // mfcc
    std::array<size_t, kMaxNumMfcc + 1> mfcc_indexs_{};
    int num_mfcc_{};
    std::vector<float> fill_gains_{};
};

} // namespace green_vocoder::dsp
