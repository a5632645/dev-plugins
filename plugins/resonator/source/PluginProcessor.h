#pragma once
#include "../../shared/juce_param_listener.hpp"
#include <complex>

#include "qwqdsp/convert.hpp"
#include "qwqdsp/filter/one_pole_tpt_simd.hpp"
#include "qwqdsp/psimd/vec4.hpp"

static constexpr size_t kNumResonators = 8;

using SimdType = qwqdsp::psimd::Vec4f32;
using SimdIntType = qwqdsp::psimd::Vec4i32;

static SimdType Sin(SimdType const& w) {
    return SimdType{
        std::sin(w.x[0]),
        std::sin(w.x[1]),
        std::sin(w.x[2]),
        std::sin(w.x[3])
    };
}

static SimdType Cos(SimdType const& w) {
    return SimdType{
        std::cos(w.x[0]),
        std::cos(w.x[1]),
        std::cos(w.x[2]),
        std::cos(w.x[3])
    };
}

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
        SimdType temp = (SimdType::FromSingle(1) - delay) / (SimdType::FromSingle(1) + delay);
        SimdType zero = SimdType::FromSingle(0);
        a1_ = SimdType{
            delay.x[0] < 1.0f ? zero.x[0] : temp.x[0],
            delay.x[1] < 1.0f ? zero.x[1] : temp.x[1],
            delay.x[2] < 1.0f ? zero.x[2] : temp.x[2],
            delay.x[3] < 1.0f ? zero.x[3] : temp.x[3]
        };
    }

    void SetGroupDelay(size_t idx, float delay) noexcept {
        if (delay < 1.0f) {
            a1_.x[idx] = 0.0f;
        }
        else {
            a1_.x[idx] = (1.0f - delay) / (1.0f + delay);
        }
    }

    SimdType GetPhaseDelay(SimdType const& w) const noexcept {
        SimdType r;
        for (size_t i= 0; i < SimdType::kSize; ++i) {
            auto z = std::polar(1.0f, w.x[i]);
            auto up = a1_.x[i] * z + 1.0f;
            auto down = z + a1_.x[i];
            float phase = std::arg(up / down);
            r.x[i] = -phase * kNumAPF / w.x[i];
        }
        return r;
    }

    float GetPhaseDelay(size_t idx, float w) const noexcept {
        auto z = std::polar(1.0f, w);
        auto up = a1_.x[idx] * z + 1.0f;
        auto down = z + a1_.x[idx];
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
    SimdIntType SetDelay(SimdType const& delay) noexcept {
        SimdIntType r;
        for (size_t i = 0; i < SimdType::kSize; ++i) {
            // thiran delay limit to 0.5 ~ 1.5
            if (delay.x[i] < 0.5f) {
                alpha_.x[i] = 0.0f; // equal to one delay
                r.x[i] = 0;
            }
            else {
                float intergalPart = std::floor(delay.x[i]);
                float fractionalPart = delay.x[i] - intergalPart;
                if (fractionalPart < 0.5f) {
                    fractionalPart += 1.0f;
                    intergalPart -= 1.0f;
                }
                alpha_.x[i] = (1.0f - fractionalPart) / (1.0f + fractionalPart);
                r.x[i] = static_cast<int>(intergalPart);
            }
        }
        return r;
    }

    int SetDelay(size_t idx, float delay) noexcept {
        // thiran delay limit to 0.5 ~ 1.5
        if (delay < 0.5f) {
            alpha_.x[idx] = 0.0f; // equal to one delay
            return 0;
        }
        else {
            float intergalPart = std::floor(delay);
            float fractionalPart = delay - intergalPart;
            if (fractionalPart < 0.5f) {
                fractionalPart += 1.0f;
                intergalPart -= 1.0f;
            }
            alpha_.x[idx] = (1.0f - fractionalPart) / (1.0f + fractionalPart);
            return static_cast<int>(intergalPart);
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
        delay_mask_ = a - 1;
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
        size_t wpos = delay_wpos_;
        // left processing
        for (size_t i = 0; i < len; ++i) {
            // input
            SimdType output = SimdType::FromSingle(left_ptr[i]);
            SimdType input_1 = input_volume_[0] * output + fb_values_[0];
            SimdType input_2 = input_volume_[1] * output + fb_values_[1];
            output *= SimdType::FromSingle(dry);
            output += fb_values_[0] * output_volume_[0];
            output += fb_values_[1] * output_volume_[1];

            // delayline
            delay_buffer_[0][wpos] = input_1;
            delay_buffer_[1][wpos] = input_2;
            wpos = (wpos + 1) & delay_mask_;
            SimdType delay_out1 = ReadFeedback(0, wpos, 0);
            SimdType delay_out2 = ReadFeedback(1, wpos, 1);

            // dispersion and damp
            delay_out1 = dispersion_[0].Tick(delay_out1);
            delay_out2 = dispersion_[1].Tick(delay_out2);
            delay_out1 = damp_[0].TickHighshelf(delay_out1, damp_highshelf_coeff[0], damp_highshelf_gain[0]);
            delay_out2 = damp_[1].TickHighshelf(delay_out2, damp_highshelf_coeff[1], damp_highshelf_gain[1]);
            delay_out1 = dc_blocker[0].TickHighpass(delay_out1, SimdType::FromSingle(0.0005f));
            delay_out2 = dc_blocker[1].TickHighpass(delay_out2, SimdType::FromSingle(0.0005f));

            // scatter signals
            SimdType scatter_sin = Sin(reflections_[0]);
            SimdType scatter_cos = Cos(reflections_[0]);
            SimdType scatter_a{
                delay_out1.x[0], delay_out1.x[2], delay_out2.x[0], delay_out2.x[2]
            };
            SimdType scatter_b{
                delay_out1.x[1], delay_out1.x[3], delay_out2.x[1], delay_out2.x[3]
            };
            // [0, 2, 4, 6]
            SimdType scatter_outa = scatter_cos * scatter_a - scatter_sin * scatter_b;
            // [1, 3, 5, 7]
            SimdType scatter_outb = scatter_sin * scatter_a + scatter_cos * scatter_b;

            SimdType scatter2_ina = scatter_outb;
            SimdType scatter2_inb = scatter_outa.Shuffle<1, 2, 3, 0>();
            scatter_sin = Sin(reflections_[1]);
            scatter_cos = Cos(reflections_[1]);
            // [1, 3, 5, 7]
            scatter_outa = scatter_cos * scatter2_ina - scatter_sin * scatter2_inb;
            // [2, 4, 6, 0]
            scatter_outb = scatter_sin * scatter2_ina + scatter_cos * scatter2_inb;

            SimdType pipe_a{
                scatter_outb.x[3], scatter_outa.x[0], scatter_outb.x[0], scatter_outa.x[1]
            };
            SimdType pipe_b{
                scatter_outb.x[1], scatter_outa.x[2], scatter_outb.x[2], scatter_outa.x[3]
            };
            
            // write
            fb_values_[0] = feedback_gain_[0] * pipe_a;
            fb_values_[1] = feedback_gain_[1] * pipe_b;

            left_ptr[i] = output.ReduceAdd();
        }

        // right processing
        wpos = delay_wpos_;
        for (size_t i = 0; i < len; ++i) {
            // input
            SimdType output = SimdType::FromSingle(right_ptr[i]);
            SimdType input_1 = input_volume_[0] * output + fb_values_[2];
            SimdType input_2 = input_volume_[1] * output + fb_values_[3];
            output *= SimdType::FromSingle(dry);
            output += fb_values_[2] * output_volume_[0];
            output += fb_values_[3] * output_volume_[1];

            // delayline
            delay_buffer_[2][wpos] = input_1;
            delay_buffer_[3][wpos] = input_2;
            wpos = (wpos + 1) & delay_mask_;
            SimdType delay_out1 = ReadFeedback(2, wpos, 0);
            SimdType delay_out2 = ReadFeedback(3, wpos, 1);

            // dispersion and damp
            delay_out1 = dispersion_[2].Tick(delay_out1);
            delay_out2 = dispersion_[3].Tick(delay_out2);
            delay_out1 = damp_[2].TickHighshelf(delay_out1, damp_highshelf_coeff[0], damp_highshelf_gain[0]);
            delay_out2 = damp_[3].TickHighshelf(delay_out2, damp_highshelf_coeff[1], damp_highshelf_gain[1]);
            delay_out1 = dc_blocker[2].TickHighpass(delay_out1, SimdType::FromSingle(0.0005f));
            delay_out2 = dc_blocker[3].TickHighpass(delay_out2, SimdType::FromSingle(0.0005f));

            // scatter signals
            SimdType scatter_sin = Sin(reflections_[0]);
            SimdType scatter_cos = Cos(reflections_[0]);
            SimdType scatter_a{
                delay_out1.x[0], delay_out1.x[2], delay_out2.x[0], delay_out2.x[2]
            };
            SimdType scatter_b{
                delay_out1.x[1], delay_out1.x[3], delay_out2.x[1], delay_out2.x[3]
            };
            // [0, 2, 4, 6]
            SimdType scatter_outa = scatter_cos * scatter_a - scatter_sin * scatter_b;
            // [1, 3, 5, 7]
            SimdType scatter_outb = scatter_sin * scatter_a + scatter_cos * scatter_b;

            SimdType scatter2_ina = scatter_outb;
            SimdType scatter2_inb = scatter_outa.Shuffle<1, 2, 3, 0>();
            scatter_sin = Sin(reflections_[1]);
            scatter_cos = Cos(reflections_[1]);
            // [1, 3, 5, 7]
            scatter_outa = scatter_cos * scatter2_ina - scatter_sin * scatter2_inb;
            // [2, 4, 6, 0]
            scatter_outb = scatter_sin * scatter2_ina + scatter_cos * scatter2_inb;

            SimdType pipe_a{
                scatter_outb.x[3], scatter_outa.x[0], scatter_outb.x[0], scatter_outa.x[1]
            };
            SimdType pipe_b{
                scatter_outb.x[1], scatter_outa.x[2], scatter_outb.x[2], scatter_outa.x[3]
            };
            
            // write
            fb_values_[2] = feedback_gain_[0] * pipe_a;
            fb_values_[3] = feedback_gain_[1] * pipe_b;
            right_ptr[i] = output.ReduceAdd();
        }

        delay_wpos_ = wpos;
    }

    void UpdateBasicParams() noexcept {
        // output mix volumes
        for (size_t j = 0; j < kMonoContainerSize; ++j) {
            for (size_t i = 0; i < SimdType::kSize; ++i) {
                size_t const param_idx = j * SimdType::kSize + i;
                float const db = mix_db[param_idx];
                if (db < -60.0f) {
                    output_volume_[j].x[i] = 0;
                }
                else {
                    output_volume_[j].x[i] = qwqdsp::convert::Db2Gain(db);
                }
            }
        }

        // update scatter matrix
        for (size_t j = 0; j < kMonoContainerSize; ++j) {
            for (size_t i = 0; i < SimdType::kSize; ++i) {
                size_t const param_idx = j * SimdType::kSize + i;
                reflections_[j].x[i] = norm_reflections[param_idx] * std::numbers::pi_v<float>;
            }
        }

        // update damp filter
        for (size_t j = 0; j < kMonoContainerSize; ++j) {
            SimdType omega;
            for (size_t i = 0; i < SimdType::kSize; ++i) {
                size_t const param_idx = j * SimdType::kSize + i;
                float const freq = qwqdsp::convert::Pitch2Freq(damp_pitch[param_idx]);
                omega.x[i] = freq * std::numbers::pi_v<float> * 2 / fs_;
                damp_highshelf_gain[j].x[i] = qwqdsp::convert::Db2Gain(damp_gain_db[param_idx]);
            }
            damp_highshelf_coeff[j] = qwqdsp::filter::OnePoleTPTSimd<SimdType>::ComputeCoeffs(omega);
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
                omega.x[j] = freq * std::numbers::pi_v<float> / fs_;
                loop_samples.x[j] = fs_ / freq;
                allpass_set_delay.x[j] = loop_samples.x[j] * dispersion[param_idx] / (ThrianDispersion::kNumAPF + 0.1f);
            }
    
            // update allpass filters
            dispersion_[i].SetGroupDelay(allpass_set_delay);
            dispersion_[i + kMonoContainerSize].SetGroupDelay(allpass_set_delay);
            SimdType allpass_delay = dispersion_[i].GetPhaseDelay(omega);
    
            // remove allpass delays
            SimdType delay_samples = loop_samples - allpass_delay;
            delay_samples = SimdType::Max(delay_samples, SimdType::FromSingle(0.0f));
    
            // process frac delays
            delay_samples_[i] = thrian_interp_[i].SetDelay(delay_samples);
            thrian_interp_[i + kMonoContainerSize].SetDelay(delay_samples);

            // feedback decay
            for (size_t j = 0; j < SimdType::kSize; ++j) {
                size_t param_idx = i * SimdType::kSize + j;
                float feedback_gain = 0;
                if (decay_ms[param_idx] > 0.5f) {
                    float const mul = -3.0f * loop_samples.x[j] / (fs_ * decay_ms[param_idx] / 1000.0f);
                    feedback_gain = std::pow(10.0f, mul);
                    feedback_gain = std::min(feedback_gain, 1.0f);
                }
                else {
                    feedback_gain = 0;
                }
                if (polarity[param_idx]) {
                    feedback_gain = -feedback_gain;
                }
                feedback_gain_[i].x[j] = feedback_gain;
            }
        }
    }

    void NoteOn(size_t idx, float pitch, float velocity) noexcept {
        size_t const simd_idx = idx / SimdType::kSize;
        size_t const scalar_idx = idx & (SimdType::kSize - 1);

        pitches[idx] = pitch;
        input_volume_[simd_idx].x[scalar_idx] = velocity;
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
        delay_samples_[simd_idx].x[scalar_idx] = thrian_interp_[simd_idx].SetDelay(scalar_idx, delay_samples);
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
        feedback_gain_[simd_idx].x[scalar_idx] = feedback_gain;
    }

    void TrunOnAllInput(float v) noexcept {
        std::ranges::fill(input_volume_, SimdType::FromSingle(v));
    }

    void Noteoff(size_t idx) noexcept {
        size_t simd_idx = idx / SimdType::kSize;
        size_t scalar_idx = idx & (SimdType::kSize - 1);
        input_volume_[simd_idx].x[scalar_idx] = 0;
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
    SimdType ReadFeedback(size_t idx, size_t wpos, size_t delay_idx) noexcept {
        SimdIntType rpos = SimdIntType::FromSingle(static_cast<int>(wpos + delay_mask_)) - delay_samples_[delay_idx];
        rpos &= SimdIntType::FromSingle(static_cast<int>(delay_mask_));
        SimdType delay_output;
        for (size_t k = 0; k < SimdType::kSize; ++k) {
            delay_output.x[k] = delay_buffer_[idx][static_cast<size_t>(rpos.x[k])].x[k];
        }
        return thrian_interp_[idx].Tick(delay_output);
    }

    static constexpr size_t kContainerSize = 2 * kNumResonators / SimdType::kSize;
    static constexpr size_t kMonoContainerSize = kNumResonators / SimdType::kSize;

    std::array<std::vector<SimdType>, kContainerSize> delay_buffer_;
    size_t delay_wpos_{};
    size_t delay_mask_{};
    std::array<TunningFilter, kContainerSize> thrian_interp_;
    std::array<ThrianDispersion, kContainerSize> dispersion_;
    std::array<qwqdsp::filter::OnePoleTPTSimd<SimdType>, kContainerSize> damp_;
    std::array<qwqdsp::filter::OnePoleTPTSimd<SimdType>, kContainerSize> dc_blocker;
    std::array<SimdType, kMonoContainerSize + 1> input_volume_{};
    std::array<SimdType, kMonoContainerSize> output_volume_{};
    std::array<SimdType, kMonoContainerSize> reflections_{};
    std::array<SimdType, kMonoContainerSize> damp_highshelf_coeff{};
    std::array<SimdType, kMonoContainerSize> damp_highshelf_gain{};
    std::array<SimdType, kMonoContainerSize> feedback_gain_{};
    std::array<SimdIntType, kMonoContainerSize> delay_samples_{};
    std::array<SimdType, kContainerSize> fb_values_{};
    float fs_{};
};

struct Voice
{
    int voiceID;   // 复音的唯一 ID (0 到 kNumResonators - 1)
    int midiNote;  // 当前分配到的 MIDI 音高 (0-127)
    bool isActive; // 是否正在被使用 (NoteOn 状态)
    // 其他您需要的参数 (例如：音量、频率、内部滤波器状态等)
};

class PolyphonyManager {
public:
    PolyphonyManager() {
        initializeVoices();
    }

    /**
     * @brief 处理 NoteOn 事件，分配或替换复音。
     * @param note 要分配的 MIDI 音高。
     * @return 分配到的复音的 ID，如果找不到则返回 -1 (理论上不会发生)。
     */
    int noteOn(int note, bool round_robin) {
        if (round_robin) {
            // 测试循环位
            if (!voices[round_robin_].isActive) {
                int id = activateVoice(static_cast<int>(round_robin_), note);
                ++round_robin_;
                round_robin_ &= (kNumResonators - 1);
                return id;
            }
        }

        // 尝试找到一个空闲的复音
        for (size_t id = 0; id < kNumResonators; ++id) {
            if (!voices[id].isActive) {
                // 找到空闲复音：直接分配
                return activateVoice(static_cast<int>(id), note);
            }
        }

        // --- 所有复音都被占用：FIFO 替换最旧的 ---
        
        // 获取最旧的复音 ID (deque 的头部)
        int oldestVoiceID = activationOrder.front();
        activationOrder.pop_front();

        // 激活新音高并返回 ID
        return activateVoice(oldestVoiceID, note);
    }

    /**
     * @brief 处理 NoteOff 事件，释放对应的复音。
     * @param note 要释放的 MIDI 音高。
     * @return 返回被关闭的id，如果没有则是kNumResonators
     */
    int noteOff(int note) {
        // 找到所有分配给该音高的复音并释放
        for (size_t id = 0; id < kNumResonators; ++id) {
            if (voices[id].isActive && voices[id].midiNote == note) {
                deactivateVoice(static_cast<int>(id));
                // 注意：如果有多个复音分配到同一个音高，这里只会释放找到的第一个。
                // 如果需要支持单音高多复音，则可以继续循环或使用更复杂的映射。
                return static_cast<int>(id);
            }
        }
        return kNumResonators;
    }
    
    // -----------------------------------------------------------
    // 调试/查询方法
    // -----------------------------------------------------------
    const Voice& getVoice(size_t id) const {
        return voices[id];
    }

    void initializeVoices() {
        for (size_t i = 0; i < kNumResonators; ++i) {
            voices[i].voiceID = static_cast<int>(i);
            voices[i].midiNote = -1;
            voices[i].isActive = false;
        }
        round_robin_ = 0;
    }

private:
    std::array<Voice, kNumResonators> voices;           // 存储所有复音实例
    std::deque<int> activationOrder;     // 存储正在使用复音的 ID，用于 FIFO 替换
    size_t round_robin_{};

    int activateVoice(int id, int note) {
        voices[static_cast<size_t>(id)].midiNote = note;
        voices[static_cast<size_t>(id)].isActive = true;
        
        // 将此复音 ID 添加到激活队列的尾部，表示它现在是“最新”的
        activationOrder.push_back(id);
        return id;
    }

    void deactivateVoice(int id) {
        voices[static_cast<size_t>(id)].isActive = false;
        voices[static_cast<size_t>(id)].midiNote = -1;

        // 从激活队列中移除此 ID，因为它不再是“激活”状态
        // 使用 std::remove_if + erase idiom
        activationOrder.erase(
            std::remove(activationOrder.begin(), activationOrder.end(), id),
            activationOrder.end()
        );
    }
};

// ---------------------------------------- juce processor ----------------------------------------
class ResonatorAudioProcessor final : public juce::AudioProcessor
{
public:
    //==============================================================================
    ResonatorAudioProcessor();
    ~ResonatorAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    JuceParamListener param_listener_;
    std::unique_ptr<juce::AudioProcessorValueTreeState> value_tree_;

    std::array<juce::AudioParameterFloat*, kNumResonators> pitches_{};
    std::array<juce::AudioParameterFloat*, kNumResonators> fine_tune_{};
    std::array<juce::AudioParameterFloat*, kNumResonators> damp_pitch_{};
    std::array<juce::AudioParameterFloat*, kNumResonators> damp_gain_db_{};
    std::array<juce::AudioParameterFloat*, kNumResonators> dispersion_pole_radius_{};
    std::array<juce::AudioParameterFloat*, kNumResonators> decays_{};
    std::array<juce::AudioParameterBool*, kNumResonators> polarity_{};
    std::array<juce::AudioParameterFloat*, kNumResonators> mix_volume_{};
    std::array<juce::AudioParameterFloat*, kNumResonators> matrix_reflections_{};

    juce::AudioParameterBool* midi_drive_{};
    juce::AudioParameterBool* allow_round_robin_{};
    juce::AudioParameterFloat* global_pitch_{};
    juce::AudioParameterFloat* global_damp_{};
    juce::AudioParameterFloat* dry_mix_{};

    bool was_midi_drive_{false};
    PolyphonyManager note_manager_;

    Resonator dsp_;
private:
    void ProcessCommon(juce::AudioBuffer<float>&, juce::MidiBuffer&);
    void ProcessMidi(juce::AudioBuffer<float>&, juce::MidiBuffer&);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResonatorAudioProcessor)
};
