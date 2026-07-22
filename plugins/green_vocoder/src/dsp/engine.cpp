#include "engine.hpp"

namespace green_vocoder {

// ============================================================================
// Lifecycle
// ============================================================================

void Engine::Init(double sample_rate, size_t block_size) {
    sample_rate_ = sample_rate;
    float fs = static_cast<float>(sample_rate);

    burg_lpc_.Init(fs, block_size);
    stft_vocoder_.Init(fs);
    channel_vocoder_.Init(fs, block_size);
    block_burg_lpc_.Init(fs);

    pitch_osc_.Init(fs);
    pitch_osc_.Reset();
    first_init_ = true;

    pre_tilt_filter_.Reset();
}

void Engine::Reset() {
}

// ============================================================================
// Mailbox → DSP sync
// ============================================================================

void Engine::UpdateTiltFilter(const TiltFilterMailbox& mb) {
    pre_tilt_filter_.SetTilt(static_cast<float>(sample_rate_), mb.pre_tilt_db);
}

void Engine::UpdateLeakyLPC(const LeakyLpcMailbox& mb) {
    burg_lpc_.SetForget(mb.forget_rate);
    burg_lpc_.SetSmooth(mb.smooth);
    burg_lpc_.SetGainAttack(mb.gain_attack);
    burg_lpc_.SetGainHold(mb.gain_hold);
    burg_lpc_.SetGainRelease(mb.gain_release);
    burg_lpc_.SetLPCOrder(static_cast<int>(mb.order));
    float scaled = mb.formant_shift * (16.0f / 24.0f) / 24.0f;
    burg_lpc_.SetFormantShift(scaled);
}

void Engine::UpdateBlockLPC(const BlockLpcMailbox& mb) {
    block_burg_lpc_.SetSmear(mb.smear);
    block_burg_lpc_.SetAttack(mb.attack);
    block_burg_lpc_.SetPoles(static_cast<size_t>(mb.poles));
    block_burg_lpc_.SetBlockSize(mb.block_size);
    float scaled = mb.formant_shift * (16.0f / 24.0f) / 24.0f;
    block_burg_lpc_.SetFormantShift(scaled);
}

void Engine::UpdateSTFT(const STFTVocoderMailbox& mb) {
    stft_vocoder_.SetBandwidth(mb.bandwidth);
    stft_vocoder_.SetNumMfcc(static_cast<size_t>(mb.num_mfcc));
    stft_vocoder_.SetAttack(mb.attack);
    stft_vocoder_.SetRelease(mb.release);
    stft_vocoder_.SetBlend(mb.blend);
    stft_vocoder_.SetDetail(mb.detail);
    stft_vocoder_.SetFFTSize(mb.fft_size);
    stft_vocoder_.SetMode(static_cast<dsp::STFTVocoder::Mode>(mb.mode));
    stft_vocoder_.SetFormantShift(mb.formant_shift);
}

void Engine::UpdateChannelVocoder(const ChannelVocoderMailbox& mb) {
    channel_vocoder_.SetFilterBankMode(
        static_cast<dsp::ChannelVocoder::FilterBankMode>(mb.filter_bank_mode));
    channel_vocoder_.SetAttack(mb.attack);
    channel_vocoder_.SetGate(mb.gate);
    channel_vocoder_.SetRelease(mb.release);
    channel_vocoder_.SetFreqBegin(mb.freq_begin);
    channel_vocoder_.SetFreqEnd(mb.freq_end);
    channel_vocoder_.SetNumBands(mb.nbands);
    channel_vocoder_.SetModulatorScale(mb.scale);
    channel_vocoder_.SetFilterRipple(mb.ripple);
    channel_vocoder_.SetCarryScale(mb.carry_scale);
    channel_vocoder_.SetMap(static_cast<dsp::eChannelVocoderMap>(mb.map));
    channel_vocoder_.SetFormantShift(mb.formant_shift);
}

void Engine::UpdatePitchOsc(const PitchOscMailbox& mb) {
    pitch_osc_.SetMinPitch(mb.min_pitch);
    pitch_osc_.SetMaxPitch(mb.max_pitch);
    pitch_osc_.SetPitchShift(mb.pitch_shift);
    pitch_osc_.SetPWM(mb.pwm);
    pitch_osc_.SetNoiseGain(mb.noise_gain);
    pitch_osc_.SetWaveform(mb.waveform);
    pitch_osc_.SetGlide(mb.glide);
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
    size_t const num_samples = static_cast<size_t>(buffer.getNumSamples());

    // --- block processing ---
    for (size_t pos = 0; pos < num_samples; pos += kBlockSize) {
        size_t const n = std::min(kBlockSize, num_samples - pos);

        // fill modulator
        {
            float const* ml = buffer.getReadPointer(mod_ch) + pos;
            float const* mr = buffer.getReadPointer(mod_ch + 1) + pos;
            for (size_t i = 0; i < n; ++i)
                crossing_main_buffer_[i] = {ml[i], mr[i]};
        }

        // fill carrier (pitch tracking or direct channel pair)
        if (use_pitch) {
            std::array<float, 256> mono;
            float const* src = buffer.getReadPointer(pitch_ch) + pos;
            std::copy_n(src, n, mono.begin());
            pitch_osc_.Process(mono.data(), static_cast<int>(n));
            for (size_t i = 0; i < n; ++i)
                crossing_side_buffer_[i] = {mono[i], mono[i]};
        } else {
            float const* sl = buffer.getReadPointer(carry_ch) + pos;
            float const* sr = buffer.getReadPointer(carry_ch + 1) + pos;
            for (size_t i = 0; i < n; ++i)
                crossing_side_buffer_[i] = {sl[i], sr[i]};
        }

        // pre-tilt filter
        for (size_t i = 0; i < n; ++i)
            crossing_main_buffer_[i] = pre_tilt_filter_.Tick(crossing_main_buffer_[i]);

        // vocoder
        {
            if (last_vocoder_type_ != vocoder_type) last_vocoder_type_ = vocoder_type;

            switch (vocoder_type) {
                case eVocoderType_LeakyBurgLPC:
                    burg_lpc_.Process({crossing_main_buffer_.data(), n}, {crossing_side_buffer_.data(), n});
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
                default:
                    jassertfalse;
                    break;
            }
        }

        // write output
        {
            float* out_l = buffer.getWritePointer(0) + pos;
            float* out_r = buffer.getWritePointer(1) + pos;
            for (size_t i = 0; i < n; ++i) {
                out_l[i] = crossing_main_buffer_[i][0];
                out_r[i] = crossing_main_buffer_[i][1];
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
