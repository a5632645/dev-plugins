#pragma once
#include <atomic>
#include <array>

#include <qwqdsp/extension_marcos.hpp>
#include <qwqdsp/filter/window_fir.hpp>
#include <qwqdsp/misc/smoother.hpp>
#include <qwqdsp/oscillator/vic_sine_osc.hpp>
#include <qwqdsp/simd_element/align_allocator.hpp>
#include <qwqdsp/simd_element/simd_element.hpp>
#include <qwqdsp/spectral/complex_fft.hpp>
#include <qwqdsp/window/kaiser.hpp>

#include "shared.hpp"
#include "simd_detector.h"
#include "x86/avx.h"
#include "x86/sse4.1.h"

struct Complex32x4 {
    qwqdsp_simd_element::PackFloat<4> re;
    qwqdsp_simd_element::PackFloat<4> im;

    QWQDSP_FORCE_INLINE
    constexpr Complex32x4& operator*=(const Complex32x4& a) noexcept;
};

struct Complex32x8 {
    qwqdsp_simd_element::PackFloat<8> re;
    qwqdsp_simd_element::PackFloat<8> im;

    QWQDSP_FORCE_INLINE
    constexpr Complex32x8& operator*=(const Complex32x8& a) noexcept;
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
        mask_ = static_cast<uint32_t>(a - 1);
        delay_length_ = static_cast<uint32_t>(a);
        buffer_.resize(a * 2);
    }

    void Reset() noexcept {
        wpos_ = 0;
        std::fill(buffer_.begin(), buffer_.end(), 0.0f);
    }

    QWQDSP_FORCE_INLINE
    qwqdsp_simd_element::PackFloat<4> GetAfterPush(
        qwqdsp_simd_element::PackFloat<4> const& delay_samples) const noexcept;

    QWQDSP_FORCE_INLINE
    qwqdsp_simd_element::PackFloat<8> GetAfterPush(
        qwqdsp_simd_element::PackFloat<8> const& delay_samples) const noexcept;

    QWQDSP_FORCE_INLINE
    void Push(float x) noexcept {
        wpos_ = (wpos_ + 1) & mask_;
        buffer_[wpos_] = x;
        buffer_[wpos_ + delay_length_] = x;
    }

    QWQDSP_FORCE_INLINE
    void WrapBuffer() noexcept {
        // auto a = simde_mm_load_ps(buffer_.data());
        // simde_mm_store_ps(buffer_.data() + delay_length_, a);
    }
private:
    std::vector<float, qwqdsp_simd_element::AlignedAllocator<float, 32>> buffer_;
    uint32_t delay_length_{};
    uint32_t wpos_{};
    uint32_t mask_{};
};

class SteepFlangerParameter {
public:
    // => is mapping internal
    float delay_ms;       // >=0
    float depth_ms;       // >=0
    float lfo_freq;       // hz
    float lfo_phase;      // 0~1 => 0~2pi
    float fir_cutoff;     // 0~pi
    size_t fir_coeff_len; // 4~kMaxCoeffLen
    float fir_side_lobe;  // >20
    bool fir_min_phase;
    bool fir_highpass;
    float feedback; // gain
    float damp_pitch;
    float barber_phase; // 0~1 => 0~2pi
    float barber_speed; // hz
    bool barber_enable;
    float barber_stereo_phase;              // 0~pi/2
    float drywet;                           // 0~1
    std::atomic<bool> should_update_fir_{}; // tell flanger to update coeffs
    std::atomic<bool> is_using_custom_{};
    std::array<float, kMaxCoeffLen> custom_coeffs_{};
    std::array<float, kMaxCoeffLen> custom_spectral_gains{};
};

class SteepFlanger {
public:
    enum class ProcessArch {
        kVector4,
        kVector8,
        kNothing
    };

