#pragma once
#include "../../shared/juce_param_listener.hpp"
#include "shared.hpp"

#include "qwqdsp/misc/smoother.hpp"
#include "qwqdsp/spectral/complex_fft.hpp"
#include "qwqdsp/osciilor/vic_sine_osc.hpp"
#include "qwqdsp/psimd/align_allocator.hpp"
#include "qwqdsp/psimd/vec4.hpp"
#include "qwqdsp/psimd/vec8.hpp"
#include "qwqdsp/filter/one_pole_tpt_simd.hpp"
#include "qwqdsp/filter/iir_cpx_hilbert_stereo_simd.hpp"
#include "qwqdsp/force_inline.hpp"
#include "qwqdsp/convert.hpp"
#include "qwqdsp/polymath.hpp"
#include "qwqdsp/filter/window_fir.hpp"
#include "qwqdsp/window/kaiser.hpp"
#include "simd_detector.h"
#include "x86/sse4.1.h"
#include "x86/avx.h"

using Float32x4 = qwqdsp::psimd::Vec4f32;
using Int32x4 = qwqdsp::psimd::Vec4i32;
using Float32x8 = qwqdsp::psimd::Vec8f32;
using Int32x8 = qwqdsp::psimd::Vec8i32;

struct Complex32x4 {
    Float32x4 re;
    Float32x4 im;

    QWQDSP_FORCE_INLINE
    constexpr Complex32x4& operator*=(const Complex32x4& a) {
        Float32x4 new_re = re * a.re - im * a.im;
        Float32x4 new_im = re * a.im + im * a.re;
        re = new_re;
        im = new_im;
        return *this;
    }
};

struct Complex32x8 {
    Float32x8 re;
    Float32x8 im;

    QWQDSP_FORCE_INLINE
    constexpr Complex32x8& operator*=(const Complex32x8& a) {
        Float32x8 new_re = re * a.re - im * a.im;
        Float32x8 new_im = re * a.im + im * a.re;
        re = new_re;
        im = new_im;
        return *this;
    }
};

class Vec4DelayLine {
public:
    void Init(float max_ms, float fs) {
        float d = max_ms * fs / 1000.0f;
        size_t i = static_cast<size_t>(std::ceil(d));
        Init(i);
    }

    void Init(size_t max_samples) {
        size_t a = 1;
        while (a < max_samples) {
            a *= 2;
        }
        mask_ = static_cast<int>(a - 1);
        delay_length_ = static_cast<int>(a);

        a += 4;
        if (buffer_.size() < a) {
            buffer_.resize(a);
        }
        Reset();
    }

    void Reset() noexcept {
        wpos_ = 0;
        std::fill(buffer_.begin(), buffer_.end(), 0.0f);
    }

    QWQDSP_FORCE_INLINE
    Float32x4 GetAfterPush(Float32x4 delay_samples) const noexcept {
        Float32x4 frpos = Float32x4::FromSingle(static_cast<float>(wpos_ + mask_)) - delay_samples;
        Int32x4 rpos = frpos.ToInt();
        Int32x4 mask = Int32x4::FromSingle(mask_);
        Int32x4 irpos = rpos & mask;
        Float32x4 frac = frpos.Frac();

        Float32x4 interp0;
        simde_mm_store_ps(interp0.x, simde_mm_loadu_ps(buffer_.data() + irpos.x[0]));
        Float32x4 interp1;
        simde_mm_store_ps(interp1.x, simde_mm_loadu_ps(buffer_.data() + irpos.x[1]));
        Float32x4 interp2;
        simde_mm_store_ps(interp2.x, simde_mm_loadu_ps(buffer_.data() + irpos.x[2]));
        Float32x4 interp3;
        simde_mm_store_ps(interp3.x, simde_mm_loadu_ps(buffer_.data() + irpos.x[3]));

        Float32x4 y0;
        y0.x[0] = interp0.x[0];
        y0.x[1] = interp1.x[0];
        y0.x[2] = interp2.x[0];
        y0.x[3] = interp3.x[0];
        Float32x4 y1;
        y1.x[0] = interp0.x[1];
        y1.x[1] = interp1.x[1];
        y1.x[2] = interp2.x[1];
        y1.x[3] = interp3.x[1];
        Float32x4 y2;
        y2.x[0] = interp0.x[2];
        y2.x[1] = interp1.x[2];
        y2.x[2] = interp2.x[2];
        y2.x[3] = interp3.x[2];
        Float32x4 y3;
        y3.x[0] = interp0.x[3];
        y3.x[1] = interp1.x[3];
        y3.x[2] = interp2.x[3];
        y3.x[3] = interp3.x[3];

        Float32x4 d1 = frac - Float32x4::FromSingle(1.0f);
        Float32x4 d2 = frac - Float32x4::FromSingle(2.0f);
        Float32x4 d3 = frac - Float32x4::FromSingle(3.0f);

        auto c1 = d1 * d2 * d3 / Float32x4::FromSingle(-6.0f);
        auto c2 = d2 * d3 * Float32x4::FromSingle(0.5f);
        auto c3 = d1 * d3 * Float32x4::FromSingle(-0.5f);
        auto c4 = d1 * d2 / Float32x4::FromSingle(6.0f);

        return y0 * c1 + frac * (y1 * c2 + y2 * c3 + y3 * c4);
    }

