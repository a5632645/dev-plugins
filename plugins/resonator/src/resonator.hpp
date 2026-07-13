#pragma once
#include <complex>
#include <array>
#include <vector>

#include "config.hpp"

#include <qwqdsp/convert.hpp>
#include <qwqdsp/simd_element/simd_pack.hpp>
#include <qwqdsp/simd_element/one_pole_tpt.hpp>

using SimdType = qwqdsp_simd_element::PackFloat<4>;

class ThrianDispersion {
public:
    static constexpr size_t kNumAPF = 8;

    void Reset() noexcept {
        std::ranges::fill(lag1_, SimdType{});
        std::ranges::fill(lag2_, SimdType{});
    }

    SimdType Tick(SimdType x) noexcept {
        for (size_t i = 0; i < kNumAPF; ++i) {
            auto y = lag1_[i] + a1_ * (x - lag2_[i]);
            lag1_[i] = x;
            lag2_[i] = y;
            x = y;
        }
        return x;
    }

    void SetGroupDelay(SimdType const& delay) noexcept {
        SimdType temp = (SimdType::vBroadcast(1) - delay) / (SimdType::vBroadcast(1) + delay);
        SimdType zero = SimdType::vBroadcast(0);
        a1_ = SimdType{
            delay[0] < 1.0f ? zero[0] : temp[0],
            delay[1] < 1.0f ? zero[1] : temp[1],
            delay[2] < 1.0f ? zero[2] : temp[2],
            delay[3] < 1.0f ? zero[3] : temp[3]
        };
    }

    void SetGroupDelay(size_t idx, float delay) noexcept {
        if (delay < 1.0f) {
            a1_[idx] = 0.0f;
        }
        else {
            a1_[idx] = (1.0f - delay) / (1.0f + delay);
        }
    }

    SimdType GetPhaseDelay(SimdType const& w) const noexcept {
        SimdType r;
        for (size_t i= 0; i < SimdType::kSize; ++i) {
            auto z = std::polar(1.0f, w[i]);
            auto up = a1_[i] * z + 1.0f;
            auto down = z + a1_[i];
            float phase = std::arg(up / down);
            r[i] = -phase * kNumAPF / w[i];
        }
        return r;
    }

    float GetPhaseDelay(size_t idx, float w) const noexcept {
        auto z = std::polar(1.0f, w);
        auto up = a1_[idx] * z + 1.0f;
        auto down = z + a1_[idx];
        float phase = std::arg(up / down);
        return -phase * kNumAPF / w;
    }
private:
    SimdType a1_{};
    std::array<SimdType, kNumAPF> lag1_{};
    std::array<SimdType, kNumAPF> lag2_{};
};

// ---------------------------------------- delays ----------------------------------------
// one pole all pass filter
class TunningFilter {
public:
    SimdType Tick(SimdType const& in) noexcept {
        auto v = latch_;
        auto t = in - alpha_ * v;
        latch_ = t;
        return v + alpha_ * t;
    }

    void Reset() noexcept {
        latch_ = SimdType{};
    }
    
    /**
     * @brief 
     * @param delay 环路延迟
     * @return 还剩下多少延迟
     */
    qwqdsp_simd_element::PackUint32<4> SetDelay(SimdType const& delay) noexcept {
        qwqdsp_simd_element::PackUint32<4> r;
        for (size_t i = 0; i < SimdType::kSize; ++i) {
            // thiran delay limit to 0.5 ~ 1.5
            if (delay[i] < 0.5f) {
                alpha_[i] = 0.0f; // equal to one delay
                r[i] = 0;
            }
            else {
                float intergalPart = std::floor(delay[i]);
                float fractionalPart = delay[i] - intergalPart;
                if (fractionalPart < 0.5f) {
                    fractionalPart += 1.0f;
                    intergalPart -= 1.0f;
                }
                alpha_[i] = (1.0f - fractionalPart) / (1.0f + fractionalPart);
                r[i] = static_cast<uint32_t>(intergalPart);
            }
        }
        return r;
    }

