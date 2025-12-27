#include "PluginProcessor.h"

#include "qwqdsp/convert.hpp"
#include "qwqdsp/polymath.hpp"

QWQDSP_FORCE_INLINE
constexpr Complex32x8& Complex32x8::operator*=(const Complex32x8& a) noexcept {
    qwqdsp_simd_element::PackFloat<8> new_re = re * a.re - im * a.im;
    qwqdsp_simd_element::PackFloat<8> new_im = re * a.im + im * a.re;
    re = new_re;
    im = new_im;
    return *this;
}

QWQDSP_FORCE_INLINE
qwqdsp_simd_element::PackFloat<8> Vec4DelayLine::GetAfterPush(qwqdsp_simd_element::PackFloat<8> const& delay_samples) const noexcept {
    auto frpos = static_cast<float>(wpos_ + delay_length_) - delay_samples;
    auto t = qwqdsp_simd_element::PackOps::Frac(frpos);
    auto rpos = frpos.ToUint() - 1u;
    auto irpos = rpos & mask_;

    qwqdsp_simd_element::PackFloat<8> interp0;
    simde_mm_store_ps(interp0.data, simde_mm_loadu_ps(buffer_.data() + irpos[0]));
    qwqdsp_simd_element::PackFloat<8> interp1;
    simde_mm_store_ps(interp1.data, simde_mm_loadu_ps(buffer_.data() + irpos[1]));
    qwqdsp_simd_element::PackFloat<8> interp2;
    simde_mm_store_ps(interp2.data, simde_mm_loadu_ps(buffer_.data() + irpos[2]));
    qwqdsp_simd_element::PackFloat<8> interp3;
    simde_mm_store_ps(interp3.data, simde_mm_loadu_ps(buffer_.data() + irpos[3]));
    qwqdsp_simd_element::PackFloat<8> interp4;
    simde_mm_store_ps(interp4.data, simde_mm_loadu_ps(buffer_.data() + irpos[4]));
    qwqdsp_simd_element::PackFloat<8> interp5;
    simde_mm_store_ps(interp5.data, simde_mm_loadu_ps(buffer_.data() + irpos[5]));
    qwqdsp_simd_element::PackFloat<8> interp6;
    simde_mm_store_ps(interp6.data, simde_mm_loadu_ps(buffer_.data() + irpos[6]));
    qwqdsp_simd_element::PackFloat<8> interp7;
    simde_mm_store_ps(interp7.data, simde_mm_loadu_ps(buffer_.data() + irpos[7]));

    qwqdsp_simd_element::PackFloat<8> yn1;
    yn1[0] = interp0[0];
    yn1[1] = interp1[0];
    yn1[2] = interp2[0];
    yn1[3] = interp3[0];
    yn1[4] = interp4[0];
    yn1[5] = interp5[0];
    yn1[6] = interp6[0];
    yn1[7] = interp7[0];
    qwqdsp_simd_element::PackFloat<8> y0;
    y0[0] = interp0[1];
    y0[1] = interp1[1];
    y0[2] = interp2[1];
    y0[3] = interp3[1];
    y0[4] = interp4[1];
    y0[5] = interp5[1];
    y0[6] = interp6[1];
    y0[7] = interp7[1];
    qwqdsp_simd_element::PackFloat<8> y1;
    y1[0] = interp0[2];
    y1[1] = interp1[2];
    y1[2] = interp2[2];
    y1[3] = interp3[2];
    y1[4] = interp4[2];
    y1[5] = interp5[2];
    y1[6] = interp6[2];
    y1[7] = interp7[2];
    qwqdsp_simd_element::PackFloat<8> y2;
    y2[0] = interp0[3];
    y2[1] = interp1[3];
    y2[2] = interp2[3];
    y2[3] = interp3[3];
    y2[4] = interp4[3];
    y2[5] = interp5[3];
    y2[6] = interp6[3];
    y2[7] = interp7[3];

    auto d0 = (y1 - yn1) * 0.5f;
    auto d1 = (y2 - y0) * 0.5f;
    auto d = y1 - y0;
    auto m0 = 3.0f * d - 2.0f * d0 - d1;
    auto m1 = d0 - 2.0f * d + d1;
    return y0 + t * (
        d0 + t * (
            m0 + t * m1
        )
    );
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
        float feedback_mul = warp_drywet * param.feedback;

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

        qwqdsp_simd_element::PackFloat<4> lfo_modu;
        lfo_modu[0] = qwqdsp::polymath::SinPi(left_phase * std::numbers::pi_v<float>);
        lfo_modu[1] = qwqdsp::polymath::SinPi(right_phase * std::numbers::pi_v<float>);

        float const delay_samples = param.delay_ms * fs_ / 1000.0f;
        float const depth_samples = param.depth_ms * fs_ / 1000.0f;
        qwqdsp_simd_element::PackFloat<4> target_delay_samples = delay_samples + lfo_modu * depth_samples;
        target_delay_samples = qwqdsp_simd_element::PackOps::Max(target_delay_samples, qwqdsp_simd_element::PackFloat<4>::vBroadcast(0.0f));
        float const delay_time_smooth_factor = 1.0f - std::exp(-1.0f / (fs_ / static_cast<float>(num_process) * kDelaySmoothMs / 1000.0f));
        last_exp_delay_samples_ += delay_time_smooth_factor * (target_delay_samples - last_exp_delay_samples_);
        auto curr_num_notch = last_delay_samples_;
        auto delta_num_notch = (last_exp_delay_samples_ - curr_num_notch) / static_cast<float>(num_process);

        float curr_damp_coeff = last_damp_lowpass_coeff_;
        float delta_damp_coeff = (damp_lowpass_coeff_ - curr_damp_coeff) / (static_cast<float>(num_process));

        delay_left_.WrapBuffer();
        delay_right_.WrapBuffer();

        float inv_samples = 1.0f / static_cast<float>(num_process);
        std::array<qwqdsp_simd_element::PackFloat<8>, kSIMDMaxCoeffLen / 8> delta_coeffs;
        auto* coeffs_ptr = reinterpret_cast<qwqdsp_simd_element::PackFloat<8>*>(coeffs_.data());
        auto* last_coeffs_ptr = reinterpret_cast<qwqdsp_simd_element::PackFloat<8>*>(last_coeffs_.data());
        size_t const coeff_len_div8 = (coeff_len_ + 7) / 8;
        float const wet_mix = param.drywet;
        qwqdsp_simd_element::PackFloat<8> dry_coeff{1.0f - wet_mix, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
        for (size_t i = 0; i < coeff_len_div8; ++i) {
            qwqdsp_simd_element::PackFloat<8> target_wet_coeff = coeffs_ptr[i] * wet_mix + dry_coeff;
            delta_coeffs[i] = (target_wet_coeff - last_coeffs_ptr[i]) * inv_samples;
            dry_coeff.Broadcast(0);
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
                float const left_num_notch = curr_num_notch[0];
                qwqdsp_simd_element::PackFloat<8> current_delay;
                current_delay[0] = 0;
                current_delay[1] = left_num_notch;
                current_delay[2] = left_num_notch * 2;
                current_delay[3] = left_num_notch * 3;
                current_delay[4] = left_num_notch * 4;
                current_delay[5] = left_num_notch * 5;
                current_delay[6] = left_num_notch * 6;
                current_delay[7] = left_num_notch * 7;
                auto delay_inc = qwqdsp_simd_element::PackFloat<8>::vBroadcast(left_num_notch * 8);
                delay_left_.Push(*left_ptr + left_fb_ * feedback_mul);
                for (size_t i = 0; i < coeff_len_div8; ++i) {
                    auto taps_out = delay_left_.GetAfterPush(current_delay);
                    current_delay += delay_inc;

                    taps_out *= last_coeffs_ptr[i];
                    left_sum += qwqdsp_simd_element::PackOps::ReduceAdd(taps_out);
                }

                float right_sum = 0;
                float const right_num_notch = curr_num_notch[1];
                current_delay[0] = 0;
                current_delay[1] = right_num_notch;
                current_delay[2] = right_num_notch * 2;
                current_delay[3] = right_num_notch * 3;
                current_delay[4] = right_num_notch * 4;
                current_delay[5] = right_num_notch * 5;
                current_delay[6] = right_num_notch * 6;
                current_delay[7] = right_num_notch * 7;
                delay_inc = qwqdsp_simd_element::PackFloat<8>::vBroadcast(right_num_notch * 8);
                delay_right_.Push(*right_ptr + right_fb_ * feedback_mul);
                for (size_t i = 0; i < coeff_len_div8; ++i) {
                    auto taps_out = delay_right_.GetAfterPush(current_delay);
                    current_delay += delay_inc;
                    taps_out *= last_coeffs_ptr[i];
                    right_sum += qwqdsp_simd_element::PackOps::ReduceAdd(taps_out);
                }

                qwqdsp_simd_element::PackFloat<4> damp_x;
                damp_x[0] = left_sum;
                damp_x[1] = right_sum;
                *left_ptr = left_sum * fir_gain_;
                *right_ptr = right_sum * fir_gain_;
                ++left_ptr;
                ++right_ptr;
                damp_x = damp_.TickLowpass(damp_x, qwqdsp_simd_element::PackFloat<4>::vBroadcast(curr_damp_coeff));
                auto dc_remove = dc_.TickHighpass(damp_x, qwqdsp_simd_element::PackFloat<4>::vBroadcast(0.0005f));
                left_fb_ = qwqdsp::polymath::ArctanPade(dc_remove[0]);
                right_fb_ = qwqdsp::polymath::ArctanPade(dc_remove[1]);
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

                float const left_num_notch = curr_num_notch[0];
                float const right_num_notch = curr_num_notch[1];
                qwqdsp_simd_element::PackFloat<8> left_current_delay;
                qwqdsp_simd_element::PackFloat<8> right_current_delay;
                left_current_delay[0] = 0;
                left_current_delay[1] = left_num_notch;
                left_current_delay[2] = left_num_notch * 2;
                left_current_delay[3] = left_num_notch * 3;
                left_current_delay[4] = left_num_notch * 4;
                left_current_delay[5] = left_num_notch * 5;
                left_current_delay[6] = left_num_notch * 6;
                left_current_delay[7] = left_num_notch * 7;
                right_current_delay[0] = 0;
                right_current_delay[1] = right_num_notch;
                right_current_delay[2] = right_num_notch * 2;
                right_current_delay[3] = right_num_notch * 3;
                right_current_delay[4] = right_num_notch * 4;
                right_current_delay[5] = right_num_notch * 5;
                right_current_delay[6] = right_num_notch * 6;
                right_current_delay[7] = right_num_notch * 7;
                auto left_delay_inc = qwqdsp_simd_element::PackFloat<8>::vBroadcast(left_num_notch * 8);
                auto right_delay_inc = qwqdsp_simd_element::PackFloat<8>::vBroadcast(right_num_notch * 8);

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
                left_rotation_coeff.re[0] = 1;
                left_rotation_coeff.re[1] = rotation_once.real();
                left_rotation_coeff.re[2] = rotation_2.real();
                left_rotation_coeff.re[3] = rotation_3.real();
                left_rotation_coeff.re[4] = rotation_4.real();
                left_rotation_coeff.re[5] = rotation_5.real();
                left_rotation_coeff.re[6] = rotation_6.real();
                left_rotation_coeff.re[7] = rotation_7.real();
                left_rotation_coeff.im[0] = 0;
                left_rotation_coeff.im[1] = rotation_once.imag();
                left_rotation_coeff.im[2] = rotation_2.imag();
                left_rotation_coeff.im[3] = rotation_3.imag();
                left_rotation_coeff.im[4] = rotation_4.imag();
                left_rotation_coeff.im[5] = rotation_5.imag();
                left_rotation_coeff.im[6] = rotation_6.imag();
                left_rotation_coeff.im[7] = rotation_7.imag();
                Complex32x8 right_rotation_coeff = left_rotation_coeff;
                right_rotation_coeff *= Complex32x8{
                    .re = qwqdsp_simd_element::PackFloat<8>::vBroadcast(std::cos(param.barber_stereo_phase)),
                    .im = qwqdsp_simd_element::PackFloat<8>::vBroadcast(std::sin(param.barber_stereo_phase))
                };
                Complex32x8 left_rotation_mul{
                    .re = qwqdsp_simd_element::PackFloat<8>::vBroadcast(rotation_8.real()),
                    .im = qwqdsp_simd_element::PackFloat<8>::vBroadcast(rotation_8.imag())
                };
                Complex32x8 right_rotation_mul = left_rotation_mul;

                float left_re_sum = 0;
                float left_im_sum = 0;
                float right_re_sum = 0;
                float right_im_sum = 0;
                for (size_t i = 0; i < coeff_len_div8; ++i) {
                    auto left_taps_out = delay_left_.GetAfterPush(left_current_delay);
                    auto right_taps_out = delay_right_.GetAfterPush(right_current_delay);
                    left_current_delay += left_delay_inc;
                    right_current_delay += right_delay_inc;

                    left_taps_out *= last_coeffs_ptr[i];
                    auto temp = left_taps_out * left_rotation_coeff.re;
                    left_re_sum += qwqdsp_simd_element::PackOps::ReduceAdd(temp);
                    temp = left_taps_out * left_rotation_coeff.im;
                    left_im_sum += qwqdsp_simd_element::PackOps::ReduceAdd(temp);

                    right_taps_out *= last_coeffs_ptr[i];
                    temp = right_taps_out * right_rotation_coeff.re;
                    right_re_sum += qwqdsp_simd_element::PackOps::ReduceAdd(temp);
                    temp = right_taps_out * right_rotation_coeff.im;
                    right_im_sum += qwqdsp_simd_element::PackOps::ReduceAdd(temp);

                    left_rotation_coeff *= left_rotation_mul;
                    right_rotation_coeff *= right_rotation_mul;
                }
                
                auto remove_positive_spectrum = hilbert_complex_.Tick(qwqdsp_simd_element::PackFloat<4>{
                    left_re_sum, left_im_sum, right_re_sum, right_im_sum
                });
                // this will mirror the positive spectrum to negative domain, forming a real value signal
                auto damp_x = qwqdsp_simd_element::PackOps::Shuffle<0, 2, 1, 3>(remove_positive_spectrum);
                *left_ptr = damp_x[0] * fir_gain_;
                *right_ptr = damp_x[1] * fir_gain_;
                ++left_ptr;
                ++right_ptr;
                damp_x = damp_.TickLowpass(damp_x, qwqdsp_simd_element::PackFloat<4>::vBroadcast(curr_damp_coeff));
                auto dc_remove = dc_.TickHighpass(damp_x, qwqdsp_simd_element::PackFloat<4>::vBroadcast(0.0005f));
                left_fb_ = qwqdsp::polymath::ArctanPade(dc_remove[0]);
                right_fb_ = qwqdsp::polymath::ArctanPade(dc_remove[1]);
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