    QWQDSP_FORCE_INLINE
    Float32x8 GetAfterPush(Float32x8 const& delay_samples) const noexcept {
        Float32x8 frpos = Float32x8::FromSingle(static_cast<float>(wpos_ + mask_)) - delay_samples;
        Int32x8 rpos = frpos.ToInt();
        Int32x8 mask = Int32x8::FromSingle(mask_);
        Int32x8 irpos = rpos & mask;
        Float32x8 frac = frpos.Frac();

        Float32x8 interp0;
        simde_mm256_store_ps(interp0.x, simde_mm256_loadu_ps(buffer_.data() + irpos.x[0]));
        Float32x8 interp1;
        simde_mm256_store_ps(interp1.x, simde_mm256_loadu_ps(buffer_.data() + irpos.x[1]));
        Float32x8 interp2;
        simde_mm256_store_ps(interp2.x, simde_mm256_loadu_ps(buffer_.data() + irpos.x[2]));
        Float32x8 interp3;
        simde_mm256_store_ps(interp3.x, simde_mm256_loadu_ps(buffer_.data() + irpos.x[3]));
        Float32x8 interp4;
        simde_mm256_store_ps(interp4.x, simde_mm256_loadu_ps(buffer_.data() + irpos.x[4]));
        Float32x8 interp5;
        simde_mm256_store_ps(interp5.x, simde_mm256_loadu_ps(buffer_.data() + irpos.x[5]));
        Float32x8 interp6;
        simde_mm256_store_ps(interp6.x, simde_mm256_loadu_ps(buffer_.data() + irpos.x[6]));
        Float32x8 interp7;
        simde_mm256_store_ps(interp7.x, simde_mm256_loadu_ps(buffer_.data() + irpos.x[7]));

        Float32x8 y0;
        y0.x[0] = interp0.x[0];
        y0.x[1] = interp1.x[0];
        y0.x[2] = interp2.x[0];
        y0.x[3] = interp3.x[0];
        y0.x[4] = interp4.x[0];
        y0.x[5] = interp5.x[0];
        y0.x[6] = interp6.x[0];
        y0.x[7] = interp7.x[0];
        Float32x8 y1;
        y1.x[0] = interp0.x[1];
        y1.x[1] = interp1.x[1];
        y1.x[2] = interp2.x[1];
        y1.x[3] = interp3.x[1];
        y1.x[4] = interp4.x[1];
        y1.x[5] = interp5.x[1];
        y1.x[6] = interp6.x[1];
        y1.x[7] = interp7.x[1];
        Float32x8 y2;
        y2.x[0] = interp0.x[2];
        y2.x[1] = interp1.x[2];
        y2.x[2] = interp2.x[2];
        y2.x[3] = interp3.x[2];
        y2.x[4] = interp4.x[2];
        y2.x[5] = interp5.x[2];
        y2.x[6] = interp6.x[2];
        y2.x[7] = interp7.x[2];
        Float32x8 y3;
        y3.x[0] = interp0.x[3];
        y3.x[1] = interp1.x[3];
        y3.x[2] = interp2.x[3];
        y3.x[3] = interp3.x[3];
        y3.x[4] = interp4.x[3];
        y3.x[5] = interp5.x[3];
        y3.x[6] = interp6.x[3];
        y3.x[7] = interp7.x[3];

        Float32x8 d1 = frac - Float32x8::FromSingle(1.0f);
        Float32x8 d2 = frac - Float32x8::FromSingle(2.0f);
        Float32x8 d3 = frac - Float32x8::FromSingle(3.0f);

        auto c1 = d1 * d2 * d3 / Float32x8::FromSingle(-6.0f);
        auto c2 = d2 * d3 * Float32x8::FromSingle(0.5f);
        auto c3 = d1 * d3 * Float32x8::FromSingle(-0.5f);
        auto c4 = d1 * d2 / Float32x8::FromSingle(6.0f);

        return y0 * c1 + frac * (y1 * c2 + y2 * c3 + y3 * c4);
    }

    QWQDSP_FORCE_INLINE
    void Push(float x) noexcept {
        buffer_[static_cast<size_t>(wpos_++)] = x;
        wpos_ &= mask_;
    }

    QWQDSP_FORCE_INLINE
    void WrapBuffer() noexcept {
        auto a = simde_mm_load_ps(buffer_.data());
        simde_mm_store_ps(buffer_.data() + delay_length_, a);
    }

private:
    std::vector<float, qwqdsp::psimd::AlignedAllocator<float, 32>> buffer_;
    int delay_length_{};
    int wpos_{};
    int mask_{};
};

class SteepFlangerParameter {
public:
    float delay_ms;
    float depth_ms;
    float lfo_freq;
    float lfo_phase;
    float fir_cutoff;
    size_t fir_coeff_len;
    float fir_side_lobe;
    bool fir_min_phase;
    bool fir_highpass;
    float feedback;
    float damp_pitch;
    bool feedback_enable;
    float barber_phase;
    float barber_speed;
    bool barber_enable;
    std::atomic<bool> should_update_fir_{};
    std::atomic<bool> is_using_custom_{};
    std::array<float, kMaxCoeffLen> custom_coeffs_{};
    std::array<float, kMaxCoeffLen> custom_spectral_gains{};
};

class SteepFlanger {
public:
    SteepFlanger() {
        complex_fft_.Init(kFFTSize);

        cpu_arch_ = 0;
        if (simd_detector::is_supported(simd_detector::InstructionSet::AVX)) {
            cpu_arch_ = 2;
        }
        else if (simd_detector::is_supported(simd_detector::InstructionSet::SSE4_1)) {
            cpu_arch_ = 1;
        }
        else if (simd_detector::is_supported(simd_detector::InstructionSet::NEON)) {
            cpu_arch_ = 1;
        }
    }

