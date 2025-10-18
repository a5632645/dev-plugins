#pragma once
#include <array>
#include <vector>
#include <numbers>
#include <cmath>
#include <algorithm>
#include <span>

#include "qwqdsp/psimd/vec4.hpp"
#include "qwqdsp/fx/delay_line_simd.hpp"
#include "qwqdsp/polymath.hpp"
#include "qwqdsp/convert.hpp"
#include "x86/sse2.h"

using SimdType = qwqdsp::psimd::Vec4f32;
using SimdIntType = qwqdsp::psimd::Vec4i32;

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

class VitalReverb {
public:
    // -------------------- params --------------------
    float chorus_amount{0.05f}; // [0, 1]
    float chorus_freq{0.25f}; // [0.003, 8.0]
    float wet{0.25f}; // [0, 1]
    float pre_lowpass{0.0f}; // [0, 130]
    float pre_highpass{110.0f}; // [0, 130]
    float low_damp_pitch{0.0f}; // [0, 130]
    float high_damp_pitch{90.0f}; // [0, 130]
    float low_damp_db{0.0f}; // [-6, 0]
    float high_damp_db{-1.0f}; // [-6, 0]
    float size{0.5f}; // [0, 1]
    float decay_ms{1000.0f}; // [15ms, 64s]
    float pre_delay{0.0f}; // [0, 300ms]

    void Init(float fs) {
        fs_ = fs;
        fs_ratio_ = fs / kBaseSampleRate;

        predelay_.Init(fs, 300.0f);

        low_pre_coefficient_ = 0.1f;
        high_pre_coefficient_ = 0.1f;
        low_coefficient_ = 0.1f;
        high_coefficient_ = 0.1f;
        low_amplitude_ = 0.0f;
        high_amplitude_ = 0.0f;
        sample_delay_ = kMinDelay;

        float const buffer_scale_ratio = fs / kBaseSampleRate;
        buffer_scale_ratio_ = buffer_scale_ratio;
        int const base_feedback_size = static_cast<int>(std::ceil(buffer_scale_ratio * (1 << (kBaseFeedbackBits + kMaxSizePower))));
        int max_feedback_size = 1;
        while (max_feedback_size < base_feedback_size) {
            max_feedback_size *= 2;
        }
        max_feedback_size_ = max_feedback_size;
        feedback_mask_ = max_feedback_size_ - 1;
        for (auto& buffer : feedback_memories_) {
            buffer.resize(static_cast<size_t>(max_feedback_size + kExtraLookupSample));
        }

        int const base_allpass_size = static_cast<int>(std::ceil(buffer_scale_ratio * (1 << kBaseAllpassBits)));
        int max_allpass_size =  1;
        while (max_allpass_size < base_allpass_size) {
            max_allpass_size *= 2;
        }
        max_allpass_size_ = max_allpass_size;
        poly_allpass_mask_ = max_allpass_size_ - 1;
        for (auto& buffer : allpass_lookups_) {
            buffer.resize(static_cast<size_t>(max_allpass_size));
        }

        feedback_offset_smooth_factor_ = 1.0f - std::exp(-1.0f / (fs_ * 50.0f / 1000.0f));

        write_index_ = 0;
        allpass_write_pos_ = 0;
    }

    void Reset() noexcept {
        wet_ = SimdType::FromSingle(0.0f);
        dry_ = SimdType::FromSingle(0.0f);
        chorus_amount_ = SimdType::FromSingle(chorus_amount * kMaxChorusDrift);

        for (auto& f : low_shelf_filters_) {
            f.Reset();
        }
        for (auto& f : high_shelf_filters_) {
            f.Reset();
        }
        low_pre_filter_.Reset();
        high_pre_filter_.Reset();
        predelay_.Reset();
        for (auto& d : decays_) {
            d = SimdType::FromSingle(0);
        }
        for (size_t i = 0; i < kNetworkContainers; ++i) {
            feedback_offsets_[i] = kFeedbackDelays[i];
        }

        for (auto& buffer : allpass_lookups_) {
            std::ranges::fill(buffer, SimdType::FromSingle(0));
        }
        for (auto& buffer : feedback_memories_) {
            std::ranges::fill(buffer, SimdType::FromSingle(0));
        }
    }