    SteepFlanger() {
        complex_fft_.Init(kFFTSize);

        process_arch_ = ProcessArch::kNothing;
        if (simd_detector::is_supported(simd_detector::InstructionSet::PLUGIN_VEC8_DISPATCH_ISET)) {
            process_arch_ = ProcessArch::kVector8;
        }
        else if (simd_detector::is_supported(simd_detector::InstructionSet::PLUGIN_VEC4_DISPATCH_ISET)) {
            process_arch_ = ProcessArch::kVector4;
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

    void Process(float* left_ptr, float* right_ptr, size_t len, SteepFlangerParameter& param) noexcept {
        if (process_arch_ == ProcessArch::kVector4) {
            ProcessVec4(left_ptr, right_ptr, len, param);
        }
        else if (process_arch_ == ProcessArch::kVector8) {
            ProcessVec8(left_ptr, right_ptr, len, param);
        }
    }

    void ProcessVec8(float* left_ptr, float* right_ptr, size_t len, SteepFlangerParameter& param) noexcept;

    void ProcessVec4(float* left_ptr, float* right_ptr, size_t len, SteepFlangerParameter& param) noexcept;

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

    ProcessArch GetProcessArch() const noexcept {
        return process_arch_;
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
                qwqdsp_filter::WindowFIR::Highpass(kernel, std::numbers::pi_v<float> - cutoff_w);
            }
            else {
                qwqdsp_filter::WindowFIR::Lowpass(kernel, cutoff_w);
            }
            float const beta = qwqdsp_window::Kaiser::Beta(param.fir_side_lobe);
            qwqdsp_window::Kaiser::ApplyWindow(kernel, beta, false);
        }
        else {
            std::copy_n(param.custom_coeffs_.begin(), coeff_len, coeffs_.begin());
        }

        if (process_arch_ == ProcessArch::kVector4) {
            size_t const coeff_len_div_4 = (coeff_len + 3) / 4;
            size_t const idxend = coeff_len_div_4 * 4;
            for (size_t i = coeff_len; i < idxend; ++i) {
                coeffs_[i] = 0;
            }
        }
        else if (process_arch_ == ProcessArch::kVector8) {
            size_t const coeff_len_div_8 = (coeff_len + 7) / 8;
            size_t const idxend = coeff_len_div_8 * 8;
            for (size_t i = coeff_len; i < idxend; ++i) {
                coeffs_[i] = 0;
            }
        }

        std::span<float> kernel{coeffs_.data(), coeff_len};
        float pad[kFFTSize]{};
        constexpr size_t num_bins = qwqdsp_spectral::ComplexFFT::NumBins(kFFTSize);
        std::array<float, num_bins> gains{};
        std::copy(kernel.begin(), kernel.end(), pad);
        complex_fft_.FFTGainPhase(pad, gains);
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

        float const max_spectral_gain = *std::max_element(gains.begin(), gains.end());
        float gain = 1.0f / (max_spectral_gain + 1e-10f);
        if (max_spectral_gain < 1e-10f) {
            gain = 1.0f;
        }
        for (auto& x : kernel) {
            x *= gain;
        }

        float energy = 0;
        for (auto x : kernel) {
            energy += x * x;
        }
        fir_gain_ = 1.0f / std::sqrt(energy + 1e-10f);

        have_new_coeff_ = true;
    }

    static constexpr size_t kSIMDMaxCoeffLen = ((kMaxCoeffLen + 7) / 8) * 8;
    static constexpr float kDelaySmoothMs = 20.0f;

    ProcessArch process_arch_{};
    float fs_{};

    Vec4DelayLine delay_left_;
    Vec4DelayLine delay_right_;
    // fir
    alignas(32) std::array<float, kSIMDMaxCoeffLen> coeffs_{};
    alignas(32) std::array<float, kSIMDMaxCoeffLen> last_coeffs_{};
    float fir_gain_{1.0f};
    size_t coeff_len_{};

    // delay time lfo
    float phase_{};
    qwqdsp_simd_element::PackFloat<4> last_exp_delay_samples_{};
    qwqdsp_simd_element::PackFloat<4> last_delay_samples_{};

    // feedback
    float left_fb_{};
    float right_fb_{};
    qwqdsp_simd_element::OnePoleTPT<4> damp_;
    qwqdsp_simd_element::OnePoleTPT<4> dc_;
    float damp_lowpass_coeff_{1.0f};
    float last_damp_lowpass_coeff_{1.0f};

    // barberpole
    qwqdsp_simd_element::StereoIIRHilbertDeeperCpx<4> hilbert_complex_;
    qwqdsp_misc::ExpSmoother barber_phase_smoother_;
    qwqdsp_oscillator::VicSineOsc barber_oscillator_;
    size_t barber_osc_keep_amp_counter_{};
    size_t barber_osc_keep_amp_need_{};

    qwqdsp_spectral::ComplexFFT complex_fft_;
};