    void Init(float fs, float max_delay_ms) {
        fs_ = fs;
        float const samples_need = fs * max_delay_ms / 1000.0f;
        delay_left_.Init(static_cast<size_t>(samples_need * kMaxCoeffLen));
        delay_right_.Init(static_cast<size_t>(samples_need * kMaxCoeffLen));
        barber_phase_smoother_.SetSmoothTime(20.0f, fs);
        damp_.Reset();
        barber_oscillator_.Reset();
        barber_osc_keep_amp_counter_ = 0;
        // VIC正交振荡器衰减非常慢，设定为5分钟保持一次
        barber_osc_keep_amp_need_ = static_cast<size_t>(fs * 60 * 5);
    }

    void Reset() noexcept {
        delay_left_.Reset();
        delay_right_.Reset();
        left_fb_ = 0;
        right_fb_ = 0;
        damp_.Reset();
        hilbert_complex_.Reset();
    }

    void Process(
        float* left_ptr, float* right_ptr, size_t len,
        SteepFlangerParameter& param
    ) noexcept {
        if (cpu_arch_ == 1) {
            ProcessVec4(left_ptr, right_ptr, len, param);
        }
        else if (cpu_arch_ == 2) {
            ProcessVec8(left_ptr, right_ptr, len, param);
        }
    }