    uint32_t SetDelay(size_t idx, float delay) noexcept {
        // thiran delay limit to 0.5 ~ 1.5
        if (delay < 0.5f) {
            alpha_[idx] = 0.0f; // equal to one delay
            return 0;
        }
        else {
            float intergalPart = std::floor(delay);
            float fractionalPart = delay - intergalPart;
            if (fractionalPart < 0.5f) {
                fractionalPart += 1.0f;
                intergalPart -= 1.0f;
            }
            alpha_[idx] = (1.0f - fractionalPart) / (1.0f + fractionalPart);
            return static_cast<uint32_t>(intergalPart);
        }
    }
private:
    SimdType latch_{};
    SimdType alpha_{};
};

class Resonator {
public:
    void Init(float fs, float min_pitch) {
        fs_ = fs;
        float const min_frequency = qwqdsp::convert::Pitch2Freq(min_pitch);
        float const max_seconds = 1.0f / min_frequency;
        size_t const max_samples = static_cast<size_t>(std::ceil(max_seconds * fs));

        size_t a = 1;
        while (a < max_samples) {
            a *= 2;
        }
        for (auto& v : delay_buffer_) {
            v.resize(a);
        }
        delay_mask_ = static_cast<uint32_t>(a - 1);
    }

    void Reset() noexcept {
        delay_wpos_ = 0;
        for (auto& v : delay_buffer_) {
            std::ranges::fill(v, SimdType{});
        }
        for (auto& v : thrian_interp_) {
            v.Reset();
        }
        for (auto& d : dispersion_) {
            d.Reset();
        }
        for (auto& d : damp_) {
            d.Reset();
        }
    }

