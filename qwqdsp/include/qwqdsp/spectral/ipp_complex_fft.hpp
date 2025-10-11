#pragma once
#include <bit>
#include <ipp/ipps.h>

#ifndef QWQDSP_HAVE_IPP
#error "no intel ipp found"
#endif

namespace qwqdsp::spectral {
// output frequency: 0~2pi
class IppComplexFFT {
public:
    ~IppComplexFFT() {
        if (p_spec_) {
            ippsFree(p_spec_);
            p_spec_ = nullptr;
        }
        if (p_buffer_) {
            ippsFree(p_buffer_);
            p_buffer_ = nullptr;
        }
    }

    void Init(size_t fft_size) {
        fft_size_ = fft_size;

        int order = std::countr_zero(fft_size);
        int p_spec_size{};
        int p_spec_buffer_size{};
        int p_buffer_size{};
        int const flag = IPP_FFT_DIV_INV_BY_N;
        ippsFFTGetSize_C_32f(order, flag, ippAlgHintFast, &p_spec_size, &p_spec_buffer_size, &p_buffer_size);

        if (p_spec_) {
            ippsFree(p_spec_);
            p_spec_ = nullptr;
        }
        p_spec_ = ippsMalloc_8u(p_spec_size);
        if (p_spec_buffer_) {
            ippsFree(p_spec_buffer_);
            p_spec_buffer_ = nullptr;
        }
        p_spec_buffer_ = p_spec_buffer_size > 0 ? ippsMalloc_8u(p_spec_buffer_size) : nullptr;
        if (p_buffer_) {
            ippsFree(p_buffer_);
            p_buffer_ = nullptr;
        }
        p_buffer_ = p_buffer_size > 0 ? ippsMalloc_8u(p_buffer_size) : nullptr;

        ippsFFTInit_C_32f(&p_fft_spec_, order, flag, ippAlgHintFast, p_spec_, p_spec_buffer_);
        if (p_spec_buffer_) {
            ippsFree(p_spec_buffer_);
            p_spec_buffer_ = nullptr;
        }
    }

    void FFT(
        float const* in_real, float const* in_imag,
        float* out_real, float* out_imag
    ) noexcept {
        ippsFFTFwd_CToC_32f(
            in_real, in_imag,
            out_real, out_imag,
            p_fft_spec_, p_buffer_
        );
    }

    void IFFT(
        float const* in_real, float const* in_imag,
        float* out_real, float* out_imag
    ) noexcept {
        ippsFFTInv_CToC_32f(
            in_real, in_imag,
            out_real, out_imag,
            p_fft_spec_, p_buffer_
        );
    }
private:
    size_t fft_size_{};
    Ipp8u* p_spec_{};
    Ipp8u* p_spec_buffer_{};
    Ipp8u* p_buffer_{};
    IppsFFTSpec_C_32f* p_fft_spec_{};
};
}