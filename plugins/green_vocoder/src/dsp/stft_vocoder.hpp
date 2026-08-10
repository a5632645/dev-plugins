#pragma once
#include <array>
#include <vector>

#include <qwqdsp/simd_element/simd_pack.hpp>
#include <qwqdsp/spectral/complex_fft_adv.hpp>
#include <qwqdsp/spectral/real_fft_adv.hpp>

#include "../global.hpp"

namespace green_vocoder::dsp {

class STFTVocoder {
public:
    enum class Mode {
        Standard,
        Cepstrum,
        MFCC,
    };

    void Init(float fs);
    void Reset();
    void Process(qwqdsp_simd_element::PackFloat<2>* main, qwqdsp_simd_element::PackFloat<2>* side, int num_samples);

    struct Params {
        float attack{1.0f};
        float release{100.0f};
        int fft_size{1024};
        float blend{0.2f};
        float formant_shift{0.0f};
        Mode mode{Mode::Cepstrum};
        float bandwidth{2.0f};
        float detail{0.3f};
        int num_mfcc{20};
    };

    void SetParam(const Params& p);

    int GetFFTSize() const {
        return fft_size_;
    }

    // GUI 读取（保持公开）
    std::vector<float> gains_{};
    std::vector<float> gains2_{};

    std::array<float, global::kMaxNumMfcc> mfcc_gains_{};
    std::array<float, global::kMaxNumMfcc> mfcc_gains2_{};
private:
    float Blend(float x);
    void SpectralProcess_Standard(std::vector<float>& real_in, std::vector<float>& imag_in,
                                  std::vector<float>& real_out, std::vector<float>& imag_out,
                                  std::vector<float>& gains);
    void SpectralProcess_Cepstrum(std::vector<float>& real_in, std::vector<float>& imag_in,
                                  std::vector<float>& real_out, std::vector<float>& imag_out,
                                  std::vector<float>& gains);
    void SpectralProcess_MFCC(std::vector<float>& real_in, std::vector<float>& imag_in, std::vector<float>& real_out,
                              std::vector<float>& imag_out, std::array<float, global::kMaxNumMfcc>& gains);

    // common fft
    qwqdsp_spectral::RealFftAdv fft_;
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
    qwqdsp_spectral::ComplexFftAdv cep_fft_;
    std::vector<float> cep_window_{};
    std::vector<float> cep_window_fft_{};

    // mfcc
    std::array<size_t, global::kMaxNumMfcc + 1> mfcc_indexs_{};
    int num_mfcc_{};
    std::vector<float> fill_gains_{};
};

} // namespace green_vocoder::dsp