    /**
     * @param input_cross => [left, right, left, right]
     * @param lr_output   => [left, right, ?, ?]
     */
    void Process(std::span<SimdType> input_cross, std::span<SimdType> lr_output) noexcept {
        SimdType* audio_in = input_cross.data();
        SimdType* audio_out = lr_output.data();
        size_t num_samples = input_cross.size();

        float const tick_increment = 1.0f / static_cast<float>(num_samples);

        SimdType current_dry = dry_;
        SimdType current_wet = wet_;
        float current_low_pre_coefficient = low_pre_coefficient_;
        float current_high_pre_coefficient = high_pre_coefficient_;
        float current_low_coefficient = low_coefficient_;
        float current_low_amplitude = low_amplitude_;
        float current_high_coefficient = high_coefficient_;
        float current_high_amplitude = high_amplitude_;

        wet_ = SimdType::FromSingle(qwqdsp::polymath::SinPi(wet * std::numbers::pi_v<float> / 2));
        dry_ = SimdType::FromSingle(qwqdsp::polymath::CosPi(wet * std::numbers::pi_v<float> / 2));
        SimdType delta_wet = (wet_ - current_wet) * SimdType::FromSingle(tick_increment);
        SimdType delta_dry = (dry_ - current_dry) * SimdType::FromSingle(tick_increment);

        float const low_pre_cutoff_frequency = qwqdsp::convert::Pitch2Freq(pre_lowpass);
        low_pre_coefficient_ = ParalleOnePoleTPT::ComputeCoeff(qwqdsp::convert::Freq2W(low_pre_cutoff_frequency, fs_));
        float const high_pre_cutoff_frequency = qwqdsp::convert::Pitch2Freq(pre_highpass);
        high_pre_coefficient_ = ParalleOnePoleTPT::ComputeCoeff(qwqdsp::convert::Freq2W(high_pre_cutoff_frequency, fs_));
        float delta_low_pre_coefficient = (low_pre_coefficient_ - current_low_pre_coefficient) * tick_increment;
        float delta_high_pre_coefficient = (high_pre_coefficient_ - current_high_pre_coefficient) * tick_increment;

        float const low_cutoff_frequency = qwqdsp::convert::Pitch2Freq(low_damp_pitch);
        low_coefficient_ = ParalleOnePoleTPT::ComputeCoeff(qwqdsp::convert::Freq2W(low_cutoff_frequency, fs_));
        float const high_cutoff_frequency = qwqdsp::convert::Pitch2Freq(high_damp_pitch);
        high_coefficient_ = ParalleOnePoleTPT::ComputeCoeff(qwqdsp::convert::Freq2W(high_cutoff_frequency, fs_));
        float delta_low_coefficient = (low_coefficient_ - current_low_coefficient) * tick_increment;
        float delta_high_coefficient = (high_coefficient_ - current_high_coefficient) * tick_increment;

        low_amplitude_ = 1.0f - qwqdsp::convert::Db2Gain(low_damp_db);
        high_amplitude_ = qwqdsp::convert::Db2Gain(high_damp_db);
        float delta_low_amplitude = (low_amplitude_ - current_low_amplitude) * tick_increment;
        float delta_high_amplitude = (high_amplitude_ - current_high_amplitude) * tick_increment;

        float const size_mult = std::exp2(size * kSizePowerRange + kMinSizePower);

        float const decay_samples = decay_ms * kBaseSampleRate / 1000.0f;
        float const decay_period = size_mult / decay_samples;
        SimdType current_decay1 = decays_[0];
        SimdType current_decay2 = decays_[1];
        SimdType current_decay3 = decays_[2];
        SimdType current_decay4 = decays_[3];
        for (size_t j = 0; j < kNetworkContainers; ++j) {
            for (size_t i = 0; i < SimdType::kSize; ++i) {
                decays_[j].x[i] = std::pow(kT60Amplitude, kFeedbackDelays[j].x[i] * decay_period);
            }
        }
        SimdType delta_decay1 = (decays_[0] - current_decay1) * SimdType::FromSingle(tick_increment);
        SimdType delta_decay2 = (decays_[1] - current_decay2) * SimdType::FromSingle(tick_increment);
        SimdType delta_decay3 = (decays_[2] - current_decay3) * SimdType::FromSingle(tick_increment);
        SimdType delta_decay4 = (decays_[3] - current_decay4) * SimdType::FromSingle(tick_increment);

        SimdIntType allpass_offset1 = (kAllpassDelays[0] * SimdType::FromSingle(buffer_scale_ratio_)).ToInt();
        SimdIntType allpass_offset2 = (kAllpassDelays[1] * SimdType::FromSingle(buffer_scale_ratio_)).ToInt();
        SimdIntType allpass_offset3 = (kAllpassDelays[2] * SimdType::FromSingle(buffer_scale_ratio_)).ToInt();
        SimdIntType allpass_offset4 = (kAllpassDelays[3] * SimdType::FromSingle(buffer_scale_ratio_)).ToInt();

        float const chorus_phase_increment = chorus_freq / fs_;

        float const network_offset = 2.0f * std::numbers::pi_v<float> / kNetworkSize;
        SimdType phase_offset = SimdType{0.0f, 1.0f, 2.0f, 3.0f} * SimdType::FromSingle(network_offset);
        SimdType container_phase = phase_offset + SimdType::FromSingle(chorus_phase_ * 2.0f * std::numbers::pi_v<float>);
        chorus_phase_ += static_cast<float>(num_samples) * chorus_phase_increment;
        chorus_phase_ -= std::floor(chorus_phase_);

        SimdType chorus_increment_real = SimdType::FromSingle(std::cos(chorus_phase_increment * (2.0f * std::numbers::pi_v<float>)));
        SimdType chorus_increment_imaginary = SimdType::FromSingle(std::sin(chorus_phase_increment * (2.0f * std::numbers::pi_v<float>)));
        SimdType current_chorus_real;
        SimdType current_chorus_imaginary;
        for (size_t i = 0; i < SimdType::kSize; ++i) {
            current_chorus_real.x[i] = std::cos(container_phase.x[i]);
        }
        for (size_t i = 0; i < SimdType::kSize; ++i) {
            current_chorus_imaginary.x[i] = std::cos(container_phase.x[i]);
        }

        SimdType delay1 = kFeedbackDelays[0] * SimdType::FromSingle(fs_ / kBaseSampleRate * size_mult);
        SimdType delay2 = kFeedbackDelays[1] * SimdType::FromSingle(fs_ / kBaseSampleRate * size_mult);
        SimdType delay3 = kFeedbackDelays[2] * SimdType::FromSingle(fs_ / kBaseSampleRate * size_mult);
        SimdType delay4 = kFeedbackDelays[3] * SimdType::FromSingle(fs_ / kBaseSampleRate * size_mult);

        SimdType current_chorus_amount = chorus_amount_;
        chorus_amount_ = SimdType::FromSingle(chorus_amount * kMaxChorusDrift * fs_ratio_);
        chorus_amount_ = SimdType::Min(chorus_amount_, delay1 - SimdType::FromSingle(8 * SimdType::kSize));
        chorus_amount_ = SimdType::Min(chorus_amount_, delay2 - SimdType::FromSingle(8 * SimdType::kSize));
        chorus_amount_ = SimdType::Min(chorus_amount_, delay3 - SimdType::FromSingle(8 * SimdType::kSize));
        chorus_amount_ = SimdType::Min(chorus_amount_, delay4 - SimdType::FromSingle(8 * SimdType::kSize));
        SimdType delta_chorus_amount = (chorus_amount_ - current_chorus_amount) * SimdType::FromSingle(tick_increment);
        current_chorus_amount = current_chorus_amount * SimdType::FromSingle(size_mult);

        float current_sample_delay = sample_delay_;
        float current_delay_increment = sample_delay_increment_;
        float end_target = current_sample_delay + current_delay_increment * static_cast<float>(num_samples);
        float target_delay = std::max(kMinDelay, pre_delay * fs_ / 1000.0f);
        target_delay = std::lerp(sample_delay_, target_delay, kSampleDelayMultiplier);
        float makeup_delay = target_delay - end_target;
        float delta_delay_increment = makeup_delay / (0.5f * static_cast<float>(num_samples * num_samples)) * kSampleIncrementMultiplier;

        SimdType feedback_offset1 = feedback_offsets_[0];
        SimdType feedback_offset2 = feedback_offsets_[1];
        SimdType feedback_offset3 = feedback_offsets_[2];
        SimdType feedback_offset4 = feedback_offsets_[3];

        for (size_t i = 0; i < num_samples; ++i) {
            // paralle chorus delaylines
            current_chorus_amount += delta_chorus_amount;
            current_chorus_real = current_chorus_real * chorus_increment_real -
                                current_chorus_imaginary * chorus_increment_imaginary;
            current_chorus_imaginary = current_chorus_imaginary * chorus_increment_real +
                                        current_chorus_real * chorus_increment_imaginary;
            SimdType new_feedback_offset1 = delay1 + current_chorus_real * current_chorus_amount;
            SimdType new_feedback_offset2 = delay2 - current_chorus_real * current_chorus_amount;
            SimdType new_feedback_offset3 = delay3 + current_chorus_imaginary * current_chorus_amount;
            SimdType new_feedback_offset4 = delay4 - current_chorus_imaginary * current_chorus_amount;
            feedback_offset1 += SimdType::FromSingle(feedback_offset_smooth_factor_) * (new_feedback_offset1 - feedback_offset1);
            feedback_offset2 += SimdType::FromSingle(feedback_offset_smooth_factor_) * (new_feedback_offset2 - feedback_offset2);
            feedback_offset3 += SimdType::FromSingle(feedback_offset_smooth_factor_) * (new_feedback_offset3 - feedback_offset3);
            feedback_offset4 += SimdType::FromSingle(feedback_offset_smooth_factor_) * (new_feedback_offset4 - feedback_offset4);

            SimdType feedback_read1 = ReadFeedback(0, feedback_offset1);
            SimdType feedback_read2 = ReadFeedback(1, feedback_offset2);
            SimdType feedback_read3 = ReadFeedback(2, feedback_offset3);
            SimdType feedback_read4 = ReadFeedback(3, feedback_offset4);

            SimdType input = audio_in[i];
            SimdType filtered_input = high_pre_filter_.TickLowpass(input, current_high_pre_coefficient);
            filtered_input = low_pre_filter_.TickLowpass(input, current_low_pre_coefficient) - filtered_input;
            SimdType scaled_input = filtered_input * SimdType::FromSingle(0.5f);

            // paralle polyphase allpass
            SimdType allpass_read1 = ReadAllpass(0, allpass_offset1);
            SimdType allpass_read2 = ReadAllpass(1, allpass_offset2);
            SimdType allpass_read3 = ReadAllpass(2, allpass_offset3);
            SimdType allpass_read4 = ReadAllpass(3, allpass_offset4);
            
            SimdType allpass_delay_input1 = feedback_read1 - allpass_read1 * SimdType::FromSingle(kAllpassFeedback);
            SimdType allpass_delay_input2 = feedback_read2 - allpass_read2 * SimdType::FromSingle(kAllpassFeedback);
            SimdType allpass_delay_input3 = feedback_read3 - allpass_read3 * SimdType::FromSingle(kAllpassFeedback);
            SimdType allpass_delay_input4 = feedback_read4 - allpass_read4 * SimdType::FromSingle(kAllpassFeedback);

            allpass_lookups_[0][static_cast<size_t>(allpass_write_pos_)] = scaled_input + allpass_delay_input1;
            allpass_lookups_[1][static_cast<size_t>(allpass_write_pos_)] = scaled_input + allpass_delay_input2;
            allpass_lookups_[2][static_cast<size_t>(allpass_write_pos_)] = scaled_input + allpass_delay_input3;
            allpass_lookups_[3][static_cast<size_t>(allpass_write_pos_)] = scaled_input + allpass_delay_input4;
            allpass_write_pos_ = (allpass_write_pos_ + 1) & poly_allpass_mask_;

            SimdType allpass_output1 = allpass_read1 + allpass_delay_input1 * SimdType::FromSingle(kAllpassFeedback);
            SimdType allpass_output2 = allpass_read2 + allpass_delay_input2 * SimdType::FromSingle(kAllpassFeedback);
            SimdType allpass_output3 = allpass_read3 + allpass_delay_input3 * SimdType::FromSingle(kAllpassFeedback);
            SimdType allpass_output4 = allpass_read4 + allpass_delay_input4 * SimdType::FromSingle(kAllpassFeedback);

            // scatter matrix
            SimdType total_rows = allpass_output1 + allpass_output2 + allpass_output3 + allpass_output4;
            SimdType other_feedback = SimdType::FMA(total_rows, SimdType::FromSingle(-0.5f), SimdType::FromSingle(total_rows.ReduceAdd() * 0.25f));

            SimdType write1 = other_feedback + allpass_output1;
            SimdType write2 = other_feedback + allpass_output2;
            SimdType write3 = other_feedback + allpass_output3;
            SimdType write4 = other_feedback + allpass_output4;

            {
                // this is a matrix transpose
                auto row0 = simde_mm_load_ps(allpass_output1.x);
                auto row1 = simde_mm_load_ps(allpass_output2.x);
                auto row2 = simde_mm_load_ps(allpass_output3.x);
                auto row3 = simde_mm_load_ps(allpass_output4.x);
                auto low0 = simde_mm_unpacklo_ps(row0, row1);
                auto low1 = simde_mm_unpacklo_ps(row2, row3);
                auto high0 = simde_mm_unpackhi_ps(row0, row1);
                auto high1 = simde_mm_unpackhi_ps(row2, row3);
                row0 = simde_mm_movelh_ps(low0, low1);
                row1 = simde_mm_movehl_ps(low1, low0);
                row2 = simde_mm_movelh_ps(high0, high1);
                row3 = simde_mm_movehl_ps(high1, high0);
                simde_mm_store_ps(allpass_output1.x, row0);
                simde_mm_store_ps(allpass_output2.x, row1);
                simde_mm_store_ps(allpass_output3.x, row2);
                simde_mm_store_ps(allpass_output4.x, row3);
            }
            SimdType adjacent_feedback = (allpass_output1 + allpass_output2 + allpass_output3 + allpass_output4) * SimdType::FromSingle(-0.5f);
            write1 += SimdType::FromSingle(adjacent_feedback.x[0]);
            write2 += SimdType::FromSingle(adjacent_feedback.x[1]);
            write3 += SimdType::FromSingle(adjacent_feedback.x[2]);
            write4 += SimdType::FromSingle(adjacent_feedback.x[3]);

            // damp filter
            SimdType high_filtered1 = high_shelf_filters_[0].TickLowpass(write1, current_high_coefficient);
            SimdType high_filtered2 = high_shelf_filters_[1].TickLowpass(write2, current_high_coefficient);
            SimdType high_filtered3 = high_shelf_filters_[2].TickLowpass(write3, current_high_coefficient);
            SimdType high_filtered4 = high_shelf_filters_[3].TickLowpass(write4, current_high_coefficient);
            write1 = high_filtered1 + SimdType::FromSingle(current_high_amplitude) * (write1 - high_filtered1);
            write2 = high_filtered2 + SimdType::FromSingle(current_high_amplitude) * (write2 - high_filtered2);
            write3 = high_filtered3 + SimdType::FromSingle(current_high_amplitude) * (write3 - high_filtered3);
            write4 = high_filtered4 + SimdType::FromSingle(current_high_amplitude) * (write4 - high_filtered4);

            SimdType low_filtered1 = low_shelf_filters_[0].TickLowpass(write1, current_low_coefficient);
            SimdType low_filtered2 = low_shelf_filters_[1].TickLowpass(write2, current_low_coefficient);
            SimdType low_filtered3 = low_shelf_filters_[2].TickLowpass(write3, current_low_coefficient);
            SimdType low_filtered4 = low_shelf_filters_[3].TickLowpass(write4, current_low_coefficient);
            write1 -= low_filtered1 * SimdType::FromSingle(current_low_amplitude);
            write2 -= low_filtered2 * SimdType::FromSingle(current_low_amplitude);
            write3 -= low_filtered3 * SimdType::FromSingle(current_low_amplitude);
            write4 -= low_filtered4 * SimdType::FromSingle(current_low_amplitude);

            // decay block
            current_decay1 += delta_decay1;
            current_decay2 += delta_decay2;
            current_decay3 += delta_decay3;
            current_decay4 += delta_decay4;
            SimdType store1 = current_decay1 * write1;
            SimdType store2 = current_decay2 * write2;
            SimdType store3 = current_decay3 * write3;
            SimdType store4 = current_decay4 * write4;
            feedback_memories_[0][static_cast<size_t>(write_index_)] = store1;
            feedback_memories_[1][static_cast<size_t>(write_index_)] = store2;
            feedback_memories_[2][static_cast<size_t>(write_index_)] = store3;
            feedback_memories_[3][static_cast<size_t>(write_index_)] = store4;
            write_index_ = (write_index_ + 1) & feedback_mask_;

            // what is this?
            SimdType total_allpass = store1 + store2 + store3 + store4;
            SimdType other_feedback_allpass = SimdType::FMA(total_allpass, SimdType::FromSingle(-0.5f), SimdType::FromSingle(total_allpass.ReduceAdd() * 0.25f));

            SimdType feed_forward1 = other_feedback_allpass + store1;
            SimdType feed_forward2 = other_feedback_allpass + store2;
            SimdType feed_forward3 = other_feedback_allpass + store3;
            SimdType feed_forward4 = other_feedback_allpass + store4;

            {
                auto row0 = simde_mm_load_ps(store1.x);
                auto row1 = simde_mm_load_ps(store2.x);
                auto row2 = simde_mm_load_ps(store3.x);
                auto row3 = simde_mm_load_ps(store4.x);
                auto low0 = simde_mm_unpacklo_ps(row0, row1);
                auto low1 = simde_mm_unpacklo_ps(row2, row3);
                auto high0 = simde_mm_unpackhi_ps(row0, row1);
                auto high1 = simde_mm_unpackhi_ps(row2, row3);
                row0 = simde_mm_movelh_ps(low0, low1);
                row1 = simde_mm_movehl_ps(low1, low0);
                row2 = simde_mm_movelh_ps(high0, high1);
                row3 = simde_mm_movehl_ps(high1, high0);
                simde_mm_store_ps(store1.x, row0);
                simde_mm_store_ps(store2.x, row1);
                simde_mm_store_ps(store3.x, row2);
                simde_mm_store_ps(store4.x, row3);
            }
            SimdType adjacent_feedback_allpass = (store1 + store2 + store3 + store4) * SimdType::FromSingle(-0.5f);

            feed_forward1 += SimdType::FromSingle(adjacent_feedback_allpass.x[0]);
            feed_forward2 += SimdType::FromSingle(adjacent_feedback_allpass.x[1]);
            feed_forward3 += SimdType::FromSingle(adjacent_feedback_allpass.x[2]);
            feed_forward4 += SimdType::FromSingle(adjacent_feedback_allpass.x[3]);

            // predelay
            SimdType total = write1 + write2 + write3 + write4;
            total += (feed_forward1 * current_decay1 + feed_forward2 * current_decay2 +
                    feed_forward3 * current_decay3 + feed_forward4 * current_decay4) * SimdType::FromSingle(0.125f);

            SimdType output{
                total.x[0] + total.x[2],
                total.x[1] + total.x[3]
            };
            predelay_.Push(output);
            audio_out[i] = current_wet * predelay_.GetAfterPush(current_sample_delay) + current_dry * input;

            current_delay_increment += delta_delay_increment;
            current_sample_delay += current_delay_increment;
            current_sample_delay = std::max(current_sample_delay, kMinDelay);
            current_dry += delta_dry;
            current_wet += delta_wet;
            current_high_coefficient += delta_high_coefficient;
            current_high_amplitude += delta_high_amplitude;
            current_low_pre_coefficient += delta_low_pre_coefficient;
            current_high_pre_coefficient += delta_high_pre_coefficient;
            current_low_coefficient += delta_low_coefficient;
            current_low_amplitude += delta_low_amplitude;
        }

        sample_delay_increment_ = current_delay_increment;
        sample_delay_ = current_sample_delay;
        feedback_offsets_[0] = feedback_offset1;
        feedback_offsets_[1] = feedback_offset2;
        feedback_offsets_[2] = feedback_offset3;
        feedback_offsets_[3] = feedback_offset4;
    }

