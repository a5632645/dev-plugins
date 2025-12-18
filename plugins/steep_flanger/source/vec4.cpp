#include "steep_flanger.hpp"

#include <qwqdsp/convert.hpp>
#include <qwqdsp/polymath.hpp>

QWQDSP_FORCE_INLINE
constexpr Complex32x4& Complex32x4::operator*=(const Complex32x4& a) noexcept {
    auto new_re = re * a.re - im * a.im;
    auto new_im = re * a.im + im * a.re;
    re = new_re;
    im = new_im;
    return *this;
}

QWQDSP_FORCE_INLINE
qwqdsp_simd_element::PackFloat<4> Vec4DelayLine::GetAfterPush(qwqdsp_simd_element::PackFloat<4> const& delay_samples) const noexcept {
    auto frpos = qwqdsp_simd_element::PackFloat<4>::vBroadcast(static_cast<float>(wpos_ + mask_)) - delay_samples;
    auto rpos = frpos.ToInt();
    auto mask = qwqdsp_simd_element::PackInt32<4>::vBroadcast(mask_);
    auto irpos = rpos & mask;
    auto frac = qwqdsp_simd_element::PackOps::Frac(frpos);

    qwqdsp_simd_element::PackFloat<4> interp0;
    qwqdsp_simd_element::PackFloat<4> interp1;
    qwqdsp_simd_element::PackFloat<4> interp2;
    qwqdsp_simd_element::PackFloat<4> interp3;
    simde_mm_store_ps(interp0.data, simde_mm_loadu_ps(buffer_.data() + irpos[0]));
    simde_mm_store_ps(interp1.data, simde_mm_loadu_ps(buffer_.data() + irpos[1]));
    simde_mm_store_ps(interp2.data, simde_mm_loadu_ps(buffer_.data() + irpos[2]));
    simde_mm_store_ps(interp3.data, simde_mm_loadu_ps(buffer_.data() + irpos[3]));

    qwqdsp_simd_element::PackFloat<4> y0;
    y0[0] = interp0[0];
    y0[1] = interp1[0];
    y0[2] = interp2[0];
    y0[3] = interp3[0];
    qwqdsp_simd_element::PackFloat<4> y1;
    y1[0] = interp0[1];
    y1[1] = interp1[1];
    y1[2] = interp2[1];
    y1[3] = interp3[1];
    qwqdsp_simd_element::PackFloat<4> y2;
    y2[0] = interp0[2];
    y2[1] = interp1[2];
    y2[2] = interp2[2];
    y2[3] = interp3[2];
    qwqdsp_simd_element::PackFloat<4> y3;
    y3[0] = interp0[3];
    y3[1] = interp1[3];
    y3[2] = interp2[3];
    y3[3] = interp3[3];

    auto d1 = frac - 1.0f;
    auto d2 = frac - 2.0f;
    auto d3 = frac - 3.0f;

    auto c1 = d1 * d2 * d3 / -(6.0f);
    auto c2 = d2 * d3 * (0.5f);
    auto c3 = d1 * d3 * -(0.5f);
    auto c4 = d1 * d2 / (6.0f);

    return y0 * c1 + frac * (y1 * c2 + y2 * c3 + y3 * c4);
}