    void ProcessVec8(
        float* left_ptr, float* right_ptr, size_t len,
        SteepFlangerParameter& param
    ) noexcept {
        size_t cando = len;
        while (cando != 0) {
            size_t num_process = std::min<size_t>(512, cando);
            cando -= num_process;

            if (param.should_update_fir_.exchange(false)) {
                UpdateCoeff(param);
            }

            float feedback_mul = 0;
            if (param.feedback_enable) {
                float db = param.feedback;
                float abs_db = std::abs(db);
                if (param.fir_min_phase) {
                    abs_db = std::max(abs_db, 4.1f);
                }
                float abs_gain = qwqdsp::convert::Db2Gain(-abs_db);
                abs_gain = std::min(abs_gain, 0.95f);
                if (db > 0) {
                    feedback_mul = -abs_gain;
                }
                else {
                    feedback_mul = abs_gain;
                }
            }

            float const damp_pitch = param.damp_pitch;
            float const damp_freq = qwqdsp::convert::Pitch2Freq(damp_pitch);
            float const damp_w = qwqdsp::convert::Freq2W(damp_freq, fs_);
            damp_lowpass_coeff_ = damp_.ComputeCoeff(damp_w);

            barber_phase_smoother_.SetTarget(param.barber_phase);
            barber_oscillator_.SetFreq(param.barber_speed, fs_);

            // update delay times
            phase_ += param.lfo_freq / fs_ * static_cast<float>(num_process);
            float right_phase = phase_ + param.lfo_phase;
            {
                float t;
                phase_ = std::modf(phase_, &t);
                right_phase = std::modf(right_phase, &t);
            }
            float left_phase = phase_;

            Float32x4 lfo_modu;
            lfo_modu.x[0] = qwqdsp::polymath::SinPi(left_phase * std::numbers::pi_v<float>);
            lfo_modu.x[1] = qwqdsp::polymath::SinPi(right_phase * std::numbers::pi_v<float>);

            float const delay_samples = param.delay_ms * fs_ / 1000.0f;
            float const depth_samples = param.depth_ms * fs_ / 1000.0f;
            Float32x4 target_delay_samples = Float32x4::FromSingle(delay_samples) + lfo_modu * Float32x4::FromSingle(depth_samples);
            target_delay_samples = Float32x4::Max(target_delay_samples, Float32x4::FromSingle(0.0f));
            float const delay_time_smooth_factor = 1.0f - std::exp(-1.0f / (fs_ / static_cast<float>(num_process) * kDelaySmoothMs / 1000.0f));
            last_exp_delay_samples_ += Float32x4::FromSingle(delay_time_smooth_factor) * (target_delay_samples - last_exp_delay_samples_);
            Float32x4 curr_num_notch = last_delay_samples_;
            Float32x4 delta_num_notch = (last_exp_delay_samples_ - curr_num_notch) / Float32x4::FromSingle(static_cast<float>(num_process));

            float curr_damp_coeff = last_damp_lowpass_coeff_;
            float delta_damp_coeff = (damp_lowpass_coeff_ - curr_damp_coeff) / (static_cast<float>(num_process));

            delay_left_.WrapBuffer();
            delay_right_.WrapBuffer();

            float inv_samples = 1.0f / static_cast<float>(num_process);
            std::array<Float32x8, kSIMDMaxCoeffLen / 8> delta_coeffs;
            Float32x8* coeffs_ptr = reinterpret_cast<Float32x8*>(coeffs_.data());
            Float32x8* last_coeffs_ptr = reinterpret_cast<Float32x8*>(last_coeffs_.data());
            size_t const coeff_len_div8 = (coeff_len_ + 7) / 8;
            for (size_t i = 0; i < coeff_len_div8; ++i) {
                delta_coeffs[i] = (coeffs_ptr[i] - last_coeffs_ptr[i]) * Float32x8::FromSingle(inv_samples);
            }

            // fir polyphase filtering
            if (!param.barber_enable) {
                for (size_t j = 0; j < num_process; ++j) {
                    curr_num_notch += delta_num_notch;
                    curr_damp_coeff += delta_damp_coeff;

                    for (size_t i = 0; i < coeff_len_div8; ++i) {
                        last_coeffs_ptr[i] += delta_coeffs[i];
                    }
        
                    float left_sum = 0;
                    float const left_num_notch = curr_num_notch.x[0];
                    Float32x8 current_delay;
                    current_delay.x[0] = 0;
                    current_delay.x[1] = left_num_notch;
                    current_delay.x[2] = left_num_notch * 2;
                    current_delay.x[3] = left_num_notch * 3;
                    current_delay.x[4] = left_num_notch * 4;
                    current_delay.x[5] = left_num_notch * 5;
                    current_delay.x[6] = left_num_notch * 6;
                    current_delay.x[7] = left_num_notch * 7;
                    Float32x8 delay_inc = Float32x8::FromSingle(left_num_notch * 8);
                    delay_left_.Push(*left_ptr + left_fb_ * feedback_mul);
                    for (size_t i = 0; i < coeff_len_div8; ++i) {
                        Float32x8 taps_out = delay_left_.GetAfterPush(current_delay);
                        current_delay += delay_inc;

                        taps_out *= last_coeffs_ptr[i];
                        left_sum += taps_out.ReduceAdd();
                    }

                    float right_sum = 0;
                    float const right_num_notch = curr_num_notch.x[1];
                    current_delay.x[0] = 0;
                    current_delay.x[1] = right_num_notch;
                    current_delay.x[2] = right_num_notch * 2;
                    current_delay.x[3] = right_num_notch * 3;
                    current_delay.x[4] = right_num_notch * 4;
                    current_delay.x[5] = right_num_notch * 5;
                    current_delay.x[6] = right_num_notch * 6;
                    current_delay.x[7] = right_num_notch * 7;
                    delay_inc = Float32x8::FromSingle(right_num_notch * 8);
                    delay_right_.Push(*right_ptr + right_fb_ * feedback_mul);
                    for (size_t i = 0; i < coeff_len_div8; ++i) {
                        Float32x8 taps_out = delay_right_.GetAfterPush(current_delay);
                        current_delay += delay_inc;
                        taps_out *= last_coeffs_ptr[i];
                        right_sum += taps_out.ReduceAdd();
                    }

                    Float32x4 damp_x;
                    damp_x.x[0] = left_sum;
                    damp_x.x[1] = right_sum;
                    *left_ptr = left_sum;
                    *right_ptr = right_sum;
                    ++left_ptr;
                    ++right_ptr;
                    damp_x = damp_.TickLowpass(damp_x, Float32x4::FromSingle(curr_damp_coeff));
                    left_fb_ = damp_x.x[0];
                    right_fb_ = damp_x.x[1];
                }
            }
            else {
                for (size_t j = 0; j < num_process; ++j) {
                    curr_damp_coeff += delta_damp_coeff;
                    curr_num_notch += delta_num_notch;

                    for (size_t i = 0; i < coeff_len_div8; ++i) {
                        last_coeffs_ptr[i] += delta_coeffs[i];
                    }

                    delay_left_.Push(*left_ptr + left_fb_ * feedback_mul);
                    delay_right_.Push(*right_ptr + right_fb_ * feedback_mul);

                    float const left_num_notch = curr_num_notch.x[0];
                    float const right_num_notch = curr_num_notch.x[1];
                    Float32x8 left_current_delay;
                    Float32x8 right_current_delay;
                    left_current_delay.x[0] = 0;
                    left_current_delay.x[1] = left_num_notch;
                    left_current_delay.x[2] = left_num_notch * 2;
                    left_current_delay.x[3] = left_num_notch * 3;
                    left_current_delay.x[4] = left_num_notch * 4;
                    left_current_delay.x[5] = left_num_notch * 5;
                    left_current_delay.x[6] = left_num_notch * 6;
                    left_current_delay.x[7] = left_num_notch * 7;
                    right_current_delay.x[0] = 0;
                    right_current_delay.x[1] = right_num_notch;
                    right_current_delay.x[2] = right_num_notch * 2;
                    right_current_delay.x[3] = right_num_notch * 3;
                    right_current_delay.x[4] = right_num_notch * 4;
                    right_current_delay.x[5] = right_num_notch * 5;
                    right_current_delay.x[6] = right_num_notch * 6;
                    right_current_delay.x[7] = right_num_notch * 7;
                    Float32x8 left_delay_inc = Float32x8::FromSingle(left_num_notch * 8);
                    Float32x8 right_delay_inc = Float32x8::FromSingle(right_num_notch * 8);

                    auto const addition_rotation = std::polar(1.0f, barber_phase_smoother_.Tick() * std::numbers::pi_v<float> * 2);
                    barber_oscillator_.Tick();
                    auto const rotation_once = barber_oscillator_.GetCpx() * addition_rotation;
                    auto const rotation_2 = rotation_once * rotation_once;
                    auto const rotation_3 = rotation_once * rotation_2;
                    auto const rotation_4 = rotation_2 * rotation_2;
                    auto const rotation_5 = rotation_2 * rotation_3;
                    auto const rotation_6 = rotation_3 * rotation_3;
                    auto const rotation_7 = rotation_3 * rotation_4;
                    auto const rotation_8 = rotation_4 * rotation_4;
                    Complex32x8 left_rotation_coeff;
                    left_rotation_coeff.re.x[0] = 1;
                    left_rotation_coeff.re.x[1] = rotation_once.real();
                    left_rotation_coeff.re.x[2] = rotation_2.real();
                    left_rotation_coeff.re.x[3] = rotation_3.real();
                    left_rotation_coeff.re.x[4] = rotation_4.real();
                    left_rotation_coeff.re.x[5] = rotation_5.real();
                    left_rotation_coeff.re.x[6] = rotation_6.real();
                    left_rotation_coeff.re.x[7] = rotation_7.real();
                    left_rotation_coeff.im.x[0] = 0;
                    left_rotation_coeff.im.x[1] = rotation_once.imag();
                    left_rotation_coeff.im.x[2] = rotation_2.imag();
                    left_rotation_coeff.im.x[3] = rotation_3.imag();
                    left_rotation_coeff.im.x[4] = rotation_4.imag();
                    left_rotation_coeff.im.x[5] = rotation_5.imag();
                    left_rotation_coeff.im.x[6] = rotation_6.imag();
                    left_rotation_coeff.im.x[7] = rotation_7.imag();
                    Complex32x8 right_rotation_coeff = left_rotation_coeff;
                    Complex32x8 left_rotation_mul;
                    left_rotation_mul.re.x[0] = rotation_8.real();
                    left_rotation_mul.re.x[1] = rotation_8.real();
                    left_rotation_mul.re.x[2] = rotation_8.real();
                    left_rotation_mul.re.x[3] = rotation_8.real();
                    left_rotation_mul.re.x[4] = rotation_8.real();
                    left_rotation_mul.re.x[5] = rotation_8.real();
                    left_rotation_mul.re.x[6] = rotation_8.real();
                    left_rotation_mul.re.x[7] = rotation_8.real();
                    left_rotation_mul.im.x[0] = rotation_8.imag();
                    left_rotation_mul.im.x[1] = rotation_8.imag();
                    left_rotation_mul.im.x[2] = rotation_8.imag();
                    left_rotation_mul.im.x[3] = rotation_8.imag();
                    left_rotation_mul.im.x[4] = rotation_8.imag();
                    left_rotation_mul.im.x[5] = rotation_8.imag();
                    left_rotation_mul.im.x[6] = rotation_8.imag();
                    left_rotation_mul.im.x[7] = rotation_8.imag();
                    Complex32x8 right_rotation_mul = left_rotation_mul;

                    float left_re_sum = 0;
                    float left_im_sum = 0;
                    float right_re_sum = 0;
                    float right_im_sum = 0;
                    for (size_t i = 0; i < coeff_len_div8; ++i) {
                        Float32x8 left_taps_out = delay_left_.GetAfterPush(left_current_delay);
                        Float32x8 right_taps_out = delay_right_.GetAfterPush(right_current_delay);
                        left_current_delay += left_delay_inc;
                        right_current_delay += right_delay_inc;

                        left_taps_out *= last_coeffs_ptr[i];
                        Float32x8 temp = left_taps_out * left_rotation_coeff.re;
                        left_re_sum += temp.ReduceAdd();
                        temp = left_taps_out * left_rotation_coeff.im;
                        left_im_sum += temp.ReduceAdd();

                        right_taps_out *= last_coeffs_ptr[i];
                        temp = right_taps_out * right_rotation_coeff.re;
                        right_re_sum += temp.ReduceAdd();
                        temp = right_taps_out * right_rotation_coeff.im;
                        right_im_sum += temp.ReduceAdd();

                        left_rotation_coeff *= left_rotation_mul;
                        right_rotation_coeff *= right_rotation_mul;
                    }
                    
                    Float32x4 remove_positive_spectrum = hilbert_complex_.Tick(Float32x4{
                        left_re_sum, left_im_sum, right_re_sum, right_im_sum
                    });
                    // this will mirror the positive spectrum to negative domain, forming a real value signal
                    Float32x4 damp_x = remove_positive_spectrum.Shuffle<0, 2, 1, 3>();
                    *left_ptr = damp_x.x[0];
                    *right_ptr = damp_x.x[1];
                    ++left_ptr;
                    ++right_ptr;
                    damp_x = damp_.TickLowpass(damp_x, Float32x4::FromSingle(curr_damp_coeff));
                    left_fb_ = damp_x.x[0];
                    right_fb_ = damp_x.x[1];
                }

                barber_osc_keep_amp_counter_ += len;
                [[unlikely]]
                if (barber_osc_keep_amp_counter_ > barber_osc_keep_amp_need_) {
                    barber_osc_keep_amp_counter_ = 0;
                    barber_oscillator_.KeepAmp();
                }
            }
            last_delay_samples_ = last_exp_delay_samples_;
            last_damp_lowpass_coeff_ = damp_lowpass_coeff_;
        }
    }