    SimdType ReadFeedback(size_t idx, SimdType offset) {
        SimdType rpos = SimdType::FromSingle(static_cast<float>(write_index_ + feedback_mask_)) - offset;
        SimdIntType mask = SimdIntType::FromSingle(feedback_mask_);
        SimdIntType irpos = rpos.ToInt() & mask;
        SimdIntType iprev1 = (irpos - SimdIntType::FromSingle(1)) & mask;
        SimdIntType inext1 = (irpos + SimdIntType::FromSingle(1)) & mask;
        SimdIntType inext2 = (irpos + SimdIntType::FromSingle(2)) & mask;
        SimdType t = rpos.Frac();

        SimdType yn1;
        SimdType y0;
        SimdType y1;
        SimdType y2;
        auto& buffer = feedback_memories_[idx];
        for (size_t i = 0; i < SimdType::kSize; ++i) {
            yn1.x[i] = buffer[static_cast<size_t>(iprev1.x[i])].x[i];
            y0.x[i]  = buffer[static_cast<size_t>(irpos.x[i])].x[i];
            y1.x[i]  = buffer[static_cast<size_t>(inext1.x[i])].x[i];
            y2.x[i]  = buffer[static_cast<size_t>(inext2.x[i])].x[i];
        }

        SimdType d0 = (y1 - yn1) * SimdType::FromSingle(0.5f);
        SimdType d1 = (y2 - y0) * SimdType::FromSingle(0.5f);
        SimdType d = y1 - y0;
        SimdType m0 = SimdType::FromSingle(3.0f) * d - SimdType::FromSingle(2.0f) * d0 - d1;
        SimdType m1 = d0 - SimdType::FromSingle(2.0f) * d + d1;
        return y0 + t * (
            d0 + t * (
                m0 + t * m1
            )
        );
    }

