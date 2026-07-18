#pragma once
#include <memory>
#include <qwqdsp/convert.hpp>
#include "global.hpp"
#include "idsp.hpp"
#include "pluginshared/align_allocator.hpp"
#include "pluginshared/dsp/delay_line_multiple.hpp"
#include "pluginshared/dsp/delay_line_single.hpp"
#include "pluginshared/dsp/one_pole_tpt.hpp"
#include "pluginshared/simd/simd.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"

namespace vital_reverb {

static constexpr float kT60Amplitude = 0.001f;
static constexpr float kAllpassFeedback = 0.6f;
static constexpr float kMinDelay = 3.0f;

static constexpr int kBaseSampleRate = 44100;
static constexpr int kDefaultSampleRate = 88200;
static constexpr int kNetworkSize = 16;
static constexpr int kBaseFeedbackBits = 14;
static constexpr int kExtraLookupSample = 4;
static constexpr int kBaseAllpassBits = 10;
static constexpr int kMinSizePower = -3;
static constexpr int kMaxSizePower = 1;
static constexpr float kSizePowerRange = kMaxSizePower - kMinSizePower;
template <class SimdType>
static constexpr int kNetworkContainers = kNetworkSize / simd::LaneSize<SimdType>;

static constexpr float kMaxChorusDrift = 2500.0f;
static constexpr float kMinDecayTime = 0.1f;
static constexpr float kMaxDecayTime = 100.0f;
static constexpr float kMaxChorusFrequency = 16.0f;
static constexpr float kChorusShiftAmount = 0.9f;
static constexpr float kSampleDelayMultiplier = 0.05f;
static constexpr float kSampleIncrementMultiplier = 0.05f;

static constexpr simd::Array256 kAllpassDelays{
    std::array{1001, 799, 933, 876, 895, 807, 907, 853, 957, 1019, 711, 567, 833, 779, 663, 997}
};
static constexpr simd::Array256 kFeedbackDelays{
    std::array<float, kNetworkSize>{6753.2f, 9278.4f, 7704.5f, 11328.5f, 9701.12f, 5512.5f, 8480.45f, 5638.65f,
                                    3120.73f, 3429.5f, 3626.37f, 7713.52f, 4521.54f, 6518.97f, 5265.56f, 5630.25}
};

template <simd::Inst inst, class SimdT>
class DspImpl : public Idsp {
public:
    void Init(float fs) override {
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
        uint32_t const base_feedback_size =
            static_cast<uint32_t>(std::ceil(buffer_scale_ratio * (1 << (kBaseFeedbackBits + kMaxSizePower))));
        uint32_t max_feedback_size = 1;
        while (max_feedback_size < base_feedback_size) {
            max_feedback_size *= 2;
        }
        max_feedback_size_ = static_cast<int>(max_feedback_size);
        feedback_mask_ = max_feedback_size_ - 1;

        uint32_t each_size = max_feedback_size + kExtraLookupSample;
        feedback_memorie_.resize(each_size * kNetworkSize);
        for (int i = 0; i < kNetworkSize; ++i) {
            feedback_ptrs_[static_cast<size_t>(i)] = &feedback_memorie_[static_cast<size_t>(i) * each_size];
        }

        uint32_t const base_allpass_size =
            static_cast<uint32_t>(std::ceil(buffer_scale_ratio * (1 << kBaseAllpassBits)));
        uint32_t max_allpass_size = 1;
        while (max_allpass_size < base_allpass_size) {
            max_allpass_size *= 2;
        }
        max_allpass_size_ = static_cast<int>(max_allpass_size);
        poly_allpass_mask_ = max_allpass_size_ - 1;
        for (auto& buffer : allpass_lookups_) {
            buffer.resize(static_cast<size_t>(max_allpass_size));
        }

        feedback_offset_smooth_factor_ = 1.0f - std::exp(-1.0f / (fs * 50.0f / 1000.0f));

        write_index_ = 0;
        allpass_write_pos_ = 0;
    }

    void Reset() override {
        wet_ = 0;
        dry_ = 0;
        chorus_amount_ = simd::Broadcast<SimdT>(param_.chorus_amount * kMaxChorusDrift);

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
            d = simd::Broadcast<SimdT>(0);
        }

        const SimdT* feedback_delays = std::assume_aligned<alignof(SimdT)>(reinterpret_cast<const SimdT*>(kFeedbackDelays.data()));
        for (size_t i = 0; i < kContainerSize; ++i) {
            feedback_offsets_[i] = feedback_delays[i];
        }

        for (auto& buffer : allpass_lookups_) {
            std::fill(buffer.begin(), buffer.end(), SimdT{});
        }
        std::fill(feedback_memorie_.begin(), feedback_memorie_.end(), 0.0f);
    }

    void Panic() override {
        for (auto& s : allpass_lookups_) {
            std::fill(s.begin(), s.end(), SimdT{});
        }
        std::fill(feedback_memorie_.begin(), feedback_memorie_.end(), 0.0f);
        predelay_.Reset();
        for (auto& s : low_shelf_filters_) {
            s.Reset();
        }
        for (auto& s : high_shelf_filters_) {
            s.Reset();
        }
        low_pre_filter_.Reset();
        high_pre_filter_.Reset();
    }

    void Update(const Param& p) override {
        param_ = p;
    }

    void Process(float* left, float* right, int num_samples) override {
        if (right == nullptr) {
            ProcessInternal<true, SimdT>(left, right, num_samples);
        }
        else {
            ProcessInternal<false, SimdT>(left, right, num_samples);
        }
    }

