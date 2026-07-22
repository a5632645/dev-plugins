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

#include "../param_mailboxes.hpp"
#include "../params.hpp"

namespace green_vocoder {

class Engine {
public:
    void Init(double sample_rate, size_t block_size);
    void Reset();

    // --- mailbox → DSP sync (audio thread) ---
    void UpdateTiltFilter(const TiltFilterMailbox& mb);
    void UpdateLeakyLPC(const LeakyLpcMailbox& mb);
    void UpdateBlockLPC(const BlockLpcMailbox& mb);
    void UpdateSTFT(const STFTVocoderMailbox& mb);
    void UpdateChannelVocoder(const ChannelVocoderMailbox& mb);
    void UpdatePitchOsc(const PitchOscMailbox& mb);

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
    double sample_rate_{};
    dsp::TiltFilter pre_tilt_filter_;
    dsp::PitchOsc pitch_osc_;
    dsp::STFTVocoder stft_vocoder_;
    dsp::ChannelVocoder channel_vocoder_;
    dsp::LeakyBurgLPC burg_lpc_;
    dsp::BlockBurgLPC block_burg_lpc_;

    std::array<qwqdsp_simd_element::PackFloat<2>, 256> crossing_main_buffer_;
    std::array<qwqdsp_simd_element::PackFloat<2>, 256> crossing_side_buffer_;

    bool first_init_{};
    int latency_{};
    eVocoderType last_vocoder_type_{eVocoderType_LeakyBurgLPC};

    static constexpr size_t kBlockSize = 256;
};

} // namespace green_vocoder
