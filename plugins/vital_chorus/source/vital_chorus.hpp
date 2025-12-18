#pragma once
#include <array>
#include <span>

#include "qwqdsp/polymath.hpp"
#include "qwqdsp/fx/delay_line_simd.hpp"
#include "qwqdsp/psimd/vec4.hpp"

using SimdType = qwqdsp_psimd::Vec4f32;

class ParalleOnePoleTPT {
public:
    void Reset() noexcept {
        lag_ = SimdType::FromSingle(0);
    }
    
    static float ComputeCoeff(float w) noexcept {
        constexpr float kMaxOmega = std::numbers::pi_v<float> - 1e-5f;
        [[unlikely]]
        if (w < 0.0f) {
            return 0.0f;
        }
        else if (w > kMaxOmega) {
            return 1.0f;
        }
        else [[likely]] {
            auto k = std::tan(w / 2);
            return k / (1 + k);
        }
    }

    SimdType TickLowpass(SimdType x, float coeff) noexcept {
        SimdType delta = SimdType::FromSingle(coeff) * (x - lag_);
        lag_ += delta;
        SimdType y = lag_;
        lag_ += delta;
        return y;
    }

    SimdType TickHighpass(SimdType x, float coeff) noexcept {
        SimdType delta = SimdType::FromSingle(coeff) * (x - lag_);
        lag_ += delta;
        SimdType y = lag_;
        lag_ += delta;
        return x - y;
    }
private:
    SimdType lag_{};
};

class VitalChorus {
public:
    static constexpr float kMaxModulationMs = 30;
    static constexpr float kMaxStaticDelayMs = 20;
    static constexpr float kMaxDelayMs = kMaxStaticDelayMs + kMaxModulationMs * 1.5f + 1;
    static constexpr size_t kMaxNumChorus = 16;

    void Init(float fs) {
        float const samples = fs * kMaxDelayMs / 1000.0f;
        for (auto& d : delays_) {
            d.Init(static_cast<size_t>(std::ceil(samples)));
        }
        fs_ = fs;
    }

    void Reset() noexcept {
        for (auto& d : delays_) {
            d.Reset();
        }
        for (auto& f : lowpass_) {
            f.Reset();
        }
        for (auto& f : highpass_) {
            f.Reset();
        }
    }

