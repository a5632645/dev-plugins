#ifdef QWQDSP_HAVE_IPP
#pragma once

#include <bit>
#include <cassert>
#include <ipp/ipps.h>

namespace qwqdsp_spectral {
class IppRealFFT {
public:
    ~IppRealFFT() {
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
        assert(std::has_single_bit(fft_size));
        fft_size_ = fft_size;

        int order = std::countr_zero(fft_size);
        int p_spec_size{};
        int p_spec_buffer_size{};
        int p_buffer_size{};
        int const flag = IPP_FFT_DIV_INV_BY_N;
        ippsFFTGetSize_R_32f(order, flag, ippAlgHintFast, &p_spec_size, &p_spec_buffer_size, &p_buffer_size);

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

        ippsFFTInit_R_32f(&p_fft_spec_, order, flag, ippAlgHintFast, p_spec_, p_spec_buffer_);
        if (p_spec_buffer_) {
            ippsFree(p_spec_buffer_);
            p_spec_buffer_ = nullptr;
        }
    }

    /**
     * @param input  [re im] x num_bins
     * @param output [re im] x num_bins
     */
    void FFT(float const* input, float* output) noexcept {
        ippsFFTFwd_RToCCS_32f(input, output, p_fft_spec_, p_buffer_);
    }
    
    /**
     * @param input  [re im] x num_bins
     * @param output [re im] x num_bins
     */
    void IFFT(float const* input, float* output) noexcept {
        ippsFFTInv_CCSToR_32f(input, output, p_fft_spec_, p_buffer_);
    }

    size_t GetFFTSize() const noexcept {
        return fft_size_;
    }
private:
    size_t fft_size_{};
    Ipp8u* p_spec_{};
    Ipp8u* p_spec_buffer_{};
    Ipp8u* p_buffer_{};
    IppsFFTSpec_R_32f* p_fft_spec_{};
};
}
#endif
