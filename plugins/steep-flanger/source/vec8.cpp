#include "PluginProcessor.h"

#include "qwqdsp/convert.hpp"
#include "qwqdsp/polymath.hpp"

QWQDSP_FORCE_INLINE
constexpr Complex32x8& Complex32x8::operator*=(const Complex32x8& a) noexcept {
    Float32x8 new_re = re * a.re - im * a.im;
    Float32x8 new_im = re * a.im + im * a.re;
    re = new_re;
    im = new_im;
    return *this;
}

QWQDSP_FORCE_INLINE
Float32x8 Vec4DelayLine::GetAfterPush(Float32x8 const& delay_samples) const noexcept {
    Float32x8 frpos = Float32x8::FromSingle(static_cast<float>(wpos_ + mask_)) - delay_samples;
    Int32x8 rpos = frpos.ToInt();
    Int32x8 mask = Int32x8::FromSingle(mask_);
    Int32x8 irpos = rpos & mask;
    Float32x8 frac = frpos.Frac();

    Float32x8 interp0;
    simde_mm_store_ps(interp0.x, simde_mm_loadu_ps(buffer_.data() + irpos.x[0]));
    Float32x8 interp1;
    simde_mm_store_ps(interp1.x, simde_mm_loadu_ps(buffer_.data() + irpos.x[1]));
    Float32x8 interp2;
    simde_mm_store_ps(interp2.x, simde_mm_loadu_ps(buffer_.data() + irpos.x[2]));
    Float32x8 interp3;
    simde_mm_store_ps(interp3.x, simde_mm_loadu_ps(buffer_.data() + irpos.x[3]));
    Float32x8 interp4;
    simde_mm_store_ps(interp4.x, simde_mm_loadu_ps(buffer_.data() + irpos.x[4]));
    Float32x8 interp5;
    simde_mm_store_ps(interp5.x, simde_mm_loadu_ps(buffer_.data() + irpos.x[5]));
    Float32x8 interp6;
    simde_mm_store_ps(interp6.x, simde_mm_loadu_ps(buffer_.data() + irpos.x[6]));
    Float32x8 interp7;
    simde_mm_store_ps(interp7.x, simde_mm_loadu_ps(buffer_.data() + irpos.x[7]));

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

void SteepFlanger::ProcessVec8(
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

        constexpr float kWarpFactor = -0.8f;
        float warp_drywet = param.drywet;
        warp_drywet = 2.0f * warp_drywet - 1.0f;
        warp_drywet = (warp_drywet - kWarpFactor) / (1.0f - warp_drywet * kWarpFactor);
        warp_drywet = 0.5f * warp_drywet + 0.5f;
        warp_drywet = std::clamp(warp_drywet, 0.0f, 1.0f);

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
        feedback_mul *= warp_drywet;

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
        float const wet_mix = param.drywet;
        Float32x8 dry_coeff{1.0f - wet_mix, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
        for (size_t i = 0; i < coeff_len_div8; ++i) {
            Float32x8 target_wet_coeff = coeffs_ptr[i] * Float32x8::FromSingle(wet_mix) + dry_coeff;
            delta_coeffs[i] = (target_wet_coeff - last_coeffs_ptr[i]) * Float32x8::FromSingle(inv_samples);
            dry_coeff = Float32x8::FromSingle(0.0f);
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
                right_rotation_coeff *= Complex32x8{
                    .re = Float32x8::FromSingle(std::cos(param.barber_stereo_phase)),
                    .im = Float32x8::FromSingle(std::sin(param.barber_stereo_phase))
                };
                Complex32x8 left_rotation_mul{
                    .re = Float32x8::FromSingle(rotation_8.real()),
                    .im = Float32x8::FromSingle(rotation_8.imag())
                };
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