    SimdType ReadAllpass(size_t i, SimdIntType offset) {
        auto& buffer = allpass_lookups_[i];
        SimdIntType irpos = SimdIntType::FromSingle(allpass_write_pos_ + poly_allpass_mask_) - offset;
        irpos &= SimdIntType::FromSingle(poly_allpass_mask_);
        return SimdType{
            buffer[static_cast<size_t>(irpos.x[0])].x[0],
            buffer[static_cast<size_t>(irpos.x[1])].x[1],
            buffer[static_cast<size_t>(irpos.x[2])].x[2],
            buffer[static_cast<size_t>(irpos.x[3])].x[3]
        };
    }
private:
    static constexpr float kT60Amplitude = 0.001f;
    static constexpr float kAllpassFeedback = 0.6f;
    static constexpr float kMinDelay = 3.0f;

    static constexpr int kBaseSampleRate = 44100;
    static constexpr int kDefaultSampleRate = 88200;
    static constexpr int kNetworkSize = 16;
    static constexpr int kBaseFeedbackBits = 14;
    static constexpr int kExtraLookupSample = 4;
    static constexpr int kBaseAllpassBits = 10;
    static constexpr int kNetworkContainers = kNetworkSize / SimdType::kSize;
    static constexpr int kMinSizePower = -3;
    static constexpr int kMaxSizePower = 1;
    static constexpr float kSizePowerRange = kMaxSizePower - kMinSizePower;