void SteepFlanger::ProcessVec4(
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

        qwqdsp_simd_element::PackFloat<4> lfo_modu;
        lfo_modu[0] = qwqdsp::polymath::SinPi(left_phase * std::numbers::pi_v<float>);
        lfo_modu[1] = qwqdsp::polymath::SinPi(right_phase * std::numbers::pi_v<float>);

        float const delay_samples = param.delay_ms * fs_ / 1000.0f;
        float const depth_samples = param.depth_ms * fs_ / 1000.0f;
        auto target_delay_samples = delay_samples + lfo_modu * depth_samples;
        target_delay_samples = qwqdsp_simd_element::PackOps::Max(target_delay_samples, qwqdsp_simd_element::PackFloat<4>::vBroadcast(0.0f));
        float const delay_time_smooth_factor = 1.0f - std::exp(-1.0f / (fs_ / static_cast<float>(num_process) * kDelaySmoothMs / 1000.0f));
        last_exp_delay_samples_ += delay_time_smooth_factor * (target_delay_samples - last_exp_delay_samples_);
        auto curr_num_notch = last_delay_samples_;
        auto delta_num_notch = (last_exp_delay_samples_ - curr_num_notch) / static_cast<float>(num_process);

        float curr_damp_coeff = last_damp_lowpass_coeff_;
        float delta_damp_coeff = (damp_lowpass_coeff_ - curr_damp_coeff) / (static_cast<float>(num_process));

        delay_left_.WrapBuffer();
        delay_right_.WrapBuffer();

        float const inv_samples = 1.0f / static_cast<float>(num_process);
        size_t const coeff_len_div_4 = (coeff_len_ + 3) / 4;
        std::array<qwqdsp_simd_element::PackFloat<4>, kSIMDMaxCoeffLen / 4> delta_coeffs;
        auto* coeffs_ptr = reinterpret_cast<qwqdsp_simd_element::PackFloat<4>*>(coeffs_.data());
        auto* last_coeffs_ptr = reinterpret_cast<qwqdsp_simd_element::PackFloat<4>*>(last_coeffs_.data());
        float const wet_mix = param.drywet;
        qwqdsp_simd_element::PackFloat<4> dry_coeff{1.0f - wet_mix, 0.0f, 0.0f, 0.0f};
        for (size_t i = 0; i < coeff_len_div_4; ++i) {
            auto target_wet_coeff = coeffs_ptr[i] * wet_mix + dry_coeff;
            delta_coeffs[i] = (target_wet_coeff - last_coeffs_ptr[i]) * inv_samples;
            dry_coeff.Broadcast(0);
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
                float const left_num_notch = curr_num_notch[0];
                qwqdsp_simd_element::PackFloat<4> current_delay;
                current_delay[0] = 0;
                current_delay[1] = left_num_notch;
                current_delay[2] = left_num_notch * 2;
                current_delay[3] = left_num_notch * 3;
                auto delay_inc = qwqdsp_simd_element::PackFloat<4>::vBroadcast(left_num_notch * 4);
                delay_left_.Push(*left_ptr + left_fb_ * feedback_mul);
                for (size_t i = 0; i < coeff_len_div_4; ++i) {
                    auto taps_out = delay_left_.GetAfterPush(current_delay);
                    current_delay += delay_inc;

                    taps_out *= last_coeffs_ptr[i];
                    left_sum += taps_out[0];
                    left_sum += taps_out[1];
                    left_sum += taps_out[2];
                    left_sum += taps_out[3];
                }

                float right_sum = 0;
                float const right_num_notch = curr_num_notch[1];
                current_delay[0] = 0;
                current_delay[1] = right_num_notch;
                current_delay[2] = right_num_notch * 2;
                current_delay[3] = right_num_notch * 3;
                delay_inc = qwqdsp_simd_element::PackFloat<4>::vBroadcast(right_num_notch * 4);
                delay_right_.Push(*right_ptr + right_fb_ * feedback_mul);
                for (size_t i = 0; i < coeff_len_div_4; ++i) {
                    auto taps_out = delay_right_.GetAfterPush(current_delay);
                    current_delay += delay_inc;
                    taps_out *= last_coeffs_ptr[i];
                    right_sum += taps_out[0];
                    right_sum += taps_out[1];
                    right_sum += taps_out[2];
                    right_sum += taps_out[3];
                }

                qwqdsp_simd_element::PackFloat<4> damp_x;
                damp_x[0] = left_sum;
                damp_x[1] = right_sum;
                *left_ptr = left_sum * fir_gain_;
                *right_ptr = right_sum * fir_gain_;
                ++left_ptr;
                ++right_ptr;
                damp_x = damp_.TickLowpass(damp_x, qwqdsp_simd_element::PackFloat<4>::vBroadcast(curr_damp_coeff));
                left_fb_ = damp_x[0];
                right_fb_ = damp_x[1];
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

                float const left_num_notch = curr_num_notch[0];
                float const right_num_notch = curr_num_notch[1];
                qwqdsp_simd_element::PackFloat<4> left_current_delay;
                qwqdsp_simd_element::PackFloat<4> right_current_delay;
                left_current_delay[0] = 0;
                left_current_delay[1] = left_num_notch;
                left_current_delay[2] = left_num_notch * 2;
                left_current_delay[3] = left_num_notch * 3;
                right_current_delay[0] = 0;
                right_current_delay[1] = right_num_notch;
                right_current_delay[2] = right_num_notch * 2;
                right_current_delay[3] = right_num_notch * 3;
                auto left_delay_inc = qwqdsp_simd_element::PackFloat<4>::vBroadcast(left_num_notch * 4);
                auto right_delay_inc = qwqdsp_simd_element::PackFloat<4>::vBroadcast(right_num_notch * 4);

                auto const addition_rotation = std::polar(1.0f, barber_phase_smoother_.Tick() * std::numbers::pi_v<float> * 2);
                barber_oscillator_.Tick();
                auto const rotation_once = barber_oscillator_.GetCpx() * addition_rotation;
                auto const rotation_2 = rotation_once * rotation_once;
                auto const rotation_3 = rotation_once * rotation_2;
                auto const rotation_4 = rotation_2 * rotation_2;
                auto const right_channel_rotation = std::polar(1.0f, param.barber_stereo_phase);
                Complex32x4 left_rotation_coeff;
                left_rotation_coeff.re[0] = 1;
                left_rotation_coeff.re[1] = rotation_once.real();
                left_rotation_coeff.re[2] = rotation_2.real();
                left_rotation_coeff.re[3] = rotation_3.real();
                left_rotation_coeff.im[0] = 0;
                left_rotation_coeff.im[1] = rotation_once.imag();
                left_rotation_coeff.im[2] = rotation_2.imag();
                left_rotation_coeff.im[3] = rotation_3.imag();
                Complex32x4 right_rotation_coeff = left_rotation_coeff;
                right_rotation_coeff *= Complex32x4{
                    .re = qwqdsp_simd_element::PackFloat<4>::vBroadcast(right_channel_rotation.real()),
                    .im = qwqdsp_simd_element::PackFloat<4>::vBroadcast(right_channel_rotation.imag())
                };
                Complex32x4 left_rotation_mul{
                    .re = qwqdsp_simd_element::PackFloat<4>::vBroadcast(rotation_4.real()),
                    .im = qwqdsp_simd_element::PackFloat<4>::vBroadcast(rotation_4.imag())
                };
                Complex32x4 right_rotation_mul = left_rotation_mul;

                float left_re_sum = 0;
                float left_im_sum = 0;
                float right_re_sum = 0;
                float right_im_sum = 0;
                for (size_t i = 0; i < coeff_len_div_4; ++i) {
                    auto left_taps_out = delay_left_.GetAfterPush(left_current_delay);
                    auto right_taps_out = delay_right_.GetAfterPush(right_current_delay);
                    left_current_delay += left_delay_inc;
                    right_current_delay += right_delay_inc;

                    left_taps_out *= last_coeffs_ptr[i];
                    auto temp = left_taps_out * left_rotation_coeff.re;
                    left_re_sum += temp[0];
                    left_re_sum += temp[1];
                    left_re_sum += temp[2];
                    left_re_sum += temp[3];
                    temp = left_taps_out * left_rotation_coeff.im;
                    left_im_sum += temp[0];
                    left_im_sum += temp[1];
                    left_im_sum += temp[2];
                    left_im_sum += temp[3];

                    right_taps_out *= last_coeffs_ptr[i];
                    temp = right_taps_out * right_rotation_coeff.re;
                    right_re_sum += temp[0];
                    right_re_sum += temp[1];
                    right_re_sum += temp[2];
                    right_re_sum += temp[3];
                    temp = right_taps_out * right_rotation_coeff.im;
                    right_im_sum += temp[0];
                    right_im_sum += temp[1];
                    right_im_sum += temp[2];
                    right_im_sum += temp[3];

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
                left_fb_ = damp_x[0];
                right_fb_ = damp_x[1];
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
