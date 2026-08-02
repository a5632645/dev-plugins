#pragma once
#include <array>
#include <concepts>
#include <qwqdsp/polymath.hpp>
#include <span>
#include "../global.hpp"
#include "../param.hpp"
#include "idsp.hpp"
#include "pluginshared/dsp/delay_line_multiple.hpp"
#include "pluginshared/simd/simd.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"

namespace vital_chorus {

template <simd::Inst inst, class SimdType>
class ParalleOnePoleTPT {
public:
    void Reset() noexcept {
        lag_ = SimdType{};
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
        SimdType delta = (coeff) * (x - lag_);
        lag_ += delta;
        SimdType y = lag_;
        lag_ += delta;
        return y;
    }

    SimdType TickHighpass(SimdType x, float coeff) noexcept {
        SimdType delta = (coeff) * (x - lag_);
        lag_ += delta;
        SimdType y = lag_;
        lag_ += delta;
        return x - y;
    }
private:
    SimdType lag_{};
};

template <simd::Inst inst, class SimdType>
class DspImpl : public Idsp {
public:
    static constexpr int kVoicesPerVector = simd::LaneSize<SimdType>;
    static constexpr int kMaxVectorCount = global::kMaxNumChorus / kVoicesPerVector;

    ~DspImpl() override = default;

    void Init(float fs) override {
        float const samples = fs * global::kMaxDelayMs / 1000.0f;
        for (auto& d : delays_) {
            d.Init(static_cast<size_t>(std::ceil(samples)));
        }
        fs_ = fs;
    }