    std::string_view InstName() override {
        return INST_NAME;
    }
private:
    template <bool kMono, class T>
        requires std::same_as<T, simd::Float128>
    void ProcessInternal(float* left, float* right, int num_samples) noexcept {
        WarpBuffer();

        float const tick_increment = 1.0f / static_cast<float>(num_samples);

        float current_dry = dry_;
        float current_wet = wet_;
        float current_low_pre_coefficient = low_pre_coefficient_;
        float current_high_pre_coefficient = high_pre_coefficient_;
        float current_low_coefficient = low_coefficient_;
        float current_low_amplitude = low_amplitude_;
        float current_high_coefficient = high_coefficient_;
        float current_high_amplitude = high_amplitude_;

        wet_ = std::sin(param_.wet * std::numbers::pi_v<float> / 2);
        dry_ = std::cos(param_.wet * std::numbers::pi_v<float> / 2);
        float delta_wet = (wet_ - current_wet) * tick_increment;
        float delta_dry = (dry_ - current_dry) * tick_increment;

        float const low_pre_cutoff_frequency = qwqdsp::convert::Pitch2Freq(param_.pre_lowpass);
        low_pre_coefficient_ =
            decltype(low_pre_filter_)::ComputeCoeff(qwqdsp::convert::Freq2W(low_pre_cutoff_frequency, fs_));
        float const high_pre_cutoff_frequency = qwqdsp::convert::Pitch2Freq(param_.pre_highpass);
        high_pre_coefficient_ =
            decltype(high_pre_filter_)::ComputeCoeff(qwqdsp::convert::Freq2W(high_pre_cutoff_frequency, fs_));
        float delta_low_pre_coefficient = (low_pre_coefficient_ - current_low_pre_coefficient) * tick_increment;
        float delta_high_pre_coefficient = (high_pre_coefficient_ - current_high_pre_coefficient) * tick_increment;

        float const low_cutoff_frequency = qwqdsp::convert::Pitch2Freq(param_.low_damp_pitch);
        low_coefficient_ = decltype(low_pre_filter_)::ComputeCoeff(qwqdsp::convert::Freq2W(low_cutoff_frequency, fs_));
        float const high_cutoff_frequency = qwqdsp::convert::Pitch2Freq(param_.high_damp_pitch);
        high_coefficient_ =
            decltype(high_pre_filter_)::ComputeCoeff(qwqdsp::convert::Freq2W(high_cutoff_frequency, fs_));
        float delta_low_coefficient = (low_coefficient_ - current_low_coefficient) * tick_increment;
        float delta_high_coefficient = (high_coefficient_ - current_high_coefficient) * tick_increment;

        low_amplitude_ = 1.0f - qwqdsp::convert::Db2Gain(param_.low_damp_db);
        high_amplitude_ = qwqdsp::convert::Db2Gain(param_.high_damp_db);
        float delta_low_amplitude = (low_amplitude_ - current_low_amplitude) * tick_increment;
        float delta_high_amplitude = (high_amplitude_ - current_high_amplitude) * tick_increment;

        float const size_mult = std::exp2(param_.size * kSizePowerRange + kMinSizePower);

        float const decay_samples = param_.decay_ms * kBaseSampleRate / 1000.0f;
        float const decay_period = size_mult / decay_samples;
        SimdT current_decay1 = decays_[0];
        SimdT current_decay2 = decays_[1];
        SimdT current_decay3 = decays_[2];
        SimdT current_decay4 = decays_[3];

        if (param_.freeze) {
            decays_.fill(simd::Broadcast<SimdT>(1.0f));
        }
        else {
            for (size_t j = 0; j < kContainerSize; ++j) {
                for (size_t i = 0; i < 4; ++i) {
                    decays_[j][i] = std::pow(kT60Amplitude, kFeedbackDelays[j * 4 + i] * decay_period);
                }
            }
        }
        auto delta_decay1 = (decays_[0] - current_decay1) * tick_increment;
        auto delta_decay2 = (decays_[1] - current_decay2) * tick_increment;
        auto delta_decay3 = (decays_[2] - current_decay3) * tick_increment;
        auto delta_decay4 = (decays_[3] - current_decay4) * tick_increment;

        const simd::Int128* allpass_delays = std::assume_aligned<16>(reinterpret_cast<const simd::Int128*>(kAllpassDelays.data()));
        auto allpass_offset1 = simd::ToInt(simd::ToFloat(allpass_delays[0]) * buffer_scale_ratio_);
        auto allpass_offset2 = simd::ToInt(simd::ToFloat(allpass_delays[1]) * buffer_scale_ratio_);
        auto allpass_offset3 = simd::ToInt(simd::ToFloat(allpass_delays[2]) * buffer_scale_ratio_);
        auto allpass_offset4 = simd::ToInt(simd::ToFloat(allpass_delays[3]) * buffer_scale_ratio_);

        float const chorus_phase_increment = param_.chorus_freq / fs_;

        float const network_offset = 2.0f * std::numbers::pi_v<float> / kNetworkSize;
        auto phase_offset = simd::Float128{0.0f, 1.0f, 2.0f, 3.0f} * network_offset;
        auto container_phase = phase_offset + chorus_phase_ * 2.0f * std::numbers::pi_v<float>;
        chorus_phase_ += static_cast<float>(num_samples) * chorus_phase_increment;
        chorus_phase_ -= std::floor(chorus_phase_);

        auto chorus_increment_real =
            simd::BroadcastF128(std::cos(chorus_phase_increment * (2.0f * std::numbers::pi_v<float>)));
        auto chorus_increment_imaginary =
            simd::BroadcastF128(std::sin(chorus_phase_increment * (2.0f * std::numbers::pi_v<float>)));
        simd::Float128 current_chorus_real{};
        simd::Float128 current_chorus_imaginary{};
        for (size_t i = 0; i < 4; ++i) {
            current_chorus_real[i] = std::cos(container_phase[i]);
        }
        for (size_t i = 0; i < 4; ++i) {
            current_chorus_imaginary[i] = std::sin(container_phase[i]);
        }

        const simd::Float128* feedback_delays = std::assume_aligned<16>(reinterpret_cast<const simd::Float128*>(kFeedbackDelays.data()));
        auto delay1 = feedback_delays[0] * (fs_ / kBaseSampleRate * size_mult);
        auto delay2 = feedback_delays[1] * (fs_ / kBaseSampleRate * size_mult);
        auto delay3 = feedback_delays[2] * (fs_ / kBaseSampleRate * size_mult);
        auto delay4 = feedback_delays[3] * (fs_ / kBaseSampleRate * size_mult);

        auto current_chorus_amount = chorus_amount_;
        chorus_amount_ = simd::Broadcast<SimdT>(param_.chorus_amount * kMaxChorusDrift * fs_ratio_);
        chorus_amount_ = simd::Min(chorus_amount_, delay1 - 8 * simd::LaneSize<simd::Float128>);
        chorus_amount_ = simd::Min(chorus_amount_, delay2 - 8 * simd::LaneSize<simd::Float128>);
        chorus_amount_ = simd::Min(chorus_amount_, delay3 - 8 * simd::LaneSize<simd::Float128>);
        chorus_amount_ = simd::Min(chorus_amount_, delay4 - 8 * simd::LaneSize<simd::Float128>);
        auto delta_chorus_amount = (chorus_amount_ - current_chorus_amount) * tick_increment;
        current_chorus_amount = current_chorus_amount * size_mult;

        float current_sample_delay = sample_delay_;
        float current_delay_increment = sample_delay_increment_;
        float end_target = current_sample_delay + current_delay_increment * static_cast<float>(num_samples);
        float target_delay = std::max(kMinDelay, param_.pre_delay * fs_ / 1000.0f);
        target_delay = std::lerp(sample_delay_, target_delay, kSampleDelayMultiplier);
        float makeup_delay = target_delay - end_target;
        float delta_delay_increment =
            makeup_delay / (0.5f * static_cast<float>(num_samples * num_samples)) * kSampleIncrementMultiplier;

        auto feedback_offset1 = feedback_offsets_[0];
        auto feedback_offset2 = feedback_offsets_[1];
        auto feedback_offset3 = feedback_offsets_[2];
        auto feedback_offset4 = feedback_offsets_[3];

        float input_gain = param_.freeze ? 0.0f : 1.0f;

        for (int i = 0; i < num_samples; ++i) {
            // paralle chorus delaylines
            current_chorus_amount += delta_chorus_amount;
            current_chorus_real =
                current_chorus_real * chorus_increment_real - current_chorus_imaginary * chorus_increment_imaginary;
            current_chorus_imaginary =
                current_chorus_imaginary * chorus_increment_real + current_chorus_real * chorus_increment_imaginary;
            auto new_feedback_offset1 = delay1 + current_chorus_real * current_chorus_amount;
            auto new_feedback_offset2 = delay2 - current_chorus_real * current_chorus_amount;
            auto new_feedback_offset3 = delay3 + current_chorus_imaginary * current_chorus_amount;
            auto new_feedback_offset4 = delay4 - current_chorus_imaginary * current_chorus_amount;
            feedback_offset1 += (feedback_offset_smooth_factor_) * (new_feedback_offset1 - feedback_offset1);
            feedback_offset2 += (feedback_offset_smooth_factor_) * (new_feedback_offset2 - feedback_offset2);
            feedback_offset3 += (feedback_offset_smooth_factor_) * (new_feedback_offset3 - feedback_offset3);
            feedback_offset4 += (feedback_offset_smooth_factor_) * (new_feedback_offset4 - feedback_offset4);

            auto feedback_read1 = ReadFeedback(0, feedback_offset1);
            auto feedback_read2 = ReadFeedback(1, feedback_offset2);
            auto feedback_read3 = ReadFeedback(2, feedback_offset3);
            auto feedback_read4 = ReadFeedback(3, feedback_offset4);

            simd::Float128 input;
            if constexpr (kMono) {
                input = simd::Float128{left[i], left[i], left[i], left[i]};
            }
            else {
                input = simd::Float128{left[i], right[i], left[i], right[i]};
            }
            auto filtered_input =
                high_pre_filter_.TickLowpass(input * input_gain, simd::BroadcastF128(current_high_pre_coefficient));
            filtered_input =
                low_pre_filter_.TickLowpass(input, simd::BroadcastF128(current_low_pre_coefficient)) - filtered_input;
            auto scaled_input = filtered_input * 0.5f;

            // paralle polyphase allpass
            auto allpass_read1 = ReadAllpass(0, allpass_offset1);
            auto allpass_read2 = ReadAllpass(1, allpass_offset2);
            auto allpass_read3 = ReadAllpass(2, allpass_offset3);
            auto allpass_read4 = ReadAllpass(3, allpass_offset4);

            auto allpass_delay_input1 = feedback_read1 - allpass_read1 * kAllpassFeedback;
            auto allpass_delay_input2 = feedback_read2 - allpass_read2 * kAllpassFeedback;
            auto allpass_delay_input3 = feedback_read3 - allpass_read3 * kAllpassFeedback;
            auto allpass_delay_input4 = feedback_read4 - allpass_read4 * kAllpassFeedback;

            allpass_lookups_[0][static_cast<size_t>(allpass_write_pos_)] = scaled_input + allpass_delay_input1;
            allpass_lookups_[1][static_cast<size_t>(allpass_write_pos_)] = scaled_input + allpass_delay_input2;
            allpass_lookups_[2][static_cast<size_t>(allpass_write_pos_)] = scaled_input + allpass_delay_input3;
            allpass_lookups_[3][static_cast<size_t>(allpass_write_pos_)] = scaled_input + allpass_delay_input4;
            allpass_write_pos_ = (allpass_write_pos_ + 1) & poly_allpass_mask_;

            auto allpass_output1 = allpass_read1 + allpass_delay_input1 * kAllpassFeedback;
            auto allpass_output2 = allpass_read2 + allpass_delay_input2 * kAllpassFeedback;
            auto allpass_output3 = allpass_read3 + allpass_delay_input3 * kAllpassFeedback;
            auto allpass_output4 = allpass_read4 + allpass_delay_input4 * kAllpassFeedback;

            // scatter matrix
            // write1 = 0.25 * Sum(All) + Ao0 - 0.5 * Sum(Ao0) - 0.5 * (Ao0 + Ao1 + Ao2 + Ao3)
            // write2 = 0.25 * Sum(All) + Ao1 - 0.5 * Sum(Ao1) - 0.5 * (Ao0 + Ao1 + Ao2 + Ao3)
            // write3 = 0.25 * Sum(All) + Ao2 - 0.5 * Sum(Ao2) - 0.5 * (Ao0 + Ao1 + Ao2 + Ao3)
            // write4 = 0.25 * Sum(All) + Ao3 - 0.5 * Sum(Ao3) - 0.5 * (Ao0 + Ao1 + Ao2 + Ao3)
            auto total_rows = allpass_output1 + allpass_output2 + allpass_output3 + allpass_output4;
            auto other_feedback = total_rows * (-0.5f) + (simd::ReduceAdd(total_rows) * 0.25f);

            auto write1 = other_feedback + allpass_output1;
            auto write2 = other_feedback + allpass_output2;
            auto write3 = other_feedback + allpass_output3;
            auto write4 = other_feedback + allpass_output4;

            alignas(16) auto [t1, t2, t3, t4] =
                simd::Transpose(allpass_output1, allpass_output2, allpass_output3, allpass_output4);
            auto adjacent_feedback = (t1 + t2 + t3 + t4) * (-0.5f);
            write1 += adjacent_feedback[0];
            write2 += adjacent_feedback[1];
            write3 += adjacent_feedback[2];
            write4 += adjacent_feedback[3];

            // damp filter
            auto high_filtered1 =
                high_shelf_filters_[0].TickLowpass(write1, simd::BroadcastF128(current_high_coefficient));
            auto high_filtered2 =
                high_shelf_filters_[1].TickLowpass(write2, simd::BroadcastF128(current_high_coefficient));
            auto high_filtered3 =
                high_shelf_filters_[2].TickLowpass(write3, simd::BroadcastF128(current_high_coefficient));
            auto high_filtered4 =
                high_shelf_filters_[3].TickLowpass(write4, simd::BroadcastF128(current_high_coefficient));
            write1 = high_filtered1 + (current_high_amplitude) * (write1 - high_filtered1);
            write2 = high_filtered2 + (current_high_amplitude) * (write2 - high_filtered2);
            write3 = high_filtered3 + (current_high_amplitude) * (write3 - high_filtered3);
            write4 = high_filtered4 + (current_high_amplitude) * (write4 - high_filtered4);

            auto low_filtered1 =
                low_shelf_filters_[0].TickLowpass(write1, simd::BroadcastF128(current_low_coefficient));
            auto low_filtered2 =
                low_shelf_filters_[1].TickLowpass(write2, simd::BroadcastF128(current_low_coefficient));
            auto low_filtered3 =
                low_shelf_filters_[2].TickLowpass(write3, simd::BroadcastF128(current_low_coefficient));
            auto low_filtered4 =
                low_shelf_filters_[3].TickLowpass(write4, simd::BroadcastF128(current_low_coefficient));
            write1 -= low_filtered1 * (current_low_amplitude);
            write2 -= low_filtered2 * (current_low_amplitude);
            write3 -= low_filtered3 * (current_low_amplitude);
            write4 -= low_filtered4 * (current_low_amplitude);

            // decay block
            current_decay1 += delta_decay1;
            current_decay2 += delta_decay2;
            current_decay3 += delta_decay3;
            current_decay4 += delta_decay4;
            auto store1 = current_decay1 * write1;
            auto store2 = current_decay2 * write2;
            auto store3 = current_decay3 * write3;
            auto store4 = current_decay4 * write4;
            write_index_ = (write_index_ + 1) & feedback_mask_;
            feedback_ptrs_[0][write_index_] = store1[0];
            feedback_ptrs_[1][write_index_] = store1[1];
            feedback_ptrs_[2][write_index_] = store1[2];
            feedback_ptrs_[3][write_index_] = store1[3];
            feedback_ptrs_[4][write_index_] = store2[0];
            feedback_ptrs_[5][write_index_] = store2[1];
            feedback_ptrs_[6][write_index_] = store2[2];
            feedback_ptrs_[7][write_index_] = store2[3];
            feedback_ptrs_[8][write_index_] = store3[0];
            feedback_ptrs_[9][write_index_] = store3[1];
            feedback_ptrs_[10][write_index_] = store3[2];
            feedback_ptrs_[11][write_index_] = store3[3];
            feedback_ptrs_[12][write_index_] = store4[0];
            feedback_ptrs_[13][write_index_] = store4[1];
            feedback_ptrs_[14][write_index_] = store4[2];
            feedback_ptrs_[15][write_index_] = store4[3];

            // scatter matrix
            auto total_allpass = store1 + store2 + store3 + store4;
            auto other_feedback_allpass = total_allpass * (-0.5f) + (simd::ReduceAdd(total_allpass) * 0.25f);

            auto feed_forward1 = other_feedback_allpass + store1;
            auto feed_forward2 = other_feedback_allpass + store2;
            auto feed_forward3 = other_feedback_allpass + store3;
            auto feed_forward4 = other_feedback_allpass + store4;

            alignas(16) auto [s1, s2, s3, s4] = simd::Transpose(store1, store2, store3, store4);
            auto adjacent_feedback_allpass = (s1 + s2 + s3 + s4) * (-0.5f);

            feed_forward1 += (adjacent_feedback_allpass[0]);
            feed_forward2 += (adjacent_feedback_allpass[1]);
            feed_forward3 += (adjacent_feedback_allpass[2]);
            feed_forward4 += (adjacent_feedback_allpass[3]);

            // predelay
            auto total = write1 + write2 + write3 + write4;
            total += (feed_forward1 * current_decay1 + feed_forward2 * current_decay2 + feed_forward3 * current_decay3
                      + feed_forward4 * current_decay4)
                   * (0.125f);

            simd::Float128 output{total[0] + total[2], total[1] + total[3]};
            predelay_.Push(output);
            auto audio_out = current_wet * predelay_.GetAfterPush(current_sample_delay) + current_dry * input;
            left[i] = audio_out[0];
            if constexpr (!kMono) {
                right[i] = audio_out[1];
            }

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

    template <bool kMono, class T>
        requires std::same_as<T, simd::Float256>
    void ProcessInternal(float* left, float* right, int num_samples) noexcept {
        WarpBuffer();

        float const tick_increment = 1.0f / static_cast<float>(num_samples);

        float current_dry = dry_;
        float current_wet = wet_;
        float current_low_pre_coefficient = low_pre_coefficient_;
        float current_high_pre_coefficient = high_pre_coefficient_;
        float current_low_coefficient = low_coefficient_;
        float current_low_amplitude = low_amplitude_;
        float current_high_coefficient = high_coefficient_;
        float current_high_amplitude = high_amplitude_;

        wet_ = std::sin(param_.wet * std::numbers::pi_v<float> / 2);
        dry_ = std::cos(param_.wet * std::numbers::pi_v<float> / 2);
        float delta_wet = (wet_ - current_wet) * tick_increment;
        float delta_dry = (dry_ - current_dry) * tick_increment;

        float const low_pre_cutoff_frequency = qwqdsp::convert::Pitch2Freq(param_.pre_lowpass);
        low_pre_coefficient_ =
            decltype(low_pre_filter_)::ComputeCoeff(qwqdsp::convert::Freq2W(low_pre_cutoff_frequency, fs_));
        float const high_pre_cutoff_frequency = qwqdsp::convert::Pitch2Freq(param_.pre_highpass);
        high_pre_coefficient_ =
            decltype(high_pre_filter_)::ComputeCoeff(qwqdsp::convert::Freq2W(high_pre_cutoff_frequency, fs_));
        float delta_low_pre_coefficient = (low_pre_coefficient_ - current_low_pre_coefficient) * tick_increment;
        float delta_high_pre_coefficient = (high_pre_coefficient_ - current_high_pre_coefficient) * tick_increment;

        float const low_cutoff_frequency = qwqdsp::convert::Pitch2Freq(param_.low_damp_pitch);
        low_coefficient_ = decltype(low_pre_filter_)::ComputeCoeff(qwqdsp::convert::Freq2W(low_cutoff_frequency, fs_));
        float const high_cutoff_frequency = qwqdsp::convert::Pitch2Freq(param_.high_damp_pitch);
        high_coefficient_ =
            decltype(high_pre_filter_)::ComputeCoeff(qwqdsp::convert::Freq2W(high_cutoff_frequency, fs_));
        float delta_low_coefficient = (low_coefficient_ - current_low_coefficient) * tick_increment;
        float delta_high_coefficient = (high_coefficient_ - current_high_coefficient) * tick_increment;

        low_amplitude_ = 1.0f - qwqdsp::convert::Db2Gain(param_.low_damp_db);
        high_amplitude_ = qwqdsp::convert::Db2Gain(param_.high_damp_db);
        float delta_low_amplitude = (low_amplitude_ - current_low_amplitude) * tick_increment;
        float delta_high_amplitude = (high_amplitude_ - current_high_amplitude) * tick_increment;

        float const size_mult = std::exp2(param_.size * kSizePowerRange + kMinSizePower);

        float const decay_samples = param_.decay_ms * kBaseSampleRate / 1000.0f;
        float const decay_period = size_mult / decay_samples;
        simd::Float256 current_decay1 = decays_[0];
        simd::Float256 current_decay2 = decays_[1];
        if (param_.freeze) {
            decays_.fill(simd::BroadcastF256(1.0f));
        }
        else {
            for (size_t j = 0; j < kContainerSize; ++j) {
                for (size_t i = 0; i < 8; ++i) {
                    decays_[j][i] = std::pow(kT60Amplitude, kFeedbackDelays[j * 8 + i] * decay_period);
                }
            }
        }
        auto delta_decay1 = (decays_[0] - current_decay1) * tick_increment;
        auto delta_decay2 = (decays_[1] - current_decay2) * tick_increment;

        const simd::Int256* allpass_delays = std::assume_aligned<32>(reinterpret_cast<const simd::Int256*>(kAllpassDelays.data()));
        auto allpass_offset1 = simd::ToInt(simd::ToFloat(allpass_delays[0]) * buffer_scale_ratio_);
        auto allpass_offset2 = simd::ToInt(simd::ToFloat(allpass_delays[1]) * buffer_scale_ratio_);

        float const chorus_phase_increment = param_.chorus_freq / fs_;

        float const network_offset = 2.0f * std::numbers::pi_v<float> / kNetworkSize;
        auto phase_offset = simd::Float128{0.0f, 1.0f, 2.0f, 3.0f} * network_offset;
        auto container_phase = phase_offset + chorus_phase_ * 2.0f * std::numbers::pi_v<float>;
        chorus_phase_ += static_cast<float>(num_samples) * chorus_phase_increment;
        chorus_phase_ -= std::floor(chorus_phase_);

        auto chorus_increment_real =
            simd::BroadcastF128(std::cos(chorus_phase_increment * (2.0f * std::numbers::pi_v<float>)));
        auto chorus_increment_imaginary =
            simd::BroadcastF128(std::sin(chorus_phase_increment * (2.0f * std::numbers::pi_v<float>)));
        simd::Float128 current_chorus_real{};
        simd::Float128 current_chorus_imaginary{};
        for (size_t i = 0; i < 4; ++i) {
            current_chorus_real[i] = std::cos(container_phase[i]);
        }
        for (size_t i = 0; i < 4; ++i) {
            current_chorus_imaginary[i] = std::sin(container_phase[i]);
        }

        const simd::Float256* feedback_delays = std::assume_aligned<32>(reinterpret_cast<const simd::Float256*>(kFeedbackDelays.data()));
        auto delay1 = feedback_delays[0] * (fs_ / kBaseSampleRate * size_mult);
        auto delay2 = feedback_delays[1] * (fs_ / kBaseSampleRate * size_mult);

        auto current_chorus_amount = chorus_amount_;
        chorus_amount_ = simd::BroadcastF256(param_.chorus_amount * kMaxChorusDrift * fs_ratio_);
        chorus_amount_ = simd::Min(chorus_amount_, delay1 - 8 * simd::LaneSize<simd::Float128>);
        chorus_amount_ = simd::Min(chorus_amount_, delay2 - 8 * simd::LaneSize<simd::Float128>);
        auto delta_chorus_amount = (chorus_amount_ - current_chorus_amount) * tick_increment;
        current_chorus_amount = current_chorus_amount * size_mult;

        float current_sample_delay = sample_delay_;
        float current_delay_increment = sample_delay_increment_;
        float end_target = current_sample_delay + current_delay_increment * static_cast<float>(num_samples);
        float target_delay = std::max(kMinDelay, param_.pre_delay * fs_ / 1000.0f);
        target_delay = std::lerp(sample_delay_, target_delay, kSampleDelayMultiplier);
        float makeup_delay = target_delay - end_target;
        float delta_delay_increment =
            makeup_delay / (0.5f * static_cast<float>(num_samples * num_samples)) * kSampleIncrementMultiplier;

        auto feedback_offset1 = feedback_offsets_[0];
        auto feedback_offset2 = feedback_offsets_[1];
        float const feedback_smooth = feedback_offset_smooth_factor_;

        float input_gain = param_.freeze ? 0.0f : 1.0f;

        for (int i = 0; i < num_samples; ++i) {
            // paralle chorus delaylines
            current_chorus_amount += delta_chorus_amount;
            auto const prev_chorus_real = current_chorus_real;
            current_chorus_real =
                prev_chorus_real * chorus_increment_real - current_chorus_imaginary * chorus_increment_imaginary;
            current_chorus_imaginary =
                current_chorus_imaginary * chorus_increment_real + prev_chorus_real * chorus_increment_imaginary;
            auto new_feedback_offset1 =
                delay1 + simd::Combine(current_chorus_real, -current_chorus_real) * current_chorus_amount;
            auto new_feedback_offset2 =
                delay2 + simd::Combine(current_chorus_imaginary, -current_chorus_imaginary) * current_chorus_amount;
            feedback_offset1 += feedback_smooth * (new_feedback_offset1 - feedback_offset1);
            feedback_offset2 += feedback_smooth * (new_feedback_offset2 - feedback_offset2);

            auto feedback_read1 = ReadFeedback(0, feedback_offset1);
            auto feedback_read2 = ReadFeedback(1, feedback_offset2);

            simd::Float128 input;
            if constexpr (kMono) {
                input = simd::Float128{left[i], left[i], left[i], left[i]};
            }
            else {
                input = simd::Float128{left[i], right[i], left[i], right[i]};
            }
            auto pre_high_coeff_v = simd::BroadcastF128(current_high_pre_coefficient);
            auto pre_low_coeff_v = simd::BroadcastF128(current_low_pre_coefficient);
            auto filtered_input = high_pre_filter_.TickLowpass(input * input_gain, pre_high_coeff_v);
            filtered_input = low_pre_filter_.TickLowpass(input, pre_low_coeff_v) - filtered_input;
            auto scaled_input = simd::Combine(filtered_input, filtered_input) * 0.5f;

            // paralle polyphase allpass
            auto allpass_read1 = ReadAllpass(0, allpass_offset1);
            auto allpass_read2 = ReadAllpass(1, allpass_offset2);

            auto allpass_delay_input1 = feedback_read1 - allpass_read1 * kAllpassFeedback;
            auto allpass_delay_input2 = feedback_read2 - allpass_read2 * kAllpassFeedback;

            allpass_lookups_[0][static_cast<size_t>(allpass_write_pos_)] = scaled_input + allpass_delay_input1;
            allpass_lookups_[1][static_cast<size_t>(allpass_write_pos_)] = scaled_input + allpass_delay_input2;
            allpass_write_pos_ = (allpass_write_pos_ + 1) & poly_allpass_mask_;

            auto allpass_output1 = allpass_read1 + allpass_delay_input1 * kAllpassFeedback;
            auto allpass_output2 = allpass_read2 + allpass_delay_input2 * kAllpassFeedback;

            // scatter matrix
            simd::Float256 write1;
            simd::Float256 write2;
            _ScatterLane8(allpass_output1, allpass_output2, write1, write2);

            // damp filter
            auto high_coeff_v = simd::BroadcastF256(current_high_coefficient);
            auto low_coeff_v = simd::BroadcastF256(current_low_coefficient);
            auto high_filtered1 = high_shelf_filters_[0].TickLowpass(write1, high_coeff_v);
            auto high_filtered2 = high_shelf_filters_[1].TickLowpass(write2, high_coeff_v);
            write1 = high_filtered1 + (current_high_amplitude) * (write1 - high_filtered1);
            write2 = high_filtered2 + (current_high_amplitude) * (write2 - high_filtered2);

            auto low_filtered1 = low_shelf_filters_[0].TickLowpass(write1, low_coeff_v);
            auto low_filtered2 = low_shelf_filters_[1].TickLowpass(write2, low_coeff_v);
            write1 -= low_filtered1 * (current_low_amplitude);
            write2 -= low_filtered2 * (current_low_amplitude);

            // decay block
            current_decay1 += delta_decay1;
            current_decay2 += delta_decay2;
            auto store1 = current_decay1 * write1;
            auto store2 = current_decay2 * write2;
            write_index_ = (write_index_ + 1) & feedback_mask_;
            PushFeedback(store1, store2);

            // scatter matrix
            simd::Float256 feed_forward1;
            simd::Float256 feed_forward2;
            _ScatterLane8(store1, store2, feed_forward1, feed_forward2);

            // predelay
            auto total = write1 + write2;
            total += (feed_forward1 * current_decay1 + feed_forward2 * current_decay2) * (0.125f);

            simd::Float128 output{total[0] + total[2] + total[4] + total[6], total[1] + total[3] + total[5] + total[7]};
            predelay_.Push(output);
            auto audio_out = current_wet * predelay_.GetAfterPush(current_sample_delay) + current_dry * input;
            left[i] = audio_out[0];
            if constexpr (!kMono) {
                right[i] = audio_out[1];
            }

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
    }

    void WarpBuffer() noexcept {
#ifndef SIMD_HAS_AVX2
        for (auto ptr : feedback_ptrs_) {
            ptr[max_feedback_size_] = ptr[0];
            ptr[max_feedback_size_ + 1] = ptr[1];
            ptr[max_feedback_size_ + 2] = ptr[2];
            ptr[max_feedback_size_ + 3] = ptr[3];
        }
#else
        size_t raw_offset = max_feedback_size_ * kNetworkSize;
        float* dst = feedback_memorie_.data() + raw_offset;
        float* src = feedback_memorie_.data();
        for (int i = 0; i < kNetworkSize / 8 * 4; ++i) {
            _mm256_store_ps(dst, _mm256_load_ps(src));
            src += 8;
            dst += 8;
        }
#endif
    }

    simd::Float128 ReadFeedback(size_t idx, simd::Float128 offset) noexcept {
        simd::Float128 rpos = (static_cast<float>(write_index_ + feedback_mask_)) - offset;
        auto irpos = (simd::ToInt(rpos) - 1) & feedback_mask_;
        simd::Float128 t = simd::Frac(rpos);

        // load [-1, 0, 1, 2]
        alignas(16) auto [yn1, y0, y1, y2] = simd::Transpose(simd::Loadu128(feedback_ptrs_[idx * 4] + irpos[0]),
                                                             simd::Loadu128(feedback_ptrs_[idx * 4 + 1] + irpos[1]),
                                                             simd::Loadu128(feedback_ptrs_[idx * 4 + 2] + irpos[2]),
                                                             simd::Loadu128(feedback_ptrs_[idx * 4 + 3] + irpos[3]));

        auto d0 = (y1 - yn1) * (0.5f);
        auto d1 = (y2 - y0) * (0.5f);
        auto d = y1 - y0;
        auto m0 = (3.0f) * d - (2.0f) * d0 - d1;
        auto m1 = d0 - (2.0f) * d + d1;
        return y0 + t * (d0 + t * (m0 + t * m1));
    }

    simd::Float256 ReadFeedback(int idx, simd::Float256 offset) noexcept {
        simd::Float256 rpos = (static_cast<float>(write_index_ + feedback_mask_)) - offset;
        auto irpos = (simd::ToInt(rpos) - 1) & feedback_mask_;
        simd::Float256 t = simd::Frac(rpos);

#ifndef SIMD_HAS_AVX2
        // load [-1, 0, 1, 2]
        alignas(32) auto [yn1, y0, y1, y2] = simd::Transpose256(simd::Loadu128(feedback_ptrs_[idx * 8] + irpos[0]),
                                                                simd::Loadu128(feedback_ptrs_[idx * 8 + 1] + irpos[1]),
                                                                simd::Loadu128(feedback_ptrs_[idx * 8 + 2] + irpos[2]),
                                                                simd::Loadu128(feedback_ptrs_[idx * 8 + 3] + irpos[3]),
                                                                simd::Loadu128(feedback_ptrs_[idx * 8 + 4] + irpos[4]),
                                                                simd::Loadu128(feedback_ptrs_[idx * 8 + 5] + irpos[5]),
                                                                simd::Loadu128(feedback_ptrs_[idx * 8 + 6] + irpos[6]),
                                                                simd::Loadu128(feedback_ptrs_[idx * 8 + 7] + irpos[7]));

        auto d0 = (y1 - yn1) * (0.5f);
        auto d1 = (y2 - y0) * (0.5f);
        auto d = y1 - y0;
        auto m0 = (3.0f) * d - (2.0f) * d0 - d1;
        auto m1 = d0 - (2.0f) * d + d1;
        return y0 + t * (d0 + t * (m0 + t * m1));
#else
        __m256i lane_ids = _mm256_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7);
        __m256i base_vindex = _mm256_add_epi32(_mm256_slli_epi32((__m256i)(irpos), 4),
                                               _mm256_add_epi32(_mm256_set1_epi32(idx * 8), lane_ids));

        float const* raw = feedback_memorie_.data();
        auto yn1 = (simd::Float256)(_mm256_i32gather_ps(raw, base_vindex, 4));
        auto y0 = (simd::Float256)(_mm256_i32gather_ps(raw, _mm256_add_epi32(base_vindex, _mm256_set1_epi32(16)), 4));
        auto y1 = (simd::Float256)(_mm256_i32gather_ps(raw, _mm256_add_epi32(base_vindex, _mm256_set1_epi32(32)), 4));
        auto y2 = (simd::Float256)(_mm256_i32gather_ps(raw, _mm256_add_epi32(base_vindex, _mm256_set1_epi32(48)), 4));

        auto d0 = (y1 - yn1) * (0.5f);
        auto d1 = (y2 - y0) * (0.5f);
        auto d = y1 - y0;
        auto m0 = (3.0f) * d - (2.0f) * d0 - d1;
        auto m1 = d0 - (2.0f) * d + d1;
        return y0 + t * (d0 + t * (m0 + t * m1));
#endif
    }

    void PushFeedback(simd::Float256 store1, simd::Float256 store2) noexcept {
#ifndef SIMD_HAS_AVX2
        int const write_idx = write_index_;
        auto* const ptrs = feedback_ptrs_.data();
        ptrs[0][write_idx] = store1[0];
        ptrs[1][write_idx] = store1[1];
        ptrs[2][write_idx] = store1[2];
        ptrs[3][write_idx] = store1[3];
        ptrs[4][write_idx] = store1[4];
        ptrs[5][write_idx] = store1[5];
        ptrs[6][write_idx] = store1[6];
        ptrs[7][write_idx] = store1[7];
        ptrs[8][write_idx] = store2[0];
        ptrs[9][write_idx] = store2[1];
        ptrs[10][write_idx] = store2[2];
        ptrs[11][write_idx] = store2[3];
        ptrs[12][write_idx] = store2[4];
        ptrs[13][write_idx] = store2[5];
        ptrs[14][write_idx] = store2[6];
        ptrs[15][write_idx] = store2[7];
#else
        size_t offset = write_index_ * 16;
        float* ptr = feedback_memorie_.data() + offset;
        _mm256_store_ps(ptr, (__m256)(store1));
        _mm256_store_ps(ptr + 8, (__m256)(store2));
#endif
    }

    simd::Float128 ReadAllpass(size_t i, simd::Int128 offset) noexcept {
        auto& buffer = allpass_lookups_[i];
        auto irpos = (allpass_write_pos_ + poly_allpass_mask_) - offset;
        irpos &= (poly_allpass_mask_);
        auto const* raw = reinterpret_cast<const float*>(buffer.data());
        return simd::Float128{raw[static_cast<size_t>(irpos[0]) * 4 + 0], raw[static_cast<size_t>(irpos[1]) * 4 + 1],
                              raw[static_cast<size_t>(irpos[2]) * 4 + 2], raw[static_cast<size_t>(irpos[3]) * 4 + 3]};
    }

    simd::Float256 ReadAllpass(size_t i, simd::Int256 offset) noexcept {
        auto& buffer = allpass_lookups_[i];
        auto irpos = (allpass_write_pos_ + poly_allpass_mask_) - offset;
        irpos &= (poly_allpass_mask_);
#ifndef SIMD_HAS_AVX2
        auto const* raw = reinterpret_cast<const float*>(buffer.data());
        return simd::Float256{raw[static_cast<size_t>(irpos[0]) * 8 + 0], raw[static_cast<size_t>(irpos[1]) * 8 + 1],
                              raw[static_cast<size_t>(irpos[2]) * 8 + 2], raw[static_cast<size_t>(irpos[3]) * 8 + 3],
                              raw[static_cast<size_t>(irpos[4]) * 8 + 4], raw[static_cast<size_t>(irpos[5]) * 8 + 5],
                              raw[static_cast<size_t>(irpos[6]) * 8 + 6], raw[static_cast<size_t>(irpos[7]) * 8 + 7]};
#else
        static const int32_t lane_offsets_data[8] = {0, 1, 2, 3, 4, 5, 6, 7};
        __m256i lane_offsets = _mm256_loadu_si256((const __m256i_u*)lane_offsets_data);
        __m256i vindex = _mm256_add_epi32(_mm256_slli_epi32((__m256i)(irpos), 3), lane_offsets);
        float const* raw = reinterpret_cast<const float*>(buffer.data());
        return (simd::Float256)(_mm256_i32gather_ps(raw, vindex, 4));
#endif
    }

    // Float256(Ao0, Ao1) -> Float256(ReduceAdd(Ao0), ReduceAdd(Ao1))
    static simd::Float256 _InternalSum(simd::Float256 x) noexcept {
#ifdef SIMD_HAS_AVX2
        __m256 t1 = _mm256_add_ps(x, _mm256_permute_ps(x, _MM_SHUFFLE(2, 3, 0, 1)));
        __m256 y = _mm256_add_ps(t1, _mm256_permute_ps(t1, _MM_SHUFFLE(1, 0, 3, 2)));
        return y;
#else
        float s0 = x[0] + x[1] + x[2] + x[3];
        float s1 = x[4] + x[5] + x[6] + x[7];
        return simd::Float256{s0, s0, s0, s0, s1, s1, s1, s1};
#endif
    }

    static void _ScatterLane8(simd::Float256 in1, simd::Float256 in2, simd::Float256& out1,
                                     simd::Float256& out2) noexcept {
        alignas(32) auto [ao1, ao2] = simd::Break(in1);
        alignas(32) auto [ao3, ao4] = simd::Break(in2);
        auto row_sum = ao1 + ao2 + ao3 + ao4;
        auto total_rows = simd::Combine(row_sum, row_sum);
        auto sum_all = simd::ReduceAdd(in1 + in2);
        auto sum_ao01 = _InternalSum(in1);
        auto sum_ao23 = _InternalSum(in2);
        auto common = 0.25f * sum_all;
        out1 = common + in1 - 0.5f * (sum_ao01 + total_rows);
        out2 = common + in2 - 0.5f * (sum_ao23 + total_rows);
    }

    static constexpr int kContainerSize = kNetworkSize / simd::LaneSize<SimdT>;

    Param param_;

    pluginshared::dsp::DelayLineSingle<inst, simd::Float128> predelay_;

    simd::Array<std::vector<SimdT>, kContainerSize> allpass_lookups_{};

    std::vector<float, simd::AlignedAllocator<float, 32>> feedback_memorie_;
    std::array<float*, kNetworkSize> feedback_ptrs_{};
    simd::Array<SimdT, kContainerSize> feedback_offsets_{};

    simd::Array<SimdT, kContainerSize> decays_{};

    simd::Array<pluginshared::dsp::OnePoleTPT<inst, SimdT>, kContainerSize> low_shelf_filters_;
    simd::Array<pluginshared::dsp::OnePoleTPT<inst, SimdT>, kContainerSize> high_shelf_filters_;

    pluginshared::dsp::OnePoleTPT<inst, simd::Float128> low_pre_filter_;
    pluginshared::dsp::OnePoleTPT<inst, simd::Float128> high_pre_filter_;

    float low_pre_coefficient_{};
    float high_pre_coefficient_{};
    float low_coefficient_{};
    float low_amplitude_{};
    float high_coefficient_{};
    float high_amplitude_{};
    float feedback_offset_smooth_factor_{};

    float chorus_phase_{};
    SimdT chorus_amount_{};
    float sample_delay_{};
    float sample_delay_increment_{};
    float dry_{};
    float wet_{};
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

} // namespace vital_reverb

#pragma GCC diagnostic pop