    void Process(float* left_ptr, float* right_ptr, size_t len) noexcept {
        for (size_t i = 0; i < len; ++i) {
            // input
            auto output_l = SimdType::vBroadcast(left_ptr[i]);
            auto output_r = SimdType::vBroadcast(right_ptr[i]);
            auto input_1_l = input_volume_[0] * output_l + fb_values_[0];
            auto input_2_l = input_volume_[1] * output_l + fb_values_[1];
            auto input_1_r = input_volume_[0] * output_r + fb_values_[2];
            auto input_2_r = input_volume_[1] * output_r + fb_values_[3];
            output_l *= SimdType::vBroadcast(dry);
            output_r *= SimdType::vBroadcast(dry);
            output_l += fb_values_[0] * output_volume_[0];
            output_l += fb_values_[1] * output_volume_[1];
            output_r += fb_values_[2] * output_volume_[0];
            output_r += fb_values_[3] * output_volume_[1];

            // delayline
            delay_buffer_[0][delay_wpos_] = input_1_l;
            delay_buffer_[1][delay_wpos_] = input_2_l;
            delay_buffer_[2][delay_wpos_] = input_1_r;
            delay_buffer_[3][delay_wpos_] = input_2_r;
            delay_wpos_ = (delay_wpos_ + 1) & delay_mask_;
            auto delay_out1_l = ReadFeedback(0, delay_wpos_, 0);
            auto delay_out2_l = ReadFeedback(1, delay_wpos_, 1);
            auto delay_out1_r = ReadFeedback(2, delay_wpos_, 0);
            auto delay_out2_r = ReadFeedback(3, delay_wpos_, 1);

            // dispersion and damp
            delay_out1_l = dispersion_[0].Tick(delay_out1_l);
            delay_out2_l = dispersion_[1].Tick(delay_out2_l);
            delay_out1_r = dispersion_[2].Tick(delay_out1_r);
            delay_out2_r = dispersion_[3].Tick(delay_out2_r);
            delay_out1_l = damp_[0].TickHighshelf(delay_out1_l, damp_highshelf_coeff[0], damp_highshelf_gain[0]);
            delay_out2_l = damp_[1].TickHighshelf(delay_out2_l, damp_highshelf_coeff[1], damp_highshelf_gain[1]);
            delay_out1_r = damp_[2].TickHighshelf(delay_out1_r, damp_highshelf_coeff[0], damp_highshelf_gain[0]);
            delay_out2_r = damp_[3].TickHighshelf(delay_out2_r, damp_highshelf_coeff[1], damp_highshelf_gain[1]);
            delay_out1_l = dc_blocker[0].TickHighpass(delay_out1_l, SimdType::vBroadcast(0.0005f));
            delay_out2_l = dc_blocker[1].TickHighpass(delay_out2_l, SimdType::vBroadcast(0.0005f));
            delay_out1_r = dc_blocker[2].TickHighpass(delay_out1_r, SimdType::vBroadcast(0.0005f));
            delay_out2_r = dc_blocker[3].TickHighpass(delay_out2_r, SimdType::vBroadcast(0.0005f));

            // scatter signals
            // first
            SimdType scatter_sin = qwqdsp_simd_element::PackOps::Sin(reflections_[0]);
            SimdType scatter_cos = qwqdsp_simd_element::PackOps::Cos(reflections_[0]);
            SimdType scatter_a_l{
                delay_out1_l[0], delay_out1_l[2], delay_out2_l[0], delay_out2_l[2]
            };
            SimdType scatter_b_l{
                delay_out1_l[1], delay_out1_l[3], delay_out2_l[1], delay_out2_l[3]
            };
            SimdType scatter_a_r{
                delay_out1_r[0], delay_out1_r[2], delay_out2_r[0], delay_out2_r[2]
            };
            SimdType scatter_b_r{
                delay_out1_r[1], delay_out1_r[3], delay_out2_r[1], delay_out2_r[3]
            };
            // [0, 2, 4, 6]
            SimdType scatter_outa_l = scatter_cos * scatter_a_l - scatter_sin * scatter_b_l;
            SimdType scatter_outa_r = scatter_cos * scatter_a_r - scatter_sin * scatter_b_r;
            // [1, 3, 5, 7]
            SimdType scatter_outb_l = scatter_sin * scatter_a_l + scatter_cos * scatter_b_l;
            SimdType scatter_outb_r = scatter_sin * scatter_a_r + scatter_cos * scatter_b_r;

            // second
            scatter_sin = qwqdsp_simd_element::PackOps::Sin(reflections_[1]);
            scatter_cos = qwqdsp_simd_element::PackOps::Cos(reflections_[1]);
            SimdType scatter2_ina_l = scatter_outb_l;
            SimdType scatter2_inb_l = qwqdsp_simd_element::PackOps::Shuffle<1, 2, 3, 0>(scatter_outa_l);
            SimdType scatter2_ina_r = scatter_outb_r;
            SimdType scatter2_inb_r = qwqdsp_simd_element::PackOps::Shuffle<1, 2, 3, 0>(scatter_outa_r);
            // [1, 3, 5, 7]
            scatter_outa_l = scatter_cos * scatter2_ina_l - scatter_sin * scatter2_inb_l;
            scatter_outa_r = scatter_cos * scatter2_ina_r - scatter_sin * scatter2_inb_r;
            // [2, 4, 6, 0]
            scatter_outb_l = scatter_sin * scatter2_ina_l + scatter_cos * scatter2_inb_l;
            scatter_outb_r = scatter_sin * scatter2_ina_r + scatter_cos * scatter2_inb_r;

            // shuffle output
            SimdType pipe_a_l{
                scatter_outb_l[3], scatter_outa_l[0], scatter_outb_l[0], scatter_outa_l[1]
            };
            SimdType pipe_b_l{
                scatter_outb_l[1], scatter_outa_l[2], scatter_outb_l[2], scatter_outa_l[3]
            };

            SimdType pipe_a_r{
                scatter_outb_r[3], scatter_outa_r[0], scatter_outb_r[0], scatter_outa_r[1]
            };
            SimdType pipe_b_r{
                scatter_outb_r[1], scatter_outa_r[2], scatter_outb_r[2], scatter_outa_r[3]
            };
            
            // write
            fb_values_[0] = feedback_gain_[0] * pipe_a_l;
            fb_values_[1] = feedback_gain_[1] * pipe_b_l;
            fb_values_[2] = feedback_gain_[0] * pipe_a_r;
            fb_values_[3] = feedback_gain_[1] * pipe_b_r;
            left_ptr[i] = qwqdsp_simd_element::PackOps::ReduceAdd(output_l);
            right_ptr[i] = qwqdsp_simd_element::PackOps::ReduceAdd(output_r);
        }
    }

