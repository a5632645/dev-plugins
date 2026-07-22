#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <span>

#include <qwqdsp/oscillator/blep_coeff.hpp>
#include <qwqdsp/oscillator/noise.hpp>
#include <qwqdsp/oscillator/polyblep.hpp>
#include <qwqdsp/pitch/pitch.hpp>
#include <qwqdsp/pitch/yin.hpp>

namespace green_vocoder::dsp {

class PitchOsc {
public:
    static constexpr int kBlockSize = 1024;
    static constexpr int kHopSize = 1024;

    void Init(float fs) noexcept {
        fs_ = fs;
        yin_.Init(fs, kBlockSize);
        yin_.SetThreshold(0.2f);
    }

    void Reset() noexcept {
        in_pos_ = 0;
        glide_y_ = 0.0f;
        gain_remain_ = 0;
        curr_osc_gain_ = 0.0f;
        curr_noise_gain_ = 0.0f;
    }

    void SetGlide(float ms) noexcept {
        glide_a_ = 1.0f - std::exp(-1.0f / (ms * 0.001f * fs_));
    }

    void SetPitchShift(float semitones) noexcept {
        frequency_mul_ = std::exp2(semitones / 12.0f);
    }

    void SetWaveform(int idx) noexcept {
        waveform_ = idx;
    }

    void SetNoiseGain(float gain) noexcept {
        noise_gain_ = gain;
    }

    void SetPWM(float width) noexcept {
        osc_.SetPWM(width);
    }

    void SetMinPitch(float hz) noexcept {
        yin_.SetMinPitch(hz);
    }

    void SetMaxPitch(float hz) noexcept {
        yin_.SetMaxPitch(hz);
    }

    /// Read pitch from `buffer` (mono), write oscillator output to `buffer`.
    void Process(float* buffer, int num_frame) noexcept {
        while (num_frame != 0) {
            int need = kBlockSize - in_pos_;
            need = std::min(need, num_frame);
            std::copy_n(buffer, need, in_buffer_.begin() + in_pos_);
            in_pos_ += need;

            if (in_pos_ == kBlockSize) ProcessFrame();

            FillAudio(buffer, need);
            num_frame -= need;
            buffer += need;
        }
    }
private:
    void ProcessFrame() noexcept {
        yin_.Process(std::span<const float>{in_buffer_.data(), static_cast<size_t>(kBlockSize)});
        in_pos_ = 0;
        latest_pitch_ = yin_.GetPitch();

        // 当yin没有找到小于阈值将返回最小值
        // 这个最小值很可能是unvoice导致pitch跳动
        // 所以小于阈值时认为pitch不正确
        // float target_osc = 1.0f - latest_pitch_.non_period_ratio;
        // float target_noise = latest_pitch_.non_period_ratio * noise_gain_;
        float target_osc = 1.0f;
        float target_noise = 0.0f;
        if (latest_pitch_.non_period_ratio >= 0.2f) {
            target_osc = 0.0f;
            target_noise = 1.0f;
        }
        target_noise *= noise_gain_;

        osc_delta_ = (target_osc - curr_osc_gain_) / static_cast<float>(kHopSize);
        noise_delta_ = (target_noise - curr_noise_gain_) / static_cast<float>(kHopSize);
        gain_remain_ = kHopSize;
    }

    void FillAudio(float* buffer, int need) noexcept {
        float target_hz = std::max(latest_pitch_.pitch_hz * frequency_mul_, 0.1f);

        for (int i = 0; i < need; ++i) {
            if (gain_remain_ > 0) {
                curr_osc_gain_ += osc_delta_;
                curr_noise_gain_ += noise_delta_;
                --gain_remain_;
            }

            glide_y_ += glide_a_ * (target_hz - glide_y_);
            osc_.SetFreq(glide_y_, fs_);
            float s = (waveform_ == 0 ? osc_.Sawtooth() : osc_.PWM_NoDC()) * curr_osc_gain_;
            buffer[i] = s + noise_.Next() * curr_noise_gain_;
        }
    }

    // -- input --
    std::array<float, kBlockSize> in_buffer_{};
    int in_pos_{};

    // -- latest pitch from yin --
    qwqdsp_pitch::Pitch latest_pitch_{};

    // -- interpolation state --
    int gain_remain_{};
    float osc_delta_{};
    float noise_delta_{};
    float curr_osc_gain_{};
    float curr_noise_gain_{};

    // -- parameters --
    float fs_{};
    float frequency_mul_{1.0f};
    float noise_gain_{0.5f};
    int waveform_{};

    // -- glide (一阶指数平滑) --
    float glide_a_{};
    float glide_y_{};

    // -- DSP --
    qwqdsp_oscillator::PolyBlep<qwqdsp_oscillator::blep_coeff::Triangle> osc_;
    qwqdsp_oscillator::WhiteNoise noise_;
    qwqdsp_pitch::Yin yin_;
};

} // namespace green_vocoder::dsp
