#pragma once
#include <array>
#include <vector>

#include <qwqdsp/simd_element/simd_pack.hpp>

#include "stft/stft_cepstrum.hpp"
#include "stft/stft_mfcc.hpp"
#include "stft/stft_standard.hpp"
#include "../global.hpp"

namespace green_vocoder::dsp {

// ------------------------------------------------------------
// STFTVocoder：STFT 声码器调度门面
// ------------------------------------------------------------
// 持有 STFT 引擎与三种算法 functor（Standard / Cepstrum / MFCC），
// 按 Mode 用 switch 派发 Process（无虚函数）；gains 统一存于引擎。
class STFTVocoder {
public:
    enum class Mode {
        Standard,
        Cepstrum,
        MFCC,
    };

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

    void Init(float fs);
    void Reset();
    void SetParam(const Params& p);
    void Process(qwqdsp_simd_element::PackFloat<2>* main, qwqdsp_simd_element::PackFloat<2>* side, int num_samples);

    int GetFFTSize() const noexcept { return stft_.GetFFTSize(); }

    // GUI 读取（gains 统一存于 STFT 引擎）
    std::vector<float> const& GetGains() const noexcept { return stft_.gains_; }
    std::vector<float> const& GetGains2() const noexcept { return stft_.gains2_; }
    std::array<float, global::kMaxNumMfcc> const& GetMfccGains() const noexcept { return stft_.mfcc_gains_; }
    std::array<float, global::kMaxNumMfcc> const& GetMfccGains2() const noexcept { return stft_.mfcc_gains2_; }

private:
    STFT stft_;
    STFTStandard standard_;
    STFTCepstrum cepstrum_;
    STFTMFCC mfcc_;
    Mode mode_{Mode::Cepstrum};
};

} // namespace green_vocoder::dsp