    void ProcessVec4(
        float* left_ptr, float* right_ptr, size_t len,
        SteepFlangerParameter& param
    ) noexcept {
        size_t cando = len;
        while (cando != 0) {
            size_t num_process = std::min<size_t>(512, cando);
            cando -= num_process;

            if (param.should_update_fir_.exchange(false)) {
                UpdateCoeff(param);
            }

            float feedback_mul = 0;
            if (param.feedback_enable) {
                float db = param.feedback;
                float abs_db = std::abs(db);
                if (param.fir_min_phase) {
                    abs_db = std::max(abs_db, 4.1f);
                }
                float abs_gain = qwqdsp::convert::Db2Gain(-abs_db);
                abs_gain = std::min(abs_gain, 0.95f);
                if (db > 0) {
                    feedback_mul = -abs_gain;
                }
                else {
                    feedback_mul = abs_gain;
                }
            }

            float const damp_pitch = param.damp_pitch;
            float const damp_freq = qwqdsp::convert::Pitch2Freq(damp_pitch);
            float const damp_w = qwqdsp::convert::Freq2W(damp_freq, fs_);
            damp_lowpass_coeff_ = damp_.ComputeCoeff(damp_w);

            barber_phase_smoother_.SetTarget(param.barber_phase);
            barber_oscillator_.SetFreq(param.barber_speed, fs_);

            // update delay times
            phase_ += param.lfo_freq / fs_ * static_cast<float>(num_process);
            float right_phase = phase_ + param.lfo_phase;
            {
                float t;
                phase_ = std::modf(phase_, &t);
                right_phase = std::modf(right_phase, &t);
            }
            float left_phase = phase_;

            Float32x4 lfo_modu;
            lfo_modu.x[0] = qwqdsp::polymath::SinPi(left_phase * std::numbers::pi_v<float>);
            lfo_modu.x[1] = qwqdsp::polymath::SinPi(right_phase * std::numbers::pi_v<float>);

            float const delay_samples = param.delay_ms * fs_ / 1000.0f;
            float const depth_samples = param.depth_ms * fs_ / 1000.0f;
            Float32x4 target_delay_samples = Float32x4::FromSingle(delay_samples) + lfo_modu * Float32x4::FromSingle(depth_samples);
            target_delay_samples = Float32x4::Max(target_delay_samples, Float32x4::FromSingle(0.0f));
            float const delay_time_smooth_factor = 1.0f - std::exp(-1.0f / (fs_ / static_cast<float>(num_process) * kDelaySmoothMs / 1000.0f));
            last_exp_delay_samples_ += Float32x4::FromSingle(delay_time_smooth_factor) * (target_delay_samples - last_exp_delay_samples_);
            Float32x4 curr_num_notch = last_delay_samples_;
            Float32x4 delta_num_notch = (last_exp_delay_samples_ - curr_num_notch) / Float32x4::FromSingle(static_cast<float>(num_process));

            float curr_damp_coeff = last_damp_lowpass_coeff_;
            float delta_damp_coeff = (damp_lowpass_coeff_ - curr_damp_coeff) / (static_cast<float>(num_process));

            delay_left_.WrapBuffer();
            delay_right_.WrapBuffer();

            float const inv_samples = 1.0f / static_cast<float>(num_process);
            size_t const coeff_len_div_4 = (coeff_len_ + 3) / 4;
            std::array<Float32x4, kSIMDMaxCoeffLen / 4> delta_coeffs;
            Float32x4* coeffs_ptr = reinterpret_cast<Float32x4*>(coeffs_.data());
            Float32x4* last_coeffs_ptr = reinterpret_cast<Float32x4*>(last_coeffs_.data());
            for (size_t i = 0; i < coeff_len_div_4; ++i) {
                delta_coeffs[i] = (coeffs_ptr[i] - last_coeffs_ptr[i]) * Float32x4::FromSingle(inv_samples);
            }

            // fir polyphase filtering
            if (!param.barber_enable) {
                for (size_t j = 0; j < num_process; ++j) {
                    curr_num_notch += delta_num_notch;
                    curr_damp_coeff += delta_damp_coeff;

                    for (size_t i = 0; i < coeff_len_div_4; ++i) {
                        last_coeffs_ptr[i] += delta_coeffs[i];
                    }
        
                    float left_sum = 0;
                    float const left_num_notch = curr_num_notch.x[0];
                    Float32x4 current_delay;
                    current_delay.x[0] = 0;
                    current_delay.x[1] = left_num_notch;
                    current_delay.x[2] = left_num_notch * 2;
                    current_delay.x[3] = left_num_notch * 3;
                    Float32x4 delay_inc = Float32x4::FromSingle(left_num_notch * 4);
                    delay_left_.Push(*left_ptr + left_fb_ * feedback_mul);
                    for (size_t i = 0; i < coeff_len_div_4; ++i) {
                        Float32x4 taps_out = delay_left_.GetAfterPush(current_delay);
                        current_delay += delay_inc;

                        taps_out *= last_coeffs_ptr[i];
                        left_sum += taps_out.x[0];
                        left_sum += taps_out.x[1];
                        left_sum += taps_out.x[2];
                        left_sum += taps_out.x[3];
                    }

                    float right_sum = 0;
                    float const right_num_notch = curr_num_notch.x[1];
                    current_delay.x[0] = 0;
                    current_delay.x[1] = right_num_notch;
                    current_delay.x[2] = right_num_notch * 2;
                    current_delay.x[3] = right_num_notch * 3;
                    delay_inc = Float32x4::FromSingle(right_num_notch * 4);
                    delay_right_.Push(*right_ptr + right_fb_ * feedback_mul);
                    for (size_t i = 0; i < coeff_len_div_4; ++i) {
                        Float32x4 taps_out = delay_right_.GetAfterPush(current_delay);
                        current_delay += delay_inc;
                        taps_out *= last_coeffs_ptr[i];
                        right_sum += taps_out.x[0];
                        right_sum += taps_out.x[1];
                        right_sum += taps_out.x[2];
                        right_sum += taps_out.x[3];
                    }

                    Float32x4 damp_x;
                    damp_x.x[0] = left_sum;
                    damp_x.x[1] = right_sum;
                    *left_ptr = left_sum;
                    *right_ptr = right_sum;
                    ++left_ptr;
                    ++right_ptr;
                    damp_x = damp_.TickLowpass(damp_x, Float32x4::FromSingle(curr_damp_coeff));
                    left_fb_ = damp_x.x[0];
                    right_fb_ = damp_x.x[1];
                }
            }
            else {
                for (size_t j = 0; j < num_process; ++j) {
                    curr_damp_coeff += delta_damp_coeff;
                    curr_num_notch += delta_num_notch;

                    for (size_t i = 0; i < coeff_len_div_4; ++i) {
                        last_coeffs_ptr[i] += delta_coeffs[i];
                    }

                    delay_left_.Push(*left_ptr + left_fb_ * feedback_mul);
                    delay_right_.Push(*right_ptr + right_fb_ * feedback_mul);

                    float const left_num_notch = curr_num_notch.x[0];
                    float const right_num_notch = curr_num_notch.x[1];
                    Float32x4 left_current_delay;
                    Float32x4 right_current_delay;
                    left_current_delay.x[0] = 0;
                    left_current_delay.x[1] = left_num_notch;
                    left_current_delay.x[2] = left_num_notch * 2;
                    left_current_delay.x[3] = left_num_notch * 3;
                    right_current_delay.x[0] = 0;
                    right_current_delay.x[1] = right_num_notch;
                    right_current_delay.x[2] = right_num_notch * 2;
                    right_current_delay.x[3] = right_num_notch * 3;
                    Float32x4 left_delay_inc = Float32x4::FromSingle(left_num_notch * 4);
                    Float32x4 right_delay_inc = Float32x4::FromSingle(right_num_notch * 4);

                    auto const addition_rotation = std::polar(1.0f, barber_phase_smoother_.Tick() * std::numbers::pi_v<float> * 2);
                    barber_oscillator_.Tick();
                    auto const rotation_once = barber_oscillator_.GetCpx() * addition_rotation;
                    auto const rotation_2 = rotation_once * rotation_once;
                    auto const rotation_3 = rotation_once * rotation_2;
                    auto const rotation_4 = rotation_2 * rotation_2;
                    Complex32x4 left_rotation_coeff;
                    left_rotation_coeff.re.x[0] = 1;
                    left_rotation_coeff.re.x[1] = rotation_once.real();
                    left_rotation_coeff.re.x[2] = rotation_2.real();
                    left_rotation_coeff.re.x[3] = rotation_3.real();
                    left_rotation_coeff.im.x[0] = 0;
                    left_rotation_coeff.im.x[1] = rotation_once.imag();
                    left_rotation_coeff.im.x[2] = rotation_2.imag();
                    left_rotation_coeff.im.x[3] = rotation_3.imag();
                    Complex32x4 right_rotation_coeff = left_rotation_coeff;
                    Complex32x4 left_rotation_mul;
                    left_rotation_mul.re.x[0] = rotation_4.real();
                    left_rotation_mul.re.x[1] = rotation_4.real();
                    left_rotation_mul.re.x[2] = rotation_4.real();
                    left_rotation_mul.re.x[3] = rotation_4.real();
                    left_rotation_mul.im.x[0] = rotation_4.imag();
                    left_rotation_mul.im.x[1] = rotation_4.imag();
                    left_rotation_mul.im.x[2] = rotation_4.imag();
                    left_rotation_mul.im.x[3] = rotation_4.imag();
                    Complex32x4 right_rotation_mul = left_rotation_mul;

                    float left_re_sum = 0;
                    float left_im_sum = 0;
                    float right_re_sum = 0;
                    float right_im_sum = 0;
                    for (size_t i = 0; i < coeff_len_div_4; ++i) {
                        Float32x4 left_taps_out = delay_left_.GetAfterPush(left_current_delay);
                        Float32x4 right_taps_out = delay_right_.GetAfterPush(right_current_delay);
                        left_current_delay += left_delay_inc;
                        right_current_delay += right_delay_inc;

                        left_taps_out *= last_coeffs_ptr[i];
                        Float32x4 temp = left_taps_out * left_rotation_coeff.re;
                        left_re_sum += temp.x[0];
                        left_re_sum += temp.x[1];
                        left_re_sum += temp.x[2];
                        left_re_sum += temp.x[3];
                        temp = left_taps_out * left_rotation_coeff.im;
                        left_im_sum += temp.x[0];
                        left_im_sum += temp.x[1];
                        left_im_sum += temp.x[2];
                        left_im_sum += temp.x[3];

                        right_taps_out *= last_coeffs_ptr[i];
                        temp = right_taps_out * right_rotation_coeff.re;
                        right_re_sum += temp.x[0];
                        right_re_sum += temp.x[1];
                        right_re_sum += temp.x[2];
                        right_re_sum += temp.x[3];
                        temp = right_taps_out * right_rotation_coeff.im;
                        right_im_sum += temp.x[0];
                        right_im_sum += temp.x[1];
                        right_im_sum += temp.x[2];
                        right_im_sum += temp.x[3];

                        left_rotation_coeff *= left_rotation_mul;
                        right_rotation_coeff *= right_rotation_mul;
                    }
                    
                    Float32x4 remove_positive_spectrum = hilbert_complex_.Tick(Float32x4{
                        left_re_sum, left_im_sum, right_re_sum, right_im_sum
                    });
                    // this will mirror the positive spectrum to negative domain, forming a real value signal
                    Float32x4 damp_x = remove_positive_spectrum.Shuffle<0, 2, 1, 3>();
                    *left_ptr = damp_x.x[0];
                    *right_ptr = damp_x.x[1];
                    ++left_ptr;
                    ++right_ptr;
                    damp_x = damp_.TickLowpass(damp_x, Float32x4::FromSingle(curr_damp_coeff));
                    left_fb_ = damp_x.x[0];
                    right_fb_ = damp_x.x[1];
                }

                barber_osc_keep_amp_counter_ += len;
                [[unlikely]]
                if (barber_osc_keep_amp_counter_ > barber_osc_keep_amp_need_) {
                    barber_osc_keep_amp_counter_ = 0;
                    barber_oscillator_.KeepAmp();
                }
            }
            last_delay_samples_ = last_exp_delay_samples_;
            last_damp_lowpass_coeff_ = damp_lowpass_coeff_;
        }
    }