    static constexpr float kMaxChorusDrift = 2500.0f;
    static constexpr float kMinDecayTime = 0.1f;
    static constexpr float kMaxDecayTime = 100.0f;
    static constexpr float kMaxChorusFrequency = 16.0f;
    static constexpr float kChorusShiftAmount = 0.9f;
    static constexpr float kSampleDelayMultiplier = 0.05f;
    static constexpr float kSampleIncrementMultiplier = 0.05f;

    static constexpr SimdType kAllpassDelays[kNetworkContainers]{
    { 1001, 799, 933, 876 },
    { 895, 807, 907, 853 },
    { 957, 1019, 711, 567 },
    { 833, 779, 663, 997 }
    };
    static constexpr SimdType kFeedbackDelays[kNetworkContainers]{
    { 6753.2f, 9278.4f, 7704.5f, 11328.5f },
    { 9701.12f, 5512.5f, 8480.45f, 5638.65f },
    { 3120.73f, 3429.5f, 3626.37f, 7713.52f },
    { 4521.54f, 6518.97f, 5265.56f, 5630.25f }
    };

    qwqdsp::fx::DelayLineSIMD<SimdType, qwqdsp::fx::DelayLineInterpSIMD::PCHIP> predelay_;
    std::array<std::vector<SimdType>, kNetworkContainers> allpass_lookups_;
    std::array<std::vector<SimdType>, kNetworkContainers> feedback_memories_;
    std::array<SimdType, kNetworkContainers> decays_{};
    std::array<SimdType, kNetworkContainers> feedback_offsets_{};

    std::array<ParalleOnePoleTPT, kNetworkContainers> low_shelf_filters_;
    std::array<ParalleOnePoleTPT, kNetworkContainers> high_shelf_filters_;

    ParalleOnePoleTPT low_pre_filter_;
    ParalleOnePoleTPT high_pre_filter_;

    float low_pre_coefficient_{};
    float high_pre_coefficient_{};
    float low_coefficient_{};
    float low_amplitude_{};
    float high_coefficient_{};
    float high_amplitude_{};
    float feedback_offset_smooth_factor_{};

    float chorus_phase_{};
    SimdType chorus_amount_{};
    float sample_delay_{};
    float sample_delay_increment_{};
    SimdType dry_{};
    SimdType wet_{};
    int write_index_{};

    int max_allpass_size_{};
    int max_feedback_size_{};
    int feedback_mask_{};
    int poly_allpass_mask_{};
    int allpass_write_pos_{};

    float fs_{};
    float fs_ratio_{};
    float buffer_scale_ratio_{};
};
