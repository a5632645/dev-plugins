#include "stft_vocoder.hpp"

namespace green_vocoder::dsp {

void STFTVocoder::Init(float fs) {
    stft_.Init(fs);
    cepstrum_.Init(stft_);
    mfcc_.Init(stft_);
    mode_ = Mode::Cepstrum;
}

void STFTVocoder::Reset() {
    stft_.Reset();
}

void STFTVocoder::SetParam(const Params& p) {
    stft_.SetParam({
        .attack = p.attack,
        .release = p.release,
        .fft_size = p.fft_size,
        .blend = p.blend,
        .formant_shift = p.formant_shift,
        .bandwidth = p.bandwidth,
    });

    cepstrum_.SetParam({.detail = p.detail}, stft_);
    mfcc_.SetParam({.num_mfcc = p.num_mfcc}, stft_);

    mode_ = p.mode;
}

void STFTVocoder::Process(qwqdsp_simd_element::PackFloat<2>* main, qwqdsp_simd_element::PackFloat<2>* side,
                          int num_samples) {
    // 仅 Standard 使用 sinc*hann 分析窗，其余用 hann
    switch (mode_) {
        case Mode::Standard:
            stft_.Process(main, side, num_samples, stft_.window_, standard_);
            break;
        case Mode::Cepstrum:
            stft_.Process(main, side, num_samples, stft_.hann_window_, cepstrum_);
            break;
        case Mode::MFCC:
            stft_.Process(main, side, num_samples, stft_.hann_window_, mfcc_);
            break;
    }
}

} // namespace green_vocoder::dsp