    /**
     * @param p [0, 1]
     */
    void SetLFOPhase(float p) noexcept {
        phase_ = p;
    }

    /**
     * @param p [0, 1]
     */
    void SetBarberLFOPhase(float p) noexcept {
        barber_oscillator_.Reset(p * std::numbers::pi_v<float> * 2);
    }

    // -------------------- lookup --------------------
    std::span<const float> GetUsingCoeffs() const noexcept {
        return {coeffs_.data(), coeff_len_};
    }

    int GetCpuArch() const noexcept {
        return cpu_arch_;
    }

    std::atomic<bool> have_new_coeff_{};
private:
    void UpdateCoeff(SteepFlangerParameter& param) noexcept {
        size_t coeff_len = static_cast<size_t>(param.fir_coeff_len);
        coeff_len_ = coeff_len;

        if (!param.is_using_custom_) {
            std::span<float> kernel{coeffs_.data(), coeff_len};
            float const cutoff_w = param.fir_cutoff;
            if (param.fir_highpass) {
                qwqdsp::filter::WindowFIR::Highpass(kernel, std::numbers::pi_v<float> - cutoff_w);
            }
            else {
                qwqdsp::filter::WindowFIR::Lowpass(kernel, cutoff_w);
            }
            float const beta = qwqdsp::window::Kaiser::Beta(param.fir_side_lobe);
            qwqdsp::window::Kaiser::ApplyWindow(kernel, beta, false);
        }
        else {
            std::copy_n(param.custom_coeffs_.begin(), coeff_len, coeffs_.begin());
        }

        if (cpu_arch_ == 1) {
            size_t const coeff_len_div_4 = (coeff_len + 3) / 4;
            size_t const idxend = coeff_len_div_4 * 4;
            for (size_t i = coeff_len; i < idxend; ++i) {
                coeffs_[i] = 0;
            }
        }
        else {
            size_t const coeff_len_div_8 = (coeff_len + 7) / 8;
            size_t const idxend = coeff_len_div_8 * 8;
            for (size_t i = coeff_len; i < idxend; ++i) {
                coeffs_[i] = 0;
            }
        }

        float pad[kFFTSize]{};
        std::span<float> kernel{coeffs_.data(), coeff_len};
        constexpr size_t num_bins = complex_fft_.NumBins(kFFTSize);
        std::array<float, num_bins> gains{};
        if (param.fir_min_phase || param.feedback_enable) {
            std::copy(kernel.begin(), kernel.end(), pad);
            complex_fft_.FFTGainPhase(pad, gains);
        }

        if (param.fir_min_phase) {
            float log_gains[num_bins]{};
            for (size_t i = 0; i < num_bins; ++i) {
                log_gains[i] = std::log(gains[i] + 1e-18f);
            }

            float phases[num_bins]{};
            complex_fft_.IFFT(pad, log_gains, phases);
            pad[0] = 0;
            pad[num_bins / 2] = 0;
            for (size_t i = num_bins / 2 + 1; i < num_bins; ++i) {
                pad[i] = -pad[i];
            }

            complex_fft_.FFT(pad, log_gains, phases);
            complex_fft_.IFFTGainPhase(pad, gains, phases);

            for (size_t i = 0; i < kernel.size(); ++i) {
                kernel[i] = pad[i];
            }
        }

        if (!param.feedback_enable) {
            float energy = 0;
            for (auto x : kernel) {
                energy += x * x;
            }
            float g = 1.0f / std::sqrt(energy + 1e-18f);
            for (auto& x : kernel) {
                x *= g;
            }
        }
        else {
            float const max_spectral_gain = *std::max_element(gains.begin(), gains.end());
            float const gain = 1.0f / (max_spectral_gain + 1e-10f);
            for (auto& x : kernel) {
                x *= gain;
            }
        }

        have_new_coeff_ = true;
    }