    void Process(std::span<float> left, std::span<float> right) noexcept {
        size_t const num_pairs = num_voices_ / SimdType::kSize;
        float const g = 1.0f / std::sqrt(static_cast<float>(num_pairs));
        size_t const num_samples = left.size();
        size_t offset = 0;
        float* left_ptr = left.data();
        float* right_ptr = right.data();

        std::array<SimdType, 256> temp_in;
        std::array<SimdType, 256> temp_out;

        while (offset != num_samples) {
            size_t const cando = std::min(256ull, num_samples - offset);
            float const delay_time_smooth_factor = 1.0f - std::exp(-1.0f / (fs_ / static_cast<float>(cando) * 20.0f / 1000.0f));

            // update delay time
            // you can see visualizer here https://www.desmos.com/calculator/5ytjkkqtbb?lang=zh-CN
            phase_ += phase_inc_ * static_cast<float>(cando);
            phase_ -= std::floor(phase_);
            float const avg_delay = (delay1 + delay2) * 0.5f;
            for (size_t i = 0; i < num_pairs; ++i) {
                SimdType static_a = SimdType{delay1, delay1, delay2, delay2};
                SimdType static_b = SimdType::FromSingle(avg_delay);
                float const lerp = static_cast<float>(i) / std::max(1.0f, static_cast<float>(num_pairs) - 1.0f);
                SimdType static_delay = static_a + SimdType::FromSingle(lerp) * (static_b - static_a);
                SimdType offsetp = SimdType{0.0f, 0.25f, 0.5f, 0.75f};
                SimdType sin_mod = SimdType::FromSingle(phase_) + offsetp + SimdType::FromSingle(0.25f * static_cast<float>(i) / static_cast<float>(num_pairs));
                sin_mod = sin_mod.Frac();
                for (size_t j = 0; j < 4; ++j) {
                    sin_mod.x[j] = qwqdsp::polymath::SinParabola(sin_mod.x[j] * 2 * std::numbers::pi_v<float> - std::numbers::pi_v<float>);
                }
                sin_mod = sin_mod * SimdType::FromSingle(0.5f) + SimdType::FromSingle(1.0f);
                SimdType delay_ms = SimdType::FromSingle(depth * kMaxModulationMs) * sin_mod + static_delay;
                delay_ms_[i] = delay_ms;

                // additional exp smooth
                SimdType target_delay_samples = delay_ms * SimdType::FromSingle(fs_ / 1000.0f);
                delay_samples_[i] += SimdType::FromSingle(delay_time_smooth_factor) * (target_delay_samples - delay_samples_[i]);
            }

            // shuffle
            for (size_t j = 0; j < cando; ++j) {
                temp_in[j].x[0] = *left_ptr;
                temp_in[j].x[1] = *right_ptr;
                temp_in[j].x[2] = *left_ptr;
                temp_in[j].x[3] = *right_ptr;
                temp_in[j] *= SimdType::FromSingle(0.5f);
                ++left_ptr;
                ++right_ptr;
            }

            // first 4 direct set
            float const inv_processing_samples = 1.0f / static_cast<float>(cando);

            SimdType curr_delay_samples = last_delay_samples_[0];
            SimdType delta_delay_samples = (delay_samples_[0] - last_delay_samples_[0]) * SimdType::FromSingle(inv_processing_samples);

            float const curr_feedback = last_feedback_;
            float const delta_feedback = (feedback - last_feedback_) * inv_processing_samples;
            SimdType vcurr_feedback = SimdType::FromSingle(curr_feedback);
            SimdType vdelta_feedback = SimdType::FromSingle(delta_feedback);

            float vcurr_lowpass = last_lowpass_coeff_;
            float vcurr_highpass = last_highpass_coeff_;
            float const delta_lowpass = (lowpass_coeff_ - last_lowpass_coeff_) * inv_processing_samples;
            float const delta_highpass = (highpass_coeff_ - last_highpass_coeff_) * inv_processing_samples;
            
            for (size_t j = 0; j < cando; ++j) {
                curr_delay_samples += delta_delay_samples;
                vcurr_feedback += vdelta_feedback;
                vcurr_lowpass += delta_lowpass;
                vcurr_highpass += delta_highpass;

                SimdType read = delays_[0].GetAfterPush(curr_delay_samples);
                SimdType write = temp_in[j] + read * vcurr_feedback;
                write = lowpass_[0].TickLowpass(write, vcurr_lowpass);
                write = highpass_[0].TickHighpass(write, vcurr_highpass);
                delays_[0].Push(write);
                temp_out[j] = read;
            }
            last_delay_samples_[0] = curr_delay_samples;

            // last add into
            for (size_t i = 1; i < num_pairs; ++i) {
                curr_delay_samples = last_delay_samples_[i];
                delta_delay_samples = (delay_samples_[i] - last_delay_samples_[i]) * SimdType::FromSingle(inv_processing_samples);

                vcurr_feedback = SimdType::FromSingle(curr_feedback);
                vdelta_feedback = SimdType::FromSingle(delta_feedback);

                vcurr_lowpass = last_lowpass_coeff_;
                vcurr_highpass = last_highpass_coeff_;
                
                for (size_t j = 0; j < cando; ++j) {
                    curr_delay_samples += delta_delay_samples;
                    vcurr_feedback += vdelta_feedback;
                    vcurr_lowpass += delta_lowpass;
                    vcurr_highpass += delta_highpass;

                    SimdType read = delays_[i].GetAfterPush(curr_delay_samples);
                    SimdType write = temp_in[j] + read * vcurr_feedback;
                    write = lowpass_[i].TickLowpass(write, vcurr_lowpass);
                    write = highpass_[i].TickHighpass(write, vcurr_highpass);
                    delays_[i].Push(write);
                    temp_out[j] += read;
                }
                last_delay_samples_[i] = curr_delay_samples;
            }

            // shuffle back
            left_ptr -= cando;
            right_ptr -= cando;
            float const dry = qwqdsp::polymath::CosPi(mix * std::numbers::pi_v<float> * 0.5f);
            float const wet = g * qwqdsp::polymath::SinPi(mix * std::numbers::pi_v<float> * 0.5f);
            SimdType curr_dry = SimdType::FromSingle(last_dry_);
            SimdType curr_wet = SimdType::FromSingle(last_wet_);
            SimdType delta_dry = SimdType::FromSingle((dry - last_dry_) * inv_processing_samples);
            SimdType delta_wet = SimdType::FromSingle((wet - last_wet_) * inv_processing_samples);
            for (size_t j = 0; j < cando; ++j) {
                curr_dry += delta_dry;
                curr_wet += delta_wet;
                SimdType t = curr_dry * temp_in[j] + curr_wet * temp_out[j];
                *left_ptr = t.x[0] + t.x[2];
                *right_ptr = t.x[1] + t.x[3];
                ++left_ptr;
                ++right_ptr;
            }

            last_feedback_ = feedback;
            last_dry_ = dry;
            last_wet_ = wet;
            last_lowpass_coeff_ = lowpass_coeff_;
            last_highpass_coeff_ = highpass_coeff_;

            offset += cando;
        }
    }

    void SyncLFOPhase(float phase) noexcept {
        phase_ = phase;
    }

    // -------------------- params --------------------
    float depth{};
    float delay1{};
    float delay2{};
    float feedback{};
    float mix{};
    void SetRate(float freq) noexcept {
        phase_inc_ = freq / fs_;
    }
    void SetFilter(float low_w, float high_w) noexcept {
        lowpass_coeff_ = ParalleOnePoleTPT::ComputeCoeff(low_w);
        highpass_coeff_ = ParalleOnePoleTPT::ComputeCoeff(high_w);
    }
    void SetNumVoices(size_t num_voices) noexcept {
        for (size_t i = num_voices_ / SimdType::kSize; i < num_voices / SimdType::kSize; ++i) {
            lowpass_[i].Reset();
            highpass_[i].Reset();
            delays_[i].Reset();
        }
        num_voices_ = num_voices;
    }

    // -------------------- lookup --------------------
    std::array<SimdType, kMaxNumChorus / SimdType::kSize> delay_ms_{};

private:
    float fs_{};
    float phase_{};
    float phase_inc_{};
    size_t num_voices_{};
    std::array<SimdType, kMaxNumChorus / SimdType::kSize> delay_samples_{};
    std::array<ParalleOnePoleTPT, kMaxNumChorus / SimdType::kSize> lowpass_;
    std::array<ParalleOnePoleTPT, kMaxNumChorus / SimdType::kSize> highpass_;
    std::array<qwqdsp_fx::DelayLineSIMD<SimdType, qwqdsp_fx::DelayLineInterpSIMD::PCHIP>, kMaxNumChorus> delays_;

    std::array<SimdType, kMaxNumChorus / SimdType::kSize> last_delay_samples_{};
    float last_feedback_{};
    float last_dry_{};
    float last_wet_{};

    // these are total pass
    float lowpass_coeff_{1.0f};
    float last_lowpass_coeff_{1.0f};
    float highpass_coeff_{0.0f};
    float last_highpass_coeff_{0.0f};
};
