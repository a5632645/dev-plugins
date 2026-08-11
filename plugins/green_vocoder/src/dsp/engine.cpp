#include "engine.hpp"

#include <cmath>
#include <numbers>
#include <span>

// 立体声调试开关：启用后左声道（ch0）作为 mod、右声道（ch1）作为 carry，
// 两者均以单声道复制到 L/R，绕过 pitch 跟踪与 sidechain 路由。
// 需要时取消下行注释开启（默认关闭）。
#define I_AM_USING_STEREO_DEBUG

namespace {

// OLA 输出重建增益（块 Burg）：无分析窗，仅 hann 合成窗重叠相加（hop=N/4），
// COLA 常数=Σhann=2。
// STFT 的纯 WOLA 重建增益（=1/Σ_k w_a·w_s，随 fft_size 变）在 InitOla 里由实际窗
// 计算存于 stft_wola_gain_：分析窗归一化后 Σ=2，hann² 的 COLA=1.5 → 增益=N/6。
constexpr float kBlockBurgGain = 0.25f;

} // namespace

namespace green_vocoder {

void Engine::Init(double sample_rate, int block_size) {
    sample_rate_ = sample_rate;
    float fs = static_cast<float>(sample_rate);

    // 先初始化 STFT（重建归一化分析窗），再初始化共享 OLA，使 InitOla 能正确计算 WOLA 增益
    stft_.Init(fs);
    InitOla(1024);
    burg_lpc_.Init(fs, block_size);
    channel_vocoder_.Init(fs, block_size);
    block_burg_.Init(fs);
    cepstrum_stft_.Init(stft_);
    mfcc_stft_.Init(stft_);
    smooth_stft_.Init(stft_);
    welch_stft_.Init(stft_);
    morph_stft_.Init(stft_);
    wiener_stft_.Init(stft_);
    stft_mode_ = dsp::STFTMode::Cepstrum;

    pitch_osc_.Init(fs);
    pitch_osc_.Reset();

    pre_tilt_filter_.Init(fs);
    pre_tilt_filter_.Reset();
}

void Engine::Reset() {}

void Engine::UpdateStftWolaGain() {
    // 纯 WOLA 重建增益 = 1/Σ_k(w_a·w_s)：由归一化分析窗 × 普通 hann 合成窗计算
    // （hop=block/4，稳态任一点被 4 帧覆盖；hann² 的 COLA=1.5 → 增益=block_size/6）
    if (stft_.hann_window_.size() != static_cast<size_t>(ola_block_size_))
        return; // 分析窗未就绪或尺寸不匹配，保留旧值
    int const hop = ola_block_size_ / 4;
    double cola = 0.0;
    for (int k = 0; k < 4; ++k)
        cola += static_cast<double>(stft_.hann_window_[static_cast<size_t>(k * hop)]
                                    * ola_window_[static_cast<size_t>(k * hop)]);
    stft_wola_gain_ = static_cast<float>(1.0 / cola);
}

void Engine::InitOla(int block_size) {
    if (block_size == ola_block_size_)
        return;
    ola_block_size_ = block_size;
    ola_window_.resize(static_cast<size_t>(block_size));
    for (int i = 0; i < block_size; ++i) {
        ola_window_[static_cast<size_t>(i)] =
            0.5f
            - 0.5f
                  * std::cos(2.0f * std::numbers::pi_v<float> * static_cast<float>(i) / static_cast<float>(block_size));
    }
    ola_.Init(block_size, block_size / 4, ola_window_);
    UpdateStftWolaGain();
}

void Engine::Update(Params& p) {
    // 从参数读取当前 vocoder 类型（供本函数选择性更新与 Process 复用）
    vocoder_type_ = static_cast<eVocoderType>(p.vocoder_type.Get());

    // 通用模块（与声码器类型无关），始终按标志更新
    if (p.should_update_tilt_.exchange(false, std::memory_order_acq_rel))
        pre_tilt_filter_.SetParam({.db = p.pre_tilt.Get()});

    if (p.should_update_pitch_osc_.exchange(false, std::memory_order_acq_rel))
        pitch_osc_.SetParam({
            .min_pitch = p.track_low.Get(),
            .max_pitch = p.track_high.Get(),
            .pitch_shift = p.track_pitch.Get(),
            .pwm = p.track_pwm.Get(),
            .noise_gain = p.track_noise.Get(),
            .waveform = p.track_waveform.Get(),
            .glide = p.track_glide.Get(),
        });

    // 仅更新当前激活的 vocoder；其余标志保留，待切换到该 vocoder 时再同步
    switch (vocoder_type_) {
        case eVocoderType_LeakyBurgLPC:
            if (p.should_update_leaky_lpc_.exchange(false, std::memory_order_acq_rel))
                burg_lpc_.SetParam({
                    .forget = p.lpc_forget.Get(),
                    .smooth = p.lpc_smooth.Get(),
                    .order = static_cast<int>(p.lpc_order.Get()),
                    .gain_attack = p.lpc_gain_attack.Get(),
                    .gain_release = p.lpc_gain_release.Get(),
                    .gain_hold = p.lpc_gain_hold.Get(),
                    .formant_shift = p.shift_pitch.Get() * (16.0f / 24.0f) / 24.0f,
                });
            break;

        case eVocoderType_BlockBurgLPC:
            if (p.should_update_block_lpc_.exchange(false, std::memory_order_acq_rel)) {
                int const block_size = global::kStftSizes[static_cast<size_t>(p.stft_size.Get())];
                block_burg_.SetParam({
                    .block_size = block_size,
                    .poles = static_cast<int>(p.lpc_order.Get()),
                    .smear = p.lpc_smooth.Get(),
                    .attack = p.lpc_gain_attack.Get(),
                    .formant_shift = p.shift_pitch.Get() * (16.0f / 24.0f) / 24.0f,
                });
                InitOla(block_size);
            }
            break;

        case eVocoderType_STFTVocoder:
            if (p.should_update_stft_.exchange(false, std::memory_order_acq_rel)) {
                int const fft_size = global::kStftSizes[static_cast<size_t>(p.stft_size.Get())];
                stft_.SetParam({
                    .attack = p.stft_attack.Get(),
                    .release = p.stft_release.Get(),
                    .fft_size = fft_size,
                    .blend = p.stft_blend.Get(),
                    .formant_shift = p.shift_pitch.Get() * (16.0f / 24.0f) / 24.0f,
                    .bandwidth = p.stft_bandwidth.Get(),
                });

                cepstrum_stft_.SetParam({.detail = p.stft_detail.Get()}, stft_);
                mfcc_stft_.SetParam({.num_mfcc = static_cast<int>(p.mfcc_nbands.Get())}, stft_);
                welch_stft_.SetParam(
                    {.welch_frames = static_cast<int>(std::round(p.stft_welch.Get())),
                     .floor_db = p.stft_floor.Get()},
                    stft_);
                smooth_stft_.SetParam(
                    {.type = p.stft_smooth_erb.Get() ? dsp::STFTSmooth::SmoothType::ERB
                                                     : dsp::STFTSmooth::SmoothType::OCT,
                     .amount = p.stft_smooth.Get()},
                    stft_);
                morph_stft_.SetParam(
                    {.morph = p.stft_morph.Get(), .direction_ab = p.stft_morph_ab.Get()}, stft_);
                wiener_stft_.SetParam(
                    {.variant = p.stft_wiener_variant.Get() ? dsp::STFTWiener::Variant::Standard
                                                             : dsp::STFTWiener::Variant::Difference,
                     .snr = p.stft_wiener_snr.Get(),
                     .direction_ab = p.stft_wiener_ab.Get()},
                    stft_);

                stft_mode_ = static_cast<dsp::STFTMode>(p.stft_type.Get());

                InitOla(fft_size);
                UpdateStftWolaGain(); // stft_.SetParam 刚重建分析窗，确保增益随新尺寸刷新（InitOla 可能 early-return）
            }
            break;

        case eVocoderType_ChannelVocoder:
            if (p.should_update_channel_vocoder_.exchange(false, std::memory_order_acq_rel))
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
            break;

        case eVocoderType_NumVocoderTypes:
        default:
            jassertfalse;
            break;
    }
}

void Engine::Process(juce::AudioBuffer<float>& buffer, int mod_ch, int carry_ch, int pitch_ch, bool use_pitch) {
    int const num_samples = buffer.getNumSamples();

#ifdef I_AM_USING_STEREO_DEBUG
    juce::ignoreUnused(mod_ch, carry_ch, pitch_ch, use_pitch);
#endif

    // --- block processing ---
    for (int pos = 0; pos < num_samples; pos += global::kBlockSize) {
        int const n = std::min(global::kBlockSize, num_samples - pos);

#ifdef I_AM_USING_STEREO_DEBUG
        // 调试：左声道（ch0）作为 mod，右声道（ch1）作为 carry（单声道复制到 L/R）
        {
            float const* m = buffer.getReadPointer(0) + pos;
            for (int i = 0; i < n; ++i)
                crossing_main_buffer_[static_cast<size_t>(i)] = {m[i], m[i]};
        }
        {
            float const* c = buffer.getReadPointer(1) + pos;
            for (int i = 0; i < n; ++i)
                crossing_side_buffer_[static_cast<size_t>(i)] = {c[i], c[i]};
        }
#else
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
        }
        else {
            float const* sl = buffer.getReadPointer(carry_ch) + pos;
            float const* sr = buffer.getReadPointer(carry_ch + 1) + pos;
            for (int i = 0; i < n; ++i)
                crossing_side_buffer_[static_cast<size_t>(i)] = {sl[i], sr[i]};
        }
#endif

        // pre-tilt filter
        for (int i = 0; i < n; ++i)
            crossing_main_buffer_[static_cast<size_t>(i)] =
                pre_tilt_filter_.Tick(crossing_main_buffer_[static_cast<size_t>(i)]);

        // vocoder
        {
            PackFloat2* const main = crossing_main_buffer_.data();
            PackFloat2* const side = crossing_side_buffer_.data();

            switch (vocoder_type_) {
                case eVocoderType_LeakyBurgLPC:
                    burg_lpc_.Process({main, static_cast<size_t>(n)}, {side, static_cast<size_t>(n)});
                    break;
                case eVocoderType_STFTVocoder:
                    switch (stft_mode_) {
                        case dsp::STFTMode::Standard:
                            ola_.Process(
                                main, side, n, stft_wola_gain_,
                                [this](std::span<dsp::PackFloat2 const> mf, std::span<dsp::PackFloat2 const> sf) {
                                    return stft_.Process(mf, sf, stft_.hann_sinc_window_, standard_stft_);
                                });
                            break;
                        case dsp::STFTMode::Cepstrum:
                            ola_.Process(
                                main, side, n, stft_wola_gain_,
                                [this](std::span<dsp::PackFloat2 const> mf, std::span<dsp::PackFloat2 const> sf) {
                                    return stft_.Process(mf, sf, stft_.hann_window_, cepstrum_stft_);
                                });
                            break;
                        case dsp::STFTMode::MFCC:
                            ola_.Process(
                                main, side, n, stft_wola_gain_,
                                [this](std::span<dsp::PackFloat2 const> mf, std::span<dsp::PackFloat2 const> sf) {
                                    return stft_.Process(mf, sf, stft_.hann_window_, mfcc_stft_);
                                });
                            break;
                        case dsp::STFTMode::Smooth:
                            ola_.Process(
                                main, side, n, stft_wola_gain_,
                                [this](std::span<dsp::PackFloat2 const> mf, std::span<dsp::PackFloat2 const> sf) {
                                    return stft_.Process(mf, sf, stft_.hann_window_, smooth_stft_);
                                });
                            break;
                        case dsp::STFTMode::Welch:
                            ola_.Process(
                                main, side, n, stft_wola_gain_,
                                [this](std::span<dsp::PackFloat2 const> mf, std::span<dsp::PackFloat2 const> sf) {
                                    return stft_.Process(mf, sf, stft_.hann_window_, welch_stft_);
                                });
                            break;
                        case dsp::STFTMode::Morph:
                            ola_.Process(
                                main, side, n, stft_wola_gain_,
                                [this](std::span<dsp::PackFloat2 const> mf, std::span<dsp::PackFloat2 const> sf) {
                                    return stft_.Process(mf, sf, stft_.hann_window_, morph_stft_);
                                });
                            break;
                        case dsp::STFTMode::Wiener:
                            ola_.Process(
                                main, side, n, stft_wola_gain_,
                                [this](std::span<dsp::PackFloat2 const> mf, std::span<dsp::PackFloat2 const> sf) {
                                    return stft_.Process(mf, sf, stft_.hann_window_, wiener_stft_);
                                });
                            break;
                    }
                    break;
                case eVocoderType_ChannelVocoder:
                    channel_vocoder_.Process(main, side, n);
                    break;
                case eVocoderType_BlockBurgLPC:
                    ola_.Process(main, side, n, kBlockBurgGain, block_burg_);
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
        if (vocoder_type_ == eVocoderType_STFTVocoder || vocoder_type_ == eVocoderType_BlockBurgLPC)
            new_latency = stft_.GetFFTSize();
        latency_ = new_latency;
    }
}

} // namespace green_vocoder