    int cpu_arch_{};
    float fs_{};

    static constexpr size_t kSIMDMaxCoeffLen = ((kMaxCoeffLen + 7) / 8) * 8;
    static constexpr float kDelaySmoothMs = 20.0f;

    Vec4DelayLine delay_left_;
    Vec4DelayLine delay_right_;
    // fir
    alignas(32) std::array<float, kSIMDMaxCoeffLen> coeffs_{};
    alignas(32) std::array<float, kSIMDMaxCoeffLen> last_coeffs_{};

    size_t coeff_len_{};

    // delay time lfo
    float phase_{};
    Float32x4 last_exp_delay_samples_{};
    Float32x4 last_delay_samples_{};

    // feedback
    float left_fb_{};
    float right_fb_{};
    qwqdsp::filter::OnePoleTPTSimd<Float32x4> damp_;
    float damp_lowpass_coeff_{1.0f};
    float last_damp_lowpass_coeff_{1.0f};

    // barberpole
    qwqdsp::filter::StereoIIRHilbertDeeperCpx<Float32x4> hilbert_complex_;
    qwqdsp::misc::ExpSmoother barber_phase_smoother_;
    qwqdsp::oscillor::VicSineOsc barber_oscillator_;
    size_t barber_osc_keep_amp_counter_{};
    size_t barber_osc_keep_amp_need_{};