    void UpdateBasicParams() noexcept {
        // output mix volumes
        for (size_t j = 0; j < kMonoContainerSize; ++j) {
            for (size_t i = 0; i < SimdType::kSize; ++i) {
                size_t const param_idx = j * SimdType::kSize + i;
                float const db = mix_db[param_idx];
                if (db < -60.0f) {
                    output_volume_[j][i] = 0;
                }
                else {
                    output_volume_[j][i] = qwqdsp::convert::Db2Gain(db);
                }
            }
        }

        // update scatter matrix
        for (size_t j = 0; j < kMonoContainerSize; ++j) {
            for (size_t i = 0; i < SimdType::kSize; ++i) {
                size_t const param_idx = j * SimdType::kSize + i;
                reflections_[j][i] = norm_reflections[param_idx] * std::numbers::pi_v<float>;
            }
        }

        // update damp filter
        for (size_t j = 0; j < kMonoContainerSize; ++j) {
            SimdType omega;
            for (size_t i = 0; i < SimdType::kSize; ++i) {
                size_t const param_idx = j * SimdType::kSize + i;
                float const freq = qwqdsp::convert::Pitch2Freq(damp_pitch[param_idx]);
                omega[i] = freq * std::numbers::pi_v<float> * 2 / fs_;
                damp_highshelf_gain[j][i] = qwqdsp::convert::Db2Gain(damp_gain_db[param_idx]);
            }
            damp_highshelf_coeff[j] = qwqdsp_simd_element::OnePoleTPT<4>::ComputeCoeffs(omega);
        }
    }

    /**
    * @brief Update all pitches and associated parameters from internal pitch array.
    *
    * This function updates the following parameters for each resonator:
    * - Omega (angular frequency)
    * - Loop samples (sample period of the resonator)
    * - Allpass filter delay (group delay of the allpass filter)
    * - Feedback gain (gain of the feedback path)
    * - Fractional delay (sample period of the fractional delay filter)
    * - Allpass filter coefficients (coefficients of the allpass filter)
    * - Feedback gain (gain of the feedback path)
    */
    void UpdateAllPitches() noexcept {
        for (size_t i = 0; i < kMonoContainerSize; ++i) {
            SimdType omega;
            SimdType loop_samples;
            SimdType allpass_set_delay;
            for (size_t j = 0; j < SimdType::kSize; ++j) {
                size_t param_idx = i * SimdType::kSize + j;
                float pitch = pitches[param_idx] + fine_tune[param_idx] / 100.0f;
                if (polarity[param_idx]) {
                    pitch += 12;
                }
                float const freq = qwqdsp::convert::Pitch2Freq(pitch);
                omega[j] = freq * std::numbers::pi_v<float> / fs_;
                loop_samples[j] = fs_ / freq;
                allpass_set_delay[j] = loop_samples[j] * dispersion[param_idx] / (ThrianDispersion::kNumAPF + 0.1f);
            }
    
            // update allpass filters
            dispersion_[i].SetGroupDelay(allpass_set_delay);
            dispersion_[i + kMonoContainerSize].SetGroupDelay(allpass_set_delay);
            SimdType allpass_delay = dispersion_[i].GetPhaseDelay(omega);
    
            // remove allpass delays
            SimdType delay_samples = loop_samples - allpass_delay;
            delay_samples = qwqdsp_simd_element::PackOps::Max(delay_samples, SimdType::vBroadcast(0.0f));
    
            // process frac delays
            delay_samples_[i] = thrian_interp_[i].SetDelay(delay_samples);
            thrian_interp_[i + kMonoContainerSize].SetDelay(delay_samples);

            // feedback decay
            for (size_t j = 0; j < SimdType::kSize; ++j) {
                size_t param_idx = i * SimdType::kSize + j;
                float feedback_gain = 0;
                if (decay_ms[param_idx] > 0.5f) {
                    float const mul = -3.0f * loop_samples[j] / (fs_ * decay_ms[param_idx] / 1000.0f);
                    feedback_gain = std::pow(10.0f, mul);
                    feedback_gain = std::min(feedback_gain, 1.0f);
                }
                else {
                    feedback_gain = 0;
                }
                if (polarity[param_idx]) {
                    feedback_gain = -feedback_gain;
                }
                feedback_gain_[i][j] = feedback_gain;
            }
        }
    }

