#pragma once
#include <array>
#include <juce_audio_basics/juce_audio_basics.h>
#include <qwqdsp/simd_element/simd_pack.hpp>
#include <vector>

#include "block_burg_lpc.hpp"
#include "block_ola.hpp"
#include "channel_vocoder.hpp"
#include "leaky_burg_lpc.hpp"
#include "pitch_osc.hpp"
#include "stft/stft.hpp"
#include "stft/stft_cepstrum.hpp"
#include "stft/stft_mfcc.hpp"
#include "stft/stft_morph.hpp"
#include "stft/stft_smooth.hpp"
#include "stft/stft_standard.hpp"
#include "stft/stft_welch.hpp"
#include "tilt_filter.hpp"

#include "../global.hpp"
#include "../params.hpp"

namespace green_vocoder {

using PackFloat2 = qwqdsp_simd_element::PackFloat<2>;

class Engine {
public:
    void Init(double sample_rate, int block_size);
    void Reset();

    // --- Params → DSP sync (audio thread) ---
    // 单入口：从 Params 读取 vocoder_type 存入成员；通用模块按标志更新；
    // vocoder 模块仅更新当前激活的那个
    void Update(Params& p);

    // --- main processing ---
    // mod_ch / carry_ch / pitch_ch / use_pitch are pre-resolved by PluginProcessor
    // vocoder 类型复用成员 vocoder_type_（由 Update 从 Params 刷新）
    void Process(juce::AudioBuffer<float>& buffer, int mod_ch, int carry_ch, int pitch_ch, bool use_pitch);

    // --- latency ---
    int GetLatency() const {
        return latency_;
    }

    // --- GUI read access ---
    dsp::STFT& GetSTFT() {
        return stft_;
    }
    dsp::ChannelVocoder& GetChannelVocoder() {
        return channel_vocoder_;
    }
    dsp::LeakyBurgLPC& GetBurgLPC() {
        return burg_lpc_;
    }
    dsp::BlockBurgLPC& GetBlockBurgLPC() {
        return block_burg_;
    }
    double GetSampleRate() const {
        return sample_rate_;
    }
private:
    // 共享 OLA：按 block_size 重建（hann 合成窗，hop = block/4）
    void InitOla(int block_size);

    double sample_rate_{};
    dsp::TiltFilter pre_tilt_filter_;
    dsp::PitchOsc pitch_osc_;
    dsp::BlockOLA<PackFloat2> ola_;
    dsp::STFT stft_;
    dsp::STFTStandard standard_stft_;
    dsp::STFTCepstrum cepstrum_stft_;
    dsp::STFTMFCC mfcc_stft_;
    dsp::STFTSmooth smooth_stft_;
    dsp::STFTWelch welch_stft_;
    dsp::STFTMorph morph_stft_;
    dsp::BlockBurgLPC block_burg_;
    dsp::ChannelVocoder channel_vocoder_;
    dsp::LeakyBurgLPC burg_lpc_;
    std::vector<float> ola_window_;
    int ola_block_size_{};
    dsp::STFTMode stft_mode_{dsp::STFTMode::Cepstrum};

    std::array<PackFloat2, global::kBlockSize> crossing_main_buffer_;
    std::array<PackFloat2, global::kBlockSize> crossing_side_buffer_;

    int latency_{};
    eVocoderType vocoder_type_{eVocoderType_LeakyBurgLPC};
};

} // namespace green_vocoder