    qwqdsp::spectral::ComplexFFT complex_fft_;
};

// ---------------------------------------- juce processor ----------------------------------------
class SteepFlangerAudioProcessor final : public juce::AudioProcessor
{
public:
    //==============================================================================
    SteepFlangerAudioProcessor();
    ~SteepFlangerAudioProcessor() override;

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

    juce::AudioParameterFloat* param_delay_ms_;
    juce::AudioParameterFloat* param_delay_depth_ms_;
    juce::AudioParameterFloat* param_lfo_speed_;
    juce::AudioParameterFloat* param_lfo_phase_;
    juce::AudioParameterFloat* param_fir_cutoff_;
    juce::AudioParameterFloat* param_fir_coeff_len_;
    juce::AudioParameterFloat* param_fir_side_lobe_;
    juce::AudioParameterBool* param_fir_min_phase_;
    juce::AudioParameterBool* param_fir_highpass_;
    juce::AudioParameterFloat* param_feedback_;
    juce::AudioParameterFloat* param_damp_pitch_;
    juce::AudioParameterBool* param_feedback_enable_;
    juce::AudioParameterFloat* param_barber_phase_;
    juce::AudioParameterFloat* param_barber_speed_;
    juce::AudioParameterBool* param_barber_enable_;
    
    SteepFlanger dsp_;
    SteepFlangerParameter dsp_param_;

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SteepFlangerAudioProcessor)
};