    void NoteOn(size_t idx, float pitch, float velocity) noexcept {
        size_t const simd_idx = idx / SimdType::kSize;
        size_t const scalar_idx = idx & (SimdType::kSize - 1);

        pitches[idx] = pitch;
        input_volume_[simd_idx][scalar_idx] = velocity;
        pitch = pitches[idx] + fine_tune[idx] / 100.0f;
        if (polarity[idx]) {
            pitch += 12;
        }
        float const freq = qwqdsp::convert::Pitch2Freq(pitch);
        float const omega = freq * std::numbers::pi_v<float> / fs_;
        float const loop_samples = fs_ / freq;
        float const allpass_set_delay = loop_samples * dispersion[idx] / (ThrianDispersion::kNumAPF + 0.1f);

        // update allpass filters
        dispersion_[simd_idx].SetGroupDelay(scalar_idx, allpass_set_delay);
        dispersion_[simd_idx + kMonoContainerSize].SetGroupDelay(scalar_idx, allpass_set_delay);
        float allpass_delay = dispersion_[simd_idx].GetPhaseDelay(scalar_idx, omega);

        // remove allpass delays
        float delay_samples = loop_samples - allpass_delay;
        delay_samples = std::max(delay_samples, 0.0f);

        // process frac delays
        delay_samples_[simd_idx][scalar_idx] = thrian_interp_[simd_idx].SetDelay(scalar_idx, delay_samples);
        thrian_interp_[simd_idx + kMonoContainerSize].SetDelay(scalar_idx, delay_samples);

        // feedback decay
        float feedback_gain = 0;
        if (decay_ms[idx] > 0.5f) {
            float const mul = -3.0f * loop_samples / (fs_ * decay_ms[idx] / 1000.0f);
            feedback_gain = std::pow(10.0f, mul);
            feedback_gain = std::min(feedback_gain, 1.0f);
        }
        else {
            feedback_gain = 0;
        }
        if (polarity[idx]) {
            feedback_gain = -feedback_gain;
        }
        feedback_gain_[simd_idx][scalar_idx] = feedback_gain;
    }

    void TrunOnAllInput(float v) noexcept {
        std::ranges::fill(input_volume_, SimdType::vBroadcast(v));
    }

    void Noteoff(size_t idx) noexcept {
        size_t simd_idx = idx / SimdType::kSize;
        size_t scalar_idx = idx & (SimdType::kSize - 1);
        input_volume_[simd_idx][scalar_idx] = 0;
    }

    // -------------------- params --------------------
    std::array<bool, kNumResonators> polarity{};
    std::array<float, kNumResonators> pitches{};
    std::array<float, kNumResonators> fine_tune{};
    std::array<float, kNumResonators> dispersion{};
    std::array<float, kNumResonators> decay_ms{};
    std::array<float, kNumResonators> damp_pitch{};
    std::array<float, kNumResonators> damp_gain_db{};
    std::array<float, kNumResonators> mix_db{};
    std::array<float, kNumResonators> norm_reflections{};
    float dry{};
private:
    SimdType ReadFeedback(uint32_t idx, uint32_t wpos, uint32_t delay_idx) noexcept {
        auto rpos = qwqdsp_simd_element::PackUint32<4>::vBroadcast(wpos + delay_mask_) - delay_samples_[delay_idx];
        rpos &= qwqdsp_simd_element::PackUint32<4>::vBroadcast(delay_mask_);
        SimdType delay_output;
        for (size_t k = 0; k < SimdType::kSize; ++k) {
            delay_output[k] = delay_buffer_[idx][static_cast<size_t>(rpos[k])][k];
        }
        return thrian_interp_[idx].Tick(delay_output);
    }

    static constexpr size_t kContainerSize = 2 * kNumResonators / SimdType::kSize;
    static constexpr size_t kMonoContainerSize = kNumResonators / SimdType::kSize;

    std::array<std::vector<SimdType>, kContainerSize> delay_buffer_;
    uint32_t delay_wpos_{};
    uint32_t delay_mask_{};
    std::array<TunningFilter, kContainerSize> thrian_interp_;
    std::array<ThrianDispersion, kContainerSize> dispersion_;
    std::array<qwqdsp_simd_element::OnePoleTPT<4>, kContainerSize> damp_;
    std::array<qwqdsp_simd_element::OnePoleTPT<4>, kContainerSize> dc_blocker;
    std::array<SimdType, kMonoContainerSize + 1> input_volume_{};
    std::array<SimdType, kMonoContainerSize> output_volume_{};
    std::array<SimdType, kMonoContainerSize> reflections_{};
    std::array<SimdType, kMonoContainerSize> damp_highshelf_coeff{};
    std::array<SimdType, kMonoContainerSize> damp_highshelf_gain{};
    std::array<SimdType, kMonoContainerSize> feedback_gain_{};
    std::array<qwqdsp_simd_element::PackUint32<4>, kMonoContainerSize> delay_samples_{};
    std::array<SimdType, kContainerSize> fb_values_{};
    float fs_{};
};
