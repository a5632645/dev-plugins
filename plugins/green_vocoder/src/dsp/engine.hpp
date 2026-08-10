#pragma once
#include <array>
#include <juce_audio_basics/juce_audio_basics.h>
#include <qwqdsp/simd_element/simd_pack.hpp>

#include "block_burg_lpc.hpp"
#include "channel_vocoder.hpp"
#include "leaky_burg_lpc.hpp"
#include "pitch_osc.hpp"
#include "stft_vocoder.hpp"
#include "tilt_filter.hpp"

#include "../global.hpp"
#include "../params.hpp"

namespace green_vocoder {

class Engine {
public:
    void Init(double sample_rate, int block_size);
    void Reset();

    // --- Params → DSP sync (audio thread) ---
    // 单入口：内部根据 Params 中的 atomic bool 标志更新对应模块
    void Update(Params& p);

    // --- main processing ---
    // mod_ch / carry_ch / pitch_ch / use_pitch are pre-resolved by PluginProcessor
    void Process(
        juce::AudioBuffer<float>& buffer,
        int mod_ch, int carry_ch,
        int pitch_ch, bool use_pitch,
        eVocoderType vocoder_type
    );

    // --- latency ---
    int GetLatency() const { return latency_; }

    // --- GUI read access ---
    dsp::STFTVocoder& GetSTFT() { return stft_vocoder_; }
    dsp::ChannelVocoder& GetChannelVocoder() { return channel_vocoder_; }
    dsp::LeakyBurgLPC& GetBurgLPC() { return burg_lpc_; }
    dsp::BlockBurgLPC& GetBlockBurgLPC() { return block_burg_lpc_; }
    double GetSampleRate() const { return sample_rate_; }

private:
    // --- per-module 更新（由 Update 内部按标志调用） ---
    void UpdateTiltFilter(Params& p);
    void UpdateLeakyLPC(Params& p);
    void UpdateBlockLPC(Params& p);
    void UpdateSTFT(Params& p);
    void UpdateChannelVocoder(Params& p);
    void UpdatePitchOsc(Params& p);

    double sample_rate_{};
    dsp::TiltFilter pre_tilt_filter_;
    dsp::PitchOsc pitch_osc_;
    dsp::STFTVocoder stft_vocoder_;
    dsp::ChannelVocoder channel_vocoder_;
    dsp::LeakyBurgLPC burg_lpc_;
    dsp::BlockBurgLPC block_burg_lpc_;

    std::array<qwqdsp_simd_element::PackFloat<2>, global::kBlockSize> crossing_main_buffer_;
    std::array<qwqdsp_simd_element::PackFloat<2>, global::kBlockSize> crossing_side_buffer_;

    bool first_init_{};
    int latency_{};
    eVocoderType last_vocoder_type_{eVocoderType_LeakyBurgLPC};
};

} // namespace green_vocoder
