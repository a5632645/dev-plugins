#include "engine.hpp"

#include <cmath>

namespace green_vocoder {

// ============================================================================
// Lifecycle
// ============================================================================

void Engine::Init(double sample_rate, int block_size) {
    sample_rate_ = sample_rate;
    float fs = static_cast<float>(sample_rate);

    burg_lpc_.Init(fs, block_size);
    stft_vocoder_.Init(fs);
    channel_vocoder_.Init(fs, block_size);
    block_burg_lpc_.Init(fs);

    pitch_osc_.Init(fs);
    pitch_osc_.Reset();
    first_init_ = true;

    pre_tilt_filter_.Init(fs);
    pre_tilt_filter_.Reset();
}

void Engine::Reset() {
}

// ============================================================================
// Params → DSP sync
// ============================================================================

void Engine::Update(Params& p) {
    if (p.should_update_tilt_.exchange(false, std::memory_order_acq_rel))
        UpdateTiltFilter(p);
    if (p.should_update_leaky_lpc_.exchange(false, std::memory_order_acq_rel))
        UpdateLeakyLPC(p);
    if (p.should_update_block_lpc_.exchange(false, std::memory_order_acq_rel))
        UpdateBlockLPC(p);
    if (p.should_update_stft_.exchange(false, std::memory_order_acq_rel))
        UpdateSTFT(p);
    if (p.should_update_channel_vocoder_.exchange(false, std::memory_order_acq_rel))
        UpdateChannelVocoder(p);
    if (p.should_update_pitch_osc_.exchange(false, std::memory_order_acq_rel))
        UpdatePitchOsc(p);
}

void Engine::UpdateTiltFilter(Params& p) {
    pre_tilt_filter_.SetParam({.db = p.pre_tilt.Get()});
}

void Engine::UpdateLeakyLPC(Params& p) {
    burg_lpc_.SetParam({
        .forget = p.lpc_forget.Get(),
        .smooth = p.lpc_smooth.Get(),
        .order = static_cast<int>(p.lpc_order.Get()),
        .gain_attack = p.lpc_gain_attack.Get(),
        .gain_release = p.lpc_gain_release.Get(),
        .gain_hold = p.lpc_gain_hold.Get(),
        .formant_shift = p.shift_pitch.Get() * (16.0f / 24.0f) / 24.0f,
    });
}

void Engine::UpdateBlockLPC(Params& p) {
    block_burg_lpc_.SetParam({
        .block_size = global::kStftSizes[static_cast<size_t>(p.stft_size.Get())],
        .poles = static_cast<int>(p.lpc_order.Get()),
        .smear = p.lpc_smooth.Get(),
        .attack = p.lpc_gain_attack.Get(),
        .formant_shift = p.shift_pitch.Get() * (16.0f / 24.0f) / 24.0f,
    });
}

void Engine::UpdateSTFT(Params& p) {
    stft_vocoder_.SetParam({
        .attack = p.stft_attack.Get(),
        .release = p.stft_release.Get(),
        .fft_size = global::kStftSizes[static_cast<size_t>(p.stft_size.Get())],
        .blend = p.stft_blend.Get(),
        .formant_shift = p.shift_pitch.Get() * (16.0f / 24.0f) / 24.0f,
        .mode = static_cast<dsp::STFTVocoder::Mode>(p.stft_type.Get()),
        .bandwidth = p.stft_bandwidth.Get(),
        .detail = p.stft_detail.Get(),
        .num_mfcc = static_cast<int>(p.mfcc_nbands.Get()),
    });
}

void Engine::UpdateChannelVocoder(Params& p) {
    channel_vocoder_.SetParam({
        .num_bands = static_cast<int>(std::round(p.cv_nbands.Get())),
        .freq_begin = p.cv_freq_begin.Get(),
        .freq_end = p.cv_freq_end.Get(),
        .attack = p.cv_attack.Get(),
        .release = p.cv_release.Get(),
        .modulator_scale = p.cv_scale.Get(),
        .carry_scale = p.cv_carry_scale.Get(),
        .map = static_cast<dsp::eChannelVocoderMap>(p.cv_map.Get()),
        .filter_bank_mode = static_cast<dsp::ChannelVocoder::FilterBankMode>(p.cv_filter_bank_mode.Get()),
        .gate = p.cv_gate.Get(),
        .formant_shift = p.shift_pitch.Get() * (16.0f / 24.0f) / 24.0f,
        .ripple = p.cv_ripple.Get(),
    });
}

void Engine::UpdatePitchOsc(Params& p) {
    pitch_osc_.SetParam({
        .min_pitch = p.track_low.Get(),
        .max_pitch = p.track_high.Get(),
        .pitch_shift = p.track_pitch.Get(),
        .pwm = p.track_pwm.Get(),
        .noise_gain = p.track_noise.Get(),
        .waveform = p.track_waveform.Get(),
        .glide = p.track_glide.Get(),
    });
}

// ============================================================================
// Main processing
// ============================================================================

void Engine::Process(
    juce::AudioBuffer<float>& buffer,
    int mod_ch, int carry_ch,
    int pitch_ch, bool use_pitch,
    eVocoderType vocoder_type
) {
    int const num_samples = buffer.getNumSamples();

    // --- block processing ---
    for (int pos = 0; pos < num_samples; pos += global::kBlockSize) {
        int const n = std::min(global::kBlockSize, num_samples - pos);

        // fill modulator
        {
            float const* ml = buffer.getReadPointer(mod_ch) + pos;
            float const* mr = buffer.getReadPointer(mod_ch + 1) + pos;
            for (int i = 0; i < n; ++i)
                crossing_main_buffer_[static_cast<size_t>(i)] = {ml[i], mr[i]};
        }

        // fill carrier (pitch tracking or direct channel pair)
        if (use_pitch) {
            std::array<float, 256> mono;
            float const* src = buffer.getReadPointer(pitch_ch) + pos;
            std::copy_n(src, n, mono.begin());
            pitch_osc_.Process(mono.data(), n);
            for (int i = 0; i < n; ++i)
                crossing_side_buffer_[static_cast<size_t>(i)] = {mono[static_cast<size_t>(i)],
                                                                 mono[static_cast<size_t>(i)]};
        } else {
            float const* sl = buffer.getReadPointer(carry_ch) + pos;
            float const* sr = buffer.getReadPointer(carry_ch + 1) + pos;
            for (int i = 0; i < n; ++i)
                crossing_side_buffer_[static_cast<size_t>(i)] = {sl[i], sr[i]};
        }

        // pre-tilt filter
        for (int i = 0; i < n; ++i)
            crossing_main_buffer_[static_cast<size_t>(i)] =
                pre_tilt_filter_.Tick(crossing_main_buffer_[static_cast<size_t>(i)]);

        // vocoder
        {
            if (last_vocoder_type_ != vocoder_type) last_vocoder_type_ = vocoder_type;

            switch (vocoder_type) {
                case eVocoderType_LeakyBurgLPC:
                    burg_lpc_.Process({crossing_main_buffer_.data(), static_cast<size_t>(n)},
                                      {crossing_side_buffer_.data(), static_cast<size_t>(n)});
                    break;
                case eVocoderType_STFTVocoder:
                    stft_vocoder_.Process(crossing_main_buffer_.data(), crossing_side_buffer_.data(), n);
                    break;
                case eVocoderType_ChannelVocoder:
                    channel_vocoder_.ProcessBlock(crossing_main_buffer_.data(), crossing_side_buffer_.data(), n);
                    break;
                case eVocoderType_BlockBurgLPC:
                    block_burg_lpc_.Process(crossing_main_buffer_.data(), crossing_side_buffer_.data(), n);
                    break;
                case eVocoderType_NumVocoderTypes:
                default:
                    jassertfalse;
                    break;
            }
        }

        // write output
        {
            float* out_l = buffer.getWritePointer(0) + pos;
            float* out_r = buffer.getWritePointer(1) + pos;
            for (int i = 0; i < n; ++i) {
                out_l[i] = crossing_main_buffer_[static_cast<size_t>(i)][0];
                out_r[i] = crossing_main_buffer_[static_cast<size_t>(i)][1];
            }
        }
    }

    // latency
    {
        int new_latency = 0;
        if (vocoder_type == eVocoderType_STFTVocoder || vocoder_type == eVocoderType_BlockBurgLPC)
            new_latency = stft_vocoder_.GetFFTSize();
        latency_ = new_latency;
    }
}

} // namespace green_vocoder