    void Reset() override {
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

    void Update(const DspParam& p) override {
        depth_ = p.depth;
        delay1_ = p.delay1;
        delay2_ = p.delay2;
        feedback_ = p.feedback;
        mix_ = p.mix;

        phase_inc_ = p.freq / fs_;

        lowpass_coeff_ = ParalleOnePoleTPT<inst, SimdType>::ComputeCoeff(p.low_w);
        highpass_coeff_ = ParalleOnePoleTPT<inst, SimdType>::ComputeCoeff(p.high_w);

        if (num_voices_ != p.num_voice) {
            // 只重置新启用的延迟线：旧计数 → 新计数之间
            // simd256：4 与 8 都是 1 条（4→8 不重置，延迟线持续工作）；只有 4/8→12/16 才需重置第二条
            const int old_count = DelayLineCount(num_voices_);
            const int new_count = DelayLineCount(p.num_voice);
            for (int i = old_count; i < new_count; ++i) {
                lowpass_[i].Reset();
                highpass_[i].Reset();
                delays_[i].Reset();
            }
            num_voices_ = p.num_voice;
        }
    }

    /** 每块调用：从 Params 中的 BpmSyncLFO 同步频率（Hz）与相位（含相位重置） */
    void SyncPhase(Params& p, juce::AudioPlayHead* ph) override {
        phase_inc_ = p.freq.GetFreqHz(ph) / fs_;
        if (auto sync_phase = p.freq.GetSyncPhase(ph)) {
            phase_ = *sync_phase;
        }
    }

    void Process(float* __restrict left_ptr, float* __restrict right_ptr, int num_samples) override {
        switch (num_voices_) {
            case 4:
                ProcessInternal<4, SimdType>(left_ptr, right_ptr, num_samples);
                break;
            case 8:
                ProcessInternal<8, SimdType>(left_ptr, right_ptr, num_samples);
                break;
            case 12:
                ProcessInternal<12, SimdType>(left_ptr, right_ptr, num_samples);
                break;
            case 16:
                ProcessInternal<16, SimdType>(left_ptr, right_ptr, num_samples);
                break;
            default:
                break; // num_voices_ 未初始化/非法时静默
        }
    }

    std::string_view InstName() override {
        return INST_NAME;
    }

    /** 各声道的延迟时间（ms），channel i → [i]（i 偶 = 左声道，i 奇 = 右声道），供 UI 可视化 */
    std::array<float, global::kMaxNumChorus> GetDelayMs() const override {
        std::array<float, global::kMaxNumChorus> out{};
        // delay_ms_ 平铺为 lane-major：声部 v 的 L/R 在 flat[2v] / flat[2v+1]
        const float* src = reinterpret_cast<const float*>(delay_ms_.data());
        for (size_t i = 0; i < out.size(); ++i) {
            out[i] = src[i];
        }
        return out;
    }
private:
    // ---------------- SIMD 128/256 分流（vital_reverb 同款 requires 约束） ----------------
    // 静态延迟：单向量内声部交替 d1/d2（128 = 2 声部，256 = 4 声部）
    template <class T>
        requires std::same_as<T, simd::Float128>
    static SimdType MakeStaticDelay(float delay1, float delay2) noexcept {
        return simd::Float128{delay1, delay1, delay2, delay2};
    }
    template <class T>
        requires std::same_as<T, simd::Float256>
    static SimdType MakeStaticDelay(float delay1, float delay2) noexcept {
        return simd::Combine(simd::Float128{delay1, delay1, delay2, delay2},
                             simd::Float128{delay1, delay1, delay2, delay2});
    }

    // 每 lane 的相位偏移（立体声合唱：各 lane 独立调制相位）
    // 128: lane j → j/4；256: lane j → j/8（均匀铺满一个周期）
    template <class T>
        requires std::same_as<T, simd::Float128>
    static SimdType MakePhaseOffset() noexcept {
        return simd::Float128{0.0f, 0.25f, 0.5f, 0.75f};
    }
    template <class T>
        requires std::same_as<T, simd::Float256>
    static SimdType MakePhaseOffset() noexcept {
        return simd::Float256{0.0f, 0.125f, 0.25f, 0.375f, 0.5f, 0.625f, 0.75f, 0.875f};
    }

    // 输入交织：L/R/L/R...
    template <class T>
        requires std::same_as<T, simd::Float128>
    static SimdType MakeShuffleIn(float l, float r) noexcept {
        return simd::Float128{l, r, l, r};
    }
    template <class T>
        requires std::same_as<T, simd::Float256>
    static SimdType MakeShuffleIn(float l, float r) noexcept {
        return simd::Combine(simd::Float128{l, r, l, r}, simd::Float128{l, r, l, r});
    }

    // 输出还原：各声部 L/R 分别求和
    template <class T>
        requires std::same_as<T, simd::Float128>
    static void ShuffleOut(SimdType t, float& l, float& r) noexcept {
        l = t[0] + t[2];
        r = t[1] + t[3];
    }
    template <class T>
        requires std::same_as<T, simd::Float256>
    static void ShuffleOut(SimdType t, float& l, float& r) noexcept {
        l = t[0] + t[2] + t[4] + t[6];
        r = t[1] + t[3] + t[5] + t[7];
    }

    /** 按声部数编译期特化：nVoice 为 4 的倍数，声部对循环可完全展开 */
    template <int nVoice, class SimdTypeTag>
        requires std::same_as<SimdTypeTag, simd::Float128>
    void ProcessInternal(float* __restrict left_ptr, float* __restrict right_ptr, int num_samples) noexcept {
        constexpr int kNumPairs = nVoice / kVoicesPerVector;
        if (num_samples == 0) {
            return;
        }
        float const g = 1.0f / std::sqrt(static_cast<float>(kNumPairs));
        float const inv_processing_samples = 1.0f / static_cast<float>(num_samples);

        // update delay time（整块一次）
        // you can see visualizer here https://www.desmos.com/calculator/5ytjkkqtbb?lang=zh-CN
        float const delay_time_smooth_factor =
            1.0f - std::exp(-1.0f / (fs_ / static_cast<float>(num_samples) * 20.0f / 1000.0f));
        phase_ += phase_inc_ * static_cast<float>(num_samples);
        phase_ -= std::floor(phase_);
        float const avg_delay = (delay1_ + delay2_) * 0.5f;
        for (int i = 0; i < kNumPairs; ++i) {
            SimdType static_a = MakeStaticDelay<SimdType>(delay1_, delay2_);
            SimdType static_b = simd::Broadcast<SimdType>(avg_delay);
            float const lerp = static_cast<float>(i) / std::max(1.0f, static_cast<float>(kNumPairs) - 1.0f);
            SimdType static_delay = static_a + lerp * (static_b - static_a);
            SimdType offsetp = MakePhaseOffset<SimdType>();
            SimdType sin_mod = phase_ + offsetp + (0.25f * static_cast<float>(i) / static_cast<float>(kNumPairs));
            sin_mod = simd::Frac(sin_mod);
            for (int j = 0; j < simd::LaneSize<SimdType>; ++j) {
                sin_mod[j] = qwqdsp::polymath::SinParabola(sin_mod[j] * 2 * std::numbers::pi_v<float>
                                                           - std::numbers::pi_v<float>);
            }
            sin_mod = sin_mod * 0.5f + 1.0f;
            SimdType delay_ms = (depth_ * global::kMaxModulationMs) * sin_mod + static_delay;
            delay_ms_[i] = delay_ms;

            // additional exp smooth
            SimdType target_delay_samples = delay_ms * (fs_ / 1000.0f);
            delay_samples_[i] += (delay_time_smooth_factor) * (target_delay_samples - delay_samples_[i]);
        }

        // 逐对的延迟采样斜坡状态（每对起点/增量不同，需各自一份）
        simd::Array<SimdType, kMaxVectorCount> curr_delay_samples;
        simd::Array<SimdType, kMaxVectorCount> delta_delay_samples;
        for (int i = 0; i < kNumPairs; ++i) {
            curr_delay_samples[i] = last_delay_samples_[i];
            delta_delay_samples[i] = (delay_samples_[i] - last_delay_samples_[i]) * inv_processing_samples;
        }

        // 共享平滑状态（各对同起点同增量，可共用一份）
        float const delta_feedback = (feedback_ - last_feedback_) * inv_processing_samples;
        SimdType vcurr_feedback = simd::Broadcast<SimdType>(last_feedback_);
        SimdType vdelta_feedback = simd::Broadcast<SimdType>(delta_feedback);
        float vcurr_lowpass = last_lowpass_coeff_;
        float const delta_lowpass = (lowpass_coeff_ - last_lowpass_coeff_) * inv_processing_samples;
        float vcurr_highpass = last_highpass_coeff_;
        float const delta_highpass = (highpass_coeff_ - last_highpass_coeff_) * inv_processing_samples;

        float const dry = qwqdsp::polymath::CosPi(mix_ * std::numbers::pi_v<float> * 0.5f);
        float const wet = g * qwqdsp::polymath::SinPi(mix_ * std::numbers::pi_v<float> * 0.5f);
        SimdType curr_dry = simd::Broadcast<SimdType>(last_dry_);
        SimdType curr_wet = simd::Broadcast<SimdType>(last_wet_);
        SimdType delta_dry = simd::Broadcast<SimdType>((dry - last_dry_) * inv_processing_samples);
        SimdType delta_wet = simd::Broadcast<SimdType>((wet - last_wet_) * inv_processing_samples);

        // 逐采样主循环：shuffle 入 → 各声部对处理 → 累加 → 干湿混音 → shuffle 出
        for (int j = 0; j < num_samples; ++j) {
            vcurr_feedback += vdelta_feedback;
            vcurr_lowpass += delta_lowpass;
            vcurr_highpass += delta_highpass;
            curr_dry += delta_dry;
            curr_wet += delta_wet;

            SimdType in = MakeShuffleIn<SimdType>(left_ptr[j], right_ptr[j]) * 0.5f;
            SimdType acc{};
            for (int i = 0; i < kNumPairs; ++i) {
                curr_delay_samples[i] += delta_delay_samples[i];
                SimdType read = delays_[i].GetAfterPush(curr_delay_samples[i]);
                SimdType write = in + read * vcurr_feedback;
                write = lowpass_[i].TickLowpass(write, vcurr_lowpass);
                write = highpass_[i].TickHighpass(write, vcurr_highpass);
                delays_[i].Push(write);
                acc += read;
            }
            SimdType t = curr_dry * in + curr_wet * acc;
            ShuffleOut<SimdType>(t, left_ptr[j], right_ptr[j]);
        }

        for (int i = 0; i < kNumPairs; ++i) {
            last_delay_samples_[i] = curr_delay_samples[i];
        }
        last_feedback_ = feedback_;
        last_dry_ = dry;
        last_wet_ = wet;
        last_lowpass_coeff_ = lowpass_coeff_;
        last_highpass_coeff_ = highpass_coeff_;
    }

    /** SIMD256 输出混合掩码：vector p 的 lane j = 通道 8p+j，有效条件 8p+j < nVoice；
     *  多余通道归零；返回两条向量（vector 0 / vector 1），调用方只取前 ceil(nVoice/8) 条 */
    template <int nVoice>
    static constexpr simd::Array<simd::Float256, 2> MakeOutMasks256() noexcept {
        if constexpr (nVoice == 4) {
            return {
                simd::Float256{1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f},
                simd::Float256{0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}
            };
        }
        else if constexpr (nVoice == 8) {
            return {
                simd::Float256{1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
                simd::Float256{0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}
            };
        }
        else if constexpr (nVoice == 12) {
            return {
                simd::Float256{1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
                simd::Float256{1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f}
            };
        }
        else { // nVoice == 16
            return {
                simd::Float256{1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
                simd::Float256{1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}
            };
        }
    }

    /** SIMD256 版：每条 256 延迟线 = 2 条 128 子延迟线（lane 0-3 / lane 4-7）
     *  延迟线数 = ceil(nVoice/8)；时间计算直接套用 simd128 公式，4 的倍数下逐通道一致；
     *  多余通道（>= nVoice）在输出用混合掩码归零。 */
    template <int nVoice, class SimdTypeTag>
        requires std::same_as<SimdTypeTag, simd::Float256>
    void ProcessInternal(float* __restrict left_ptr, float* __restrict right_ptr, int num_samples) noexcept {
        constexpr int kNumPairs = (nVoice + 7) / 8; // ceil(nVoice / 8)，<= 2
        constexpr int kNp128 = nVoice / 4;          // 等价 simd128 延迟线数
        if (num_samples == 0) {
            return;
        }
        // 干湿归一化与 simd128 一致（按 128 向量数，而非 256 向量数）
        float const g = 1.0f / std::sqrt(static_cast<float>(kNp128));
        float const inv_processing_samples = 1.0f / static_cast<float>(num_samples);

        // update delay time（整块一次）
        float const delay_time_smooth_factor =
            1.0f - std::exp(-1.0f / (fs_ / static_cast<float>(num_samples) * 20.0f / 1000.0f));
        phase_ += phase_inc_ * static_cast<float>(num_samples);
        phase_ -= std::floor(phase_);
        float const avg_delay = (delay1_ + delay2_) * 0.5f;
        // 静态延迟与相位偏移模式：与 simd128 逐通道一致（{d1,d1,d2,d2} / {0,.25,.5,.75} 各重复两遍）
        const simd::Float256 static_a =
            simd::Float256{delay1_, delay1_, delay2_, delay2_, delay1_, delay1_, delay2_, delay2_};
        const simd::Float256 static_b = simd::Broadcast<simd::Float256>(avg_delay);
        const simd::Float256 offsetp = simd::Float256{0.0f, 0.25f, 0.5f, 0.75f, 0.0f, 0.25f, 0.5f, 0.75f};
        for (int p = 0; p < kNumPairs; ++p) {
            // 前 4 lane = 128 子延迟线 2p，后 4 lane = 2p+1（越界者被输出掩码归零）
            // 直接计算，与 simd128 公式一致：lerp = i/(kNp128-1)，phase = 0.25·i/kNp128
            const int i128_lo = 2 * p;
            const int i128_hi = 2 * p + 1;
            const float lerp_lo = static_cast<float>(i128_lo) / std::max(1.0f, static_cast<float>(kNp128 - 1));
            const float lerp_hi = static_cast<float>(i128_hi) / std::max(1.0f, static_cast<float>(kNp128 - 1));
            const float phase_lo = 0.25f * static_cast<float>(i128_lo) / static_cast<float>(kNp128);
            const float phase_hi = 0.25f * static_cast<float>(i128_hi) / static_cast<float>(kNp128);
            const simd::Float256 lerp_vec =
                simd::Float256{lerp_lo, lerp_lo, lerp_lo, lerp_lo, lerp_hi, lerp_hi, lerp_hi, lerp_hi};
            const simd::Float256 phase_vec =
                simd::Float256{phase_lo, phase_lo, phase_lo, phase_lo, phase_hi, phase_hi, phase_hi, phase_hi};
            SimdType static_delay = static_a + lerp_vec * (static_b - static_a);
            SimdType sin_mod = phase_ + offsetp + phase_vec;
            sin_mod = simd::Frac(sin_mod);
            for (int j = 0; j < simd::LaneSize<SimdType>; ++j) {
                sin_mod[j] = qwqdsp::polymath::SinParabola(sin_mod[j] * 2 * std::numbers::pi_v<float>
                                                           - std::numbers::pi_v<float>);
            }
            sin_mod = sin_mod * 0.5f + 1.0f;
            SimdType delay_ms = (depth_ * global::kMaxModulationMs) * sin_mod + static_delay;
            delay_ms_[p] = delay_ms;

            SimdType target_delay_samples = delay_ms * (fs_ / 1000.0f);
            delay_samples_[p] += (delay_time_smooth_factor) * (target_delay_samples - delay_samples_[p]);
        }

        // 逐对的延迟采样斜坡状态
        simd::Array<SimdType, kMaxVectorCount> curr_delay_samples;
        simd::Array<SimdType, kMaxVectorCount> delta_delay_samples;
        for (int p = 0; p < kNumPairs; ++p) {
            curr_delay_samples[p] = last_delay_samples_[p];
            delta_delay_samples[p] = (delay_samples_[p] - last_delay_samples_[p]) * inv_processing_samples;
        }

        // 共享平滑状态
        float const delta_feedback = (feedback_ - last_feedback_) * inv_processing_samples;
        SimdType vcurr_feedback = simd::Broadcast<SimdType>(last_feedback_);
        SimdType vdelta_feedback = simd::Broadcast<SimdType>(delta_feedback);
        float vcurr_lowpass = last_lowpass_coeff_;
        float const delta_lowpass = (lowpass_coeff_ - last_lowpass_coeff_) * inv_processing_samples;
        float vcurr_highpass = last_highpass_coeff_;
        float const delta_highpass = (highpass_coeff_ - last_highpass_coeff_) * inv_processing_samples;

        float const dry = qwqdsp::polymath::CosPi(mix_ * std::numbers::pi_v<float> * 0.5f);
        float const wet = g * qwqdsp::polymath::SinPi(mix_ * std::numbers::pi_v<float> * 0.5f);
        SimdType curr_dry = simd::Broadcast<SimdType>(last_dry_);
        SimdType curr_wet = simd::Broadcast<SimdType>(last_wet_);
        SimdType delta_dry = simd::Broadcast<SimdType>((dry - last_dry_) * inv_processing_samples);
        SimdType delta_wet = simd::Broadcast<SimdType>((wet - last_wet_) * inv_processing_samples);

        // 输出混合掩码：多余通道归零（只取前 kNumPairs 条）
        constexpr auto out_masks = MakeOutMasks256<nVoice>();

        // 逐采样主循环：shuffle 入 → 各延迟线处理 → 掩码累加 → 干湿混音 → shuffle 出
        for (int j = 0; j < num_samples; ++j) {
            vcurr_feedback += vdelta_feedback;
            vcurr_lowpass += delta_lowpass;
            vcurr_highpass += delta_highpass;
            curr_dry += delta_dry;
            curr_wet += delta_wet;

            SimdType in = MakeShuffleIn<SimdType>(left_ptr[j], right_ptr[j]) * 0.5f;
            SimdType acc{};
            for (int p = 0; p < kNumPairs; ++p) {
                curr_delay_samples[p] += delta_delay_samples[p];
                SimdType read = delays_[p].GetAfterPush(curr_delay_samples[p]);
                SimdType write = in + read * vcurr_feedback;
                write = lowpass_[p].TickLowpass(write, vcurr_lowpass);
                write = highpass_[p].TickHighpass(write, vcurr_highpass);
                delays_[p].Push(write);
                acc += read * out_masks[p];
            }
            // 干声减半：ShuffleOut256 求和 4 条输入 lane（0.5L×4 = 2L），需折半为 1L 与 simd128 一致；
            // 湿声与延迟线输入不受影响
            SimdType t = 0.5f * curr_dry * in + curr_wet * acc;
            ShuffleOut<SimdType>(t, left_ptr[j], right_ptr[j]);
        }

        for (int p = 0; p < kNumPairs; ++p) {
            last_delay_samples_[p] = curr_delay_samples[p];
        }
        last_feedback_ = feedback_;
        last_dry_ = dry;
        last_wet_ = wet;
        last_lowpass_coeff_ = lowpass_coeff_;
        last_highpass_coeff_ = highpass_coeff_;
    }

    static constexpr int DelayLineCount(int num_voices) noexcept {
        return (num_voices + kVoicesPerVector - 1) / kVoicesPerVector;
    }

    float fs_{};
    float phase_{};
    float phase_inc_{};
    int num_voices_{};
    simd::Array<SimdType, kMaxVectorCount> delay_samples_{};
    simd::Array<ParalleOnePoleTPT<inst, SimdType>, kMaxVectorCount> lowpass_;
    simd::Array<ParalleOnePoleTPT<inst, SimdType>, kMaxVectorCount> highpass_;
    simd::Array<SimdType, kMaxVectorCount> last_delay_samples_{};
    simd::Array<pluginshared::dsp::DelayLineMultiple<inst, SimdType>, kMaxVectorCount> delays_;
    float last_feedback_{};
    float last_dry_{};
    float last_wet_{};

    // these are total pass
    float lowpass_coeff_{1.0f};
    float last_lowpass_coeff_{1.0f};
    float highpass_coeff_{0.0f};
    float last_highpass_coeff_{0.0f};

    float depth_{};
    float delay1_{};
    float delay2_{};
    float feedback_{};
    float mix_{};

    // UI 可视化读取（chorus_view 30Hz 刷新）
    simd::Array<SimdType, kMaxVectorCount> delay_ms_{};
};

} // namespace vital_chorus

#pragma GCC diagnostic pop
