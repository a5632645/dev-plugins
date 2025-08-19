#include "qwqdsp/spectral/real_fft.hpp"
#include "qwqdsp/spectral/complex_fft.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>

void cdft(int, int, float *, int *, float *);
void rdft(int, int, float *, int *, float *);
void ddct(int, int, float *, int *, float *);
void ddst(int, int, float *, int *, float *);
void bitrv2(int n, int *ip, float *a);
void bitrv2conj(int n, int *ip, float *a);
void cftfsub(int n, float *a, float *w);
void cftbsub(int n, float *a, float *w);
void cft1st(int n, float *a, float *w);
void makewt(int nw, int *ip, float *w);
void makect(int nc, int *ip, float *c);
void dfct(int, float *, float *, int *, float *);
void dfst(int, float *, float *, int *, float *);
void rftfsub(int n, float *a, int nc, float *c);
void rftbsub(int n, float *a, int nc, float *c);
void dctsub(int n, float *a, int nc, float *c);
void dstsub(int n, float *a, int nc, float *c);
void cftmdl(int n, int l, float *a, float *w);

namespace qwqdsp::spectral {
// --------------------------------------------------------------------------------
// real fft
// --------------------------------------------------------------------------------
void RealFFT::Init(size_t fft_size) {
    fft_size_ = fft_size;
    ip_.resize(2 + std::ceil(std::sqrt(fft_size / 2.0f)));
    w_.resize(fft_size / 2);
    buffer_.resize(fft_size);
    const size_t size4 = fft_size / 4;
    makewt(size4, ip_.data(), w_.data());
    makect(size4, ip_.data(), w_.data() + size4);
}

void RealFFT::FFT(std::span<const float> time, std::span<std::complex<float>> spectral) {
    assert(time.size() == fft_size_);
    assert(spectral.size() >= NumBins());

    std::copy(time.begin(), time.end(), buffer_.begin());
    rdft(fft_size_, 1, buffer_.data(), ip_.data(), w_.data());
    spectral.front().real(buffer_[0]);
    spectral.front().imag(0.0f);
    spectral[fft_size_ / 2].real(-buffer_[1]);
    spectral[fft_size_ / 2].imag(0.0f);
    const size_t n = fft_size_ / 2;
    for (size_t i = 1; i < n; ++i) {
        spectral[i].real(buffer_[i * 2]);
        spectral[i].imag(-buffer_[i * 2 + 1]);
    }
}

void RealFFT::FFT(std::span<const float> time, std::span<float> real, std::span<float> imag) {
    assert(time.size() == fft_size_);
    assert(real.size() >= NumBins());
    assert(imag.size() >= NumBins());

    std::copy(time.begin(), time.end(), buffer_.begin());
    rdft(fft_size_, 1, buffer_.data(), ip_.data(), w_.data());
    real.front() = buffer_[0];
    imag.front() = 0.0f;
    real[fft_size_ / 2] = -buffer_[1];
    imag[fft_size_ / 2] = 0.0f;
    const size_t n = fft_size_ / 2;
    for (size_t i = 1; i < n; ++i) {
        real[i] = buffer_[i * 2];
        imag[i] = -buffer_[i * 2 + 1];
    }
}

void RealFFT::IFFT(std::span<float> time, std::span<const std::complex<float>> spectral) {
    assert(time.size() == fft_size_);
    assert(spectral.size() >= NumBins());

    buffer_[0] = spectral.front().real();
    buffer_[1] = -spectral[fft_size_ / 2].real();
    const size_t n = fft_size_ / 2;
    for (size_t i = 1; i < n; ++i) {
        buffer_[2 * i] = spectral[i].real();
        buffer_[2 * i + 1] = -spectral[i].imag();
    }
    rdft(fft_size_, -1, buffer_.data(), ip_.data(), w_.data());
    float gain = 2.0f / fft_size_;
    for (size_t i = 0; i < fft_size_; ++i) {
        time[i] = buffer_[i] * gain;
    }
}

void RealFFT::IFFT(std::span<float> time, std::span<const float> real, std::span<const float> imag) {
    assert(time.size() == fft_size_);
    assert(real.size() >= NumBins());
    assert(imag.size() >= NumBins());

    buffer_[0] = real.front();
    buffer_[1] = -real[fft_size_ / 2];
    const size_t n = fft_size_ / 2;
    for (size_t i = 1; i < n; ++i) {
        buffer_[2 * i] = real[i];
        buffer_[2 * i + 1] = -imag[i];
    }
    rdft(fft_size_, -1, buffer_.data(), ip_.data(), w_.data());
    float gain = 2.0f / fft_size_;
    for (size_t i = 0; i < fft_size_; ++i) {
        time[i] = buffer_[i] * gain;
    }
}

// --------------------------------------------------------------------------------
// complex fft
// --------------------------------------------------------------------------------
struct ComplexFFTHelper {
    template<ComplexFFTResultType type>
    static inline void Init(ComplexFFT<type>& self, size_t fft_size) {
        self.fft_size_ = fft_size;
        self.ip_.resize(2 + std::ceil(std::sqrt(fft_size / 2.0f)));
        self.w_.resize(fft_size / 2);
        self.buffer_.resize(fft_size * 2);
        const size_t size4 = fft_size / 2;
        makewt(size4, self.ip_.data(), self.w_.data());
    }

    template<ComplexFFTResultType kType>
    static inline void Hilbert(
        ComplexFFT<kType>& self,
        std::span<const float> time,
        std::span<std::complex<float>> output
    ) {
        assert(time.size() == self.fft_size_);
        assert(output.size() == self.fft_size_);

        for (size_t i = 0; i < self.fft_size_; ++i) {
            self.buffer_[2 * i] = time[i];
            self.buffer_[2 * i + 1] = 0.0f;
        }
        cdft(self.fft_size_ * 2, 1, self.buffer_.data(), self.ip_.data(), self.w_.data());
        // Z[0] = X[0]
        self.buffer_[0] *= 0.5f;
        self.buffer_[1] *= 0.5f;
        // Z[N/2] = x[N/2]
        self.buffer_[self.fft_size_] *= 0.5f;
        self.buffer_[self.fft_size_ + 1] *= 0.5f;
        // Z[negative frequency] = 0
        for (size_t i = 1; i < self.fft_size_ / 2; ++i) {
            self.buffer_[2 * i] = 0.0f;
            self.buffer_[2 * i + 1] = 0.0f;
        }
        cdft(self.fft_size_ * 2, -1, self.buffer_.data(), self.ip_.data(), self.w_.data());
        // Z[n] = 2 * X[n]
        const float gain = 2.0f / self.fft_size_;
        for (size_t i = 0; i < self.fft_size_; ++i) {
            output[i].real(self.buffer_[i * 2] * gain);
            output[i].imag(self.buffer_[i * 2 + 1] * gain);
        }
    }

    template<ComplexFFTResultType kType>
    static inline void Hilbert(
        ComplexFFT<kType>&self,
        std::span<const float> time,
        std::span<float> real,
        std::span<float> imag
    ) {
        assert(time.size() == self.fft_size_);
        assert(real.size() == self.fft_size_);
        assert(imag.size() == self.fft_size_);

        for (size_t i = 0; i < self.fft_size_; ++i) {
            self.buffer_[2 * i] = time[i];
            self.buffer_[2 * i + 1] = 0.0f;
        }
        cdft(self.fft_size_ * 2, 1, self.buffer_.data(), self.ip_.data(), self.w_.data());
        // Z[0] = X[0]
        self.buffer_[0] *= 0.5f;
        self.buffer_[1] *= 0.5f;
        // Z[N/2] = x[N/2]
        self.buffer_[self.fft_size_] *= 0.5f;
        self.buffer_[self.fft_size_ + 1] *= 0.5f;
        // Z[negative frequency] = 0
        for (size_t i = 1; i < self.fft_size_ / 2; ++i) {
            self.buffer_[2 * i] = 0.0f;
            self.buffer_[2 * i + 1] = 0.0f;
        }
        cdft(self.fft_size_ * 2, -1, self.buffer_.data(), self.ip_.data(), self.w_.data());
        // Z[n] = 2 * X[n]
        const float gain = 2.0f / self.fft_size_;
        for (size_t i = 0; i < self.fft_size_; ++i) {
            real[i] = self.buffer_[i * 2] * gain;
            imag[i] = self.buffer_[i * 2 + 1] * gain;
        }
    }
};

template<>
void ComplexFFT<ComplexFFTResultType::kNegPiToPosPi>::Init(size_t fft_size) {
    ComplexFFTHelper::Init(*this, fft_size);
}

template<>
void ComplexFFT<ComplexFFTResultType::kZeroToTwoPi>::Init(size_t fft_size) {
    ComplexFFTHelper::Init(*this, fft_size);
}

template<>
void ComplexFFT<ComplexFFTResultType::kNegPiToPosPi>::Hilbert(
    std::span<const float> time,
    std::span<std::complex<float>> output
) {
    ComplexFFTHelper::Hilbert(*this, time, output);
}

template<>
void ComplexFFT<ComplexFFTResultType::kZeroToTwoPi>::Hilbert(
    std::span<const float> time,
    std::span<std::complex<float>> output
) {
    ComplexFFTHelper::Hilbert(*this, time, output);
}

template<>
void ComplexFFT<ComplexFFTResultType::kNegPiToPosPi>::Hilbert(
    std::span<const float> time,
    std::span<float> real,
    std::span<float> imag
) {
    ComplexFFTHelper::Hilbert(*this, time, real, imag);
}

template<>
void ComplexFFT<ComplexFFTResultType::kZeroToTwoPi>::Hilbert(
    std::span<const float> time,
    std::span<float> real,
    std::span<float> imag
) {
    ComplexFFTHelper::Hilbert(*this, time, real, imag);
}

template<>
void ComplexFFT<ComplexFFTResultType::kNegPiToPosPi>::FFT(std::span<const float> time, std::span<std::complex<float>> spectral) {
    assert(time.size() == fft_size_);
    assert(spectral.size() == NumBins());

    for (size_t i = 0; i < fft_size_; ++i) {
        buffer_[2 * i] = time[i];
        buffer_[2 * i + 1] = 0.0f;
    }
    cdft(fft_size_ * 2, 1, buffer_.data(), ip_.data(), w_.data());
    for (size_t i = 0; i <= fft_size_ / 2; ++i) {
        size_t e = fft_size_ / 2 - i;
        spectral[i].real(buffer_[e * 2]);
        spectral[i].imag(buffer_[e * 2 + 1]);
    }
    for (size_t i = 0; i < fft_size_ / 2 - 1; ++i) {
        size_t e = fft_size_ - 1 - i;
        size_t a = fft_size_ / 2 + 1 + i;
        spectral[a].real(buffer_[e * 2]);
        spectral[a].imag(buffer_[e * 2 + 1]);
    }
}

template<>
void ComplexFFT<ComplexFFTResultType::kZeroToTwoPi>::FFT(std::span<const float> time, std::span<std::complex<float>> spectral) {
    assert(time.size() == fft_size_);
    assert(spectral.size() == NumBins());

    for (size_t i = 0; i < fft_size_; ++i) {
        buffer_[2 * i] = time[i];
        buffer_[2 * i + 1] = 0.0f;
    }
    cdft(fft_size_ * 2, 1, buffer_.data(), ip_.data(), w_.data());
    spectral[0].real(buffer_[0]);
    spectral[0].imag(buffer_[1]);
    for (size_t i = 1; i < fft_size_; ++i) {
        spectral[fft_size_ - i].real(buffer_[i * 2]);
        spectral[fft_size_ - i].imag(buffer_[i * 2 + 1]);
    }
}

template<>
void ComplexFFT<ComplexFFTResultType::kNegPiToPosPi>::FFT(std::span<const std::complex<float>> time, std::span<std::complex<float>> spectral) {
    assert(time.size() == fft_size_);
    assert(spectral.size() == NumBins());

    for (size_t i = 0; i < fft_size_; ++i) {
        buffer_[2 * i] = time[i].real();
        buffer_[2 * i + 1] = time[i].imag();
    }
    cdft(fft_size_ * 2, 1, buffer_.data(), ip_.data(), w_.data());
    for (size_t i = 0; i <= fft_size_ / 2; ++i) {
        size_t e = fft_size_ / 2 - i;
        spectral[i].real(buffer_[e * 2]);
        spectral[i].imag(buffer_[e * 2 + 1]);
    }
    for (size_t i = 0; i < fft_size_ / 2 - 1; ++i) {
        size_t e = fft_size_ - 1 - i;
        size_t a = fft_size_ / 2 + 1 + i;
        spectral[a].real(buffer_[e * 2]);
        spectral[a].imag(buffer_[e * 2 + 1]);
    }
}

template<>
void ComplexFFT<ComplexFFTResultType::kZeroToTwoPi>::FFT(std::span<const std::complex<float>> time, std::span<std::complex<float>> spectral) {
    assert(time.size() == fft_size_);
    assert(spectral.size() == NumBins());

    for (size_t i = 0; i < fft_size_; ++i) {
        buffer_[2 * i] = time[i].real();
        buffer_[2 * i + 1] = time[i].imag();
    }
    cdft(fft_size_ * 2, 1, buffer_.data(), ip_.data(), w_.data());
    spectral[0].real(buffer_[0]);
    spectral[0].imag(buffer_[1]);
    for (size_t i = 1; i < fft_size_; ++i) {
        spectral[fft_size_ - i].real(buffer_[i * 2]);
        spectral[fft_size_ - i].imag(buffer_[i * 2 + 1]);
    }
}

template<>
void ComplexFFT<ComplexFFTResultType::kZeroToTwoPi>::IFFT(std::span<std::complex<float>> time, std::span<std::complex<float>> spectral) {
    buffer_[0] = spectral[0].real();
    buffer_[1] = spectral[1].imag();
    for (size_t i = 1; i < fft_size_; ++i) {
        buffer_[2 * i] = spectral[fft_size_ - i].real();
        buffer_[2 * i + 1] = spectral[fft_size_ - i].imag();
    }
    cdft(fft_size_ * 2, -1, buffer_.data(), ip_.data(), w_.data());
    const float gain = 1.0f / fft_size_;
    for (size_t i = 0; i < fft_size_; ++i) {
        time[i].real(buffer_[i * 2] * gain);
        time[i].imag(buffer_[i * 2 + 1] * gain);
    }
}

template<>
void ComplexFFT<ComplexFFTResultType::kNegPiToPosPi>::IFFT(std::span<std::complex<float>> time, std::span<std::complex<float>> spectral) {
    for (size_t i = 0; i <= fft_size_ / 2; ++i) {
        size_t a = fft_size_ / 2 - i;
        buffer_[2 * a] = spectral[i].real();
        buffer_[2 * a + 1] = spectral[i].imag();
    }
    for (size_t i = 0; i < fft_size_ / 2 - 1; ++i) {
        size_t a = fft_size_ / 2 + 1 + i;
        size_t e = fft_size_ - 1 - i;
        buffer_[2 * a] = spectral[e].real();
        buffer_[2 * a + 1] = spectral[e].imag();
    }
    cdft(fft_size_ * 2, -1, buffer_.data(), ip_.data(), w_.data());
    const float gain = 1.0f / fft_size_;
    for (size_t i = 0; i < fft_size_; ++i) {
        time[i].real(buffer_[i * 2] * gain);
        time[i].imag(buffer_[i * 2 + 1] * gain);
    }
}

}

// --------------------------------------------------------------------------------
// Copyright Takuya OOURA, 1996-2001
// You may use, copy, modify and distribute this code for any purpose (include commercial use) and without fee.
// Please refer to this package when you modify this code.
// --------------------------------------------------------------------------------
void cdft(int n, int isgn, float *a, int *ip, float *w)
{
    if (n > 4) {
        if (isgn >= 0) {
            bitrv2(n, ip + 2, a);
            cftfsub(n, a, w);
        } else {
            bitrv2conj(n, ip + 2, a);
            cftbsub(n, a, w);
        }
    } else if (n == 4) {
        cftfsub(n, a, w);
    }
}


void rdft(int n, int isgn, float *a, int *ip, float *w)
{
    int nw = ip[0];
    int nc = ip[1];
    if (isgn >= 0) {
        if (n > 4) {
            bitrv2(n, ip + 2, a);
            cftfsub(n, a, w);
            rftfsub(n, a, nc, w + nw);
        } else if (n == 4) {
            cftfsub(n, a, w);
        }
        float xi = a[0] - a[1];
        a[0] += a[1];
        a[1] = xi;
    } else {
        a[1] = 0.5 * (a[0] - a[1]);
        a[0] -= a[1];
        if (n > 4) {
            rftbsub(n, a, nc, w + nw);
            bitrv2(n, ip + 2, a);
            cftbsub(n, a, w);
        } else if (n == 4) {
            cftfsub(n, a, w);
        }
    }
}


void ddct(int n, int isgn, float *a, int *ip, float *w)
{
    int j, nw, nc;
    float xr;
    
    nw = ip[0];
    if (n > (nw << 2)) {
        nw = n >> 2;
        makewt(nw, ip, w);
    }
    nc = ip[1];
    if (n > nc) {
        nc = n;
        makect(nc, ip, w + nw);
    }
    if (isgn < 0) {
        xr = a[n - 1];
        for (j = n - 2; j >= 2; j -= 2) {
            a[j + 1] = a[j] - a[j - 1];
            a[j] += a[j - 1];
        }
        a[1] = a[0] - xr;
        a[0] += xr;
        if (n > 4) {
            rftbsub(n, a, nc, w + nw);
            bitrv2(n, ip + 2, a);
            cftbsub(n, a, w);
        } else if (n == 4) {
            cftfsub(n, a, w);
        }
    }
    dctsub(n, a, nc, w + nw);
    if (isgn >= 0) {
        if (n > 4) {
            bitrv2(n, ip + 2, a);
            cftfsub(n, a, w);
            rftfsub(n, a, nc, w + nw);
        } else if (n == 4) {
            cftfsub(n, a, w);
        }
        xr = a[0] - a[1];
        a[0] += a[1];
        for (j = 2; j < n; j += 2) {
            a[j - 1] = a[j] - a[j + 1];
            a[j] += a[j + 1];
        }
        a[n - 1] = xr;
    }
}


void ddst(int n, int isgn, float *a, int *ip, float *w)
{
    int j, nw, nc;
    float xr;
    
    nw = ip[0];
    if (n > (nw << 2)) {
        nw = n >> 2;
        makewt(nw, ip, w);
    }
    nc = ip[1];
    if (n > nc) {
        nc = n;
        makect(nc, ip, w + nw);
    }
    if (isgn < 0) {
        xr = a[n - 1];
        for (j = n - 2; j >= 2; j -= 2) {
            a[j + 1] = -a[j] - a[j - 1];
            a[j] -= a[j - 1];
        }
        a[1] = a[0] + xr;
        a[0] -= xr;
        if (n > 4) {
            rftbsub(n, a, nc, w + nw);
            bitrv2(n, ip + 2, a);
            cftbsub(n, a, w);
        } else if (n == 4) {
            cftfsub(n, a, w);
        }
    }
    dstsub(n, a, nc, w + nw);
    if (isgn >= 0) {
        if (n > 4) {
            bitrv2(n, ip + 2, a);
            cftfsub(n, a, w);
            rftfsub(n, a, nc, w + nw);
        } else if (n == 4) {
            cftfsub(n, a, w);
        }
        xr = a[0] - a[1];
        a[0] += a[1];
        for (j = 2; j < n; j += 2) {
            a[j - 1] = -a[j] - a[j + 1];
            a[j] -= a[j + 1];
        }
        a[n - 1] = -xr;
    }
}


void dfct(int n, float *a, float *t, int *ip, float *w)
{
    int j, k, l, m, mh, nw, nc;
    float xr, xi, yr, yi;
    
    nw = ip[0];
    if (n > (nw << 3)) {
        nw = n >> 3;
        makewt(nw, ip, w);
    }
    nc = ip[1];
    if (n > (nc << 1)) {
        nc = n >> 1;
        makect(nc, ip, w + nw);
    }
    m = n >> 1;
    yi = a[m];
    xi = a[0] + a[n];
    a[0] -= a[n];
    t[0] = xi - yi;
    t[m] = xi + yi;
    if (n > 2) {
        mh = m >> 1;
        for (j = 1; j < mh; j++) {
            k = m - j;
            xr = a[j] - a[n - j];
            xi = a[j] + a[n - j];
            yr = a[k] - a[n - k];
            yi = a[k] + a[n - k];
            a[j] = xr;
            a[k] = yr;
            t[j] = xi - yi;
            t[k] = xi + yi;
        }
        t[mh] = a[mh] + a[n - mh];
        a[mh] -= a[n - mh];
        dctsub(m, a, nc, w + nw);
        if (m > 4) {
            bitrv2(m, ip + 2, a);
            cftfsub(m, a, w);
            rftfsub(m, a, nc, w + nw);
        } else if (m == 4) {
            cftfsub(m, a, w);
        }
        a[n - 1] = a[0] - a[1];
        a[1] = a[0] + a[1];
        for (j = m - 2; j >= 2; j -= 2) {
            a[2 * j + 1] = a[j] + a[j + 1];
            a[2 * j - 1] = a[j] - a[j + 1];
        }
        l = 2;
        m = mh;
        while (m >= 2) {
            dctsub(m, t, nc, w + nw);
            if (m > 4) {
                bitrv2(m, ip + 2, t);
                cftfsub(m, t, w);
                rftfsub(m, t, nc, w + nw);
            } else if (m == 4) {
                cftfsub(m, t, w);
            }
            a[n - l] = t[0] - t[1];
            a[l] = t[0] + t[1];
            k = 0;
            for (j = 2; j < m; j += 2) {
                k += l << 2;
                a[k - l] = t[j] - t[j + 1];
                a[k + l] = t[j] + t[j + 1];
            }
            l <<= 1;
            mh = m >> 1;
            for (j = 0; j < mh; j++) {
                k = m - j;
                t[j] = t[m + k] - t[m + j];
                t[k] = t[m + k] + t[m + j];
            }
            t[mh] = t[m + mh];
            m = mh;
        }
        a[l] = t[0];
        a[n] = t[2] - t[1];
        a[0] = t[2] + t[1];
    } else {
        a[1] = a[0];
        a[2] = t[0];
        a[0] = t[1];
    }
}


void dfst(int n, float *a, float *t, int *ip, float *w)
{
    int j, k, l, m, mh, nw, nc;
    float xr, xi, yr, yi;
    
    nw = ip[0];
    if (n > (nw << 3)) {
        nw = n >> 3;
        makewt(nw, ip, w);
    }
    nc = ip[1];
    if (n > (nc << 1)) {
        nc = n >> 1;
        makect(nc, ip, w + nw);
    }
    if (n > 2) {
        m = n >> 1;
        mh = m >> 1;
        for (j = 1; j < mh; j++) {
            k = m - j;
            xr = a[j] + a[n - j];
            xi = a[j] - a[n - j];
            yr = a[k] + a[n - k];
            yi = a[k] - a[n - k];
            a[j] = xr;
            a[k] = yr;
            t[j] = xi + yi;
            t[k] = xi - yi;
        }
        t[0] = a[mh] - a[n - mh];
        a[mh] += a[n - mh];
        a[0] = a[m];
        dstsub(m, a, nc, w + nw);
        if (m > 4) {
            bitrv2(m, ip + 2, a);
            cftfsub(m, a, w);
            rftfsub(m, a, nc, w + nw);
        } else if (m == 4) {
            cftfsub(m, a, w);
        }
        a[n - 1] = a[1] - a[0];
        a[1] = a[0] + a[1];
        for (j = m - 2; j >= 2; j -= 2) {
            a[2 * j + 1] = a[j] - a[j + 1];
            a[2 * j - 1] = -a[j] - a[j + 1];
        }
        l = 2;
        m = mh;
        while (m >= 2) {
            dstsub(m, t, nc, w + nw);
            if (m > 4) {
                bitrv2(m, ip + 2, t);
                cftfsub(m, t, w);
                rftfsub(m, t, nc, w + nw);
            } else if (m == 4) {
                cftfsub(m, t, w);
            }
            a[n - l] = t[1] - t[0];
            a[l] = t[0] + t[1];
            k = 0;
            for (j = 2; j < m; j += 2) {
                k += l << 2;
                a[k - l] = -t[j] - t[j + 1];
                a[k + l] = t[j] - t[j + 1];
            }
            l <<= 1;
            mh = m >> 1;
            for (j = 1; j < mh; j++) {
                k = m - j;
                t[j] = t[m + k] + t[m + j];
                t[k] = t[m + k] - t[m + j];
            }
            t[0] = t[m + mh];
            m = mh;
        }
        a[l] = t[0];
    }
    a[0] = 0;
}


/* -------- initializing routines -------- */


#include <math.h>

void makewt(int nw, int *ip, float *w)
{
    void bitrv2(int n, int *ip, float *a);
    int j, nwh;
    float delta, x, y;
    
    ip[0] = nw;
    ip[1] = 1;
    if (nw > 2) {
        nwh = nw >> 1;
        delta = atan(1.0) / nwh;
        w[0] = 1;
        w[1] = 0;
        w[nwh] = cos(delta * nwh);
        w[nwh + 1] = w[nwh];
        if (nwh > 2) {
            for (j = 2; j < nwh; j += 2) {
                x = cos(delta * j);
                y = sin(delta * j);
                w[j] = x;
                w[j + 1] = y;
                w[nw - j] = y;
                w[nw - j + 1] = x;
            }
            bitrv2(nw, ip + 2, w);
        }
    }
}


void makect(int nc, int *ip, float *c)
{
    int j, nch;
    float delta;
    
    ip[1] = nc;
    if (nc > 1) {
        nch = nc >> 1;
        delta = atan(1.0) / nch;
        c[0] = cos(delta * nch);
        c[nch] = 0.5 * c[0];
        for (j = 1; j < nch; j++) {
            c[j] = 0.5 * cos(delta * j);
            c[nc - j] = 0.5 * sin(delta * j);
        }
    }
}


/* -------- child routines -------- */


void bitrv2(int n, int *ip, float *a)
{
    int j, j1, k, k1, l, m, m2;
    float xr, xi, yr, yi;
    
    ip[0] = 0;
    l = n;
    m = 1;
    while ((m << 3) < l) {
        l >>= 1;
        for (j = 0; j < m; j++) {
            ip[m + j] = ip[j] + l;
        }
        m <<= 1;
    }
    m2 = 2 * m;
    if ((m << 3) == l) {
        for (k = 0; k < m; k++) {
            for (j = 0; j < k; j++) {
                j1 = 2 * j + ip[k];
                k1 = 2 * k + ip[j];
                xr = a[j1];
                xi = a[j1 + 1];
                yr = a[k1];
                yi = a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
                j1 += m2;
                k1 += 2 * m2;
                xr = a[j1];
                xi = a[j1 + 1];
                yr = a[k1];
                yi = a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
                j1 += m2;
                k1 -= m2;
                xr = a[j1];
                xi = a[j1 + 1];
                yr = a[k1];
                yi = a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
                j1 += m2;
                k1 += 2 * m2;
                xr = a[j1];
                xi = a[j1 + 1];
                yr = a[k1];
                yi = a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
            }
            j1 = 2 * k + m2 + ip[k];
            k1 = j1 + m2;
            xr = a[j1];
            xi = a[j1 + 1];
            yr = a[k1];
            yi = a[k1 + 1];
            a[j1] = yr;
            a[j1 + 1] = yi;
            a[k1] = xr;
            a[k1 + 1] = xi;
        }
    } else {
        for (k = 1; k < m; k++) {
            for (j = 0; j < k; j++) {
                j1 = 2 * j + ip[k];
                k1 = 2 * k + ip[j];
                xr = a[j1];
                xi = a[j1 + 1];
                yr = a[k1];
                yi = a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
                j1 += m2;
                k1 += m2;
                xr = a[j1];
                xi = a[j1 + 1];
                yr = a[k1];
                yi = a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
            }
        }
    }
}


void bitrv2conj(int n, int *ip, float *a)
{
    int j, j1, k, k1, l, m, m2;
    float xr, xi, yr, yi;
    
    ip[0] = 0;
    l = n;
    m = 1;
    while ((m << 3) < l) {
        l >>= 1;
        for (j = 0; j < m; j++) {
            ip[m + j] = ip[j] + l;
        }
        m <<= 1;
    }
    m2 = 2 * m;
    if ((m << 3) == l) {
        for (k = 0; k < m; k++) {
            for (j = 0; j < k; j++) {
                j1 = 2 * j + ip[k];
                k1 = 2 * k + ip[j];
                xr = a[j1];
                xi = -a[j1 + 1];
                yr = a[k1];
                yi = -a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
                j1 += m2;
                k1 += 2 * m2;
                xr = a[j1];
                xi = -a[j1 + 1];
                yr = a[k1];
                yi = -a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
                j1 += m2;
                k1 -= m2;
                xr = a[j1];
                xi = -a[j1 + 1];
                yr = a[k1];
                yi = -a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
                j1 += m2;
                k1 += 2 * m2;
                xr = a[j1];
                xi = -a[j1 + 1];
                yr = a[k1];
                yi = -a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
            }
            k1 = 2 * k + ip[k];
            a[k1 + 1] = -a[k1 + 1];
            j1 = k1 + m2;
            k1 = j1 + m2;
            xr = a[j1];
            xi = -a[j1 + 1];
            yr = a[k1];
            yi = -a[k1 + 1];
            a[j1] = yr;
            a[j1 + 1] = yi;
            a[k1] = xr;
            a[k1 + 1] = xi;
            k1 += m2;
            a[k1 + 1] = -a[k1 + 1];
        }
    } else {
        a[1] = -a[1];
        a[m2 + 1] = -a[m2 + 1];
        for (k = 1; k < m; k++) {
            for (j = 0; j < k; j++) {
                j1 = 2 * j + ip[k];
                k1 = 2 * k + ip[j];
                xr = a[j1];
                xi = -a[j1 + 1];
                yr = a[k1];
                yi = -a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
                j1 += m2;
                k1 += m2;
                xr = a[j1];
                xi = -a[j1 + 1];
                yr = a[k1];
                yi = -a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
            }
            k1 = 2 * k + ip[k];
            a[k1 + 1] = -a[k1 + 1];
            a[k1 + m2 + 1] = -a[k1 + m2 + 1];
        }
    }
}


void cftfsub(int n, float *a, float *w)
{
    void cft1st(int n, float *a, float *w);
    void cftmdl(int n, int l, float *a, float *w);
    int j, j1, j2, j3, l;
    float x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;
    
    l = 2;
    if (n > 8) {
        cft1st(n, a, w);
        l = 8;
        while ((l << 2) < n) {
            cftmdl(n, l, a, w);
            l <<= 2;
        }
    }
    if ((l << 2) == n) {
        for (j = 0; j < l; j += 2) {
            j1 = j + l;
            j2 = j1 + l;
            j3 = j2 + l;
            x0r = a[j] + a[j1];
            x0i = a[j + 1] + a[j1 + 1];
            x1r = a[j] - a[j1];
            x1i = a[j + 1] - a[j1 + 1];
            x2r = a[j2] + a[j3];
            x2i = a[j2 + 1] + a[j3 + 1];
            x3r = a[j2] - a[j3];
            x3i = a[j2 + 1] - a[j3 + 1];
            a[j] = x0r + x2r;
            a[j + 1] = x0i + x2i;
            a[j2] = x0r - x2r;
            a[j2 + 1] = x0i - x2i;
            a[j1] = x1r - x3i;
            a[j1 + 1] = x1i + x3r;
            a[j3] = x1r + x3i;
            a[j3 + 1] = x1i - x3r;
        }
    } else {
        for (j = 0; j < l; j += 2) {
            j1 = j + l;
            x0r = a[j] - a[j1];
            x0i = a[j + 1] - a[j1 + 1];
            a[j] += a[j1];
            a[j + 1] += a[j1 + 1];
            a[j1] = x0r;
            a[j1 + 1] = x0i;
        }
    }
}


void cftbsub(int n, float *a, float *w)
{
    void cft1st(int n, float *a, float *w);
    void cftmdl(int n, int l, float *a, float *w);
    int j, j1, j2, j3, l;
    float x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;
    
    l = 2;
    if (n > 8) {
        cft1st(n, a, w);
        l = 8;
        while ((l << 2) < n) {
            cftmdl(n, l, a, w);
            l <<= 2;
        }
    }
    if ((l << 2) == n) {
        for (j = 0; j < l; j += 2) {
            j1 = j + l;
            j2 = j1 + l;
            j3 = j2 + l;
            x0r = a[j] + a[j1];
            x0i = -a[j + 1] - a[j1 + 1];
            x1r = a[j] - a[j1];
            x1i = -a[j + 1] + a[j1 + 1];
            x2r = a[j2] + a[j3];
            x2i = a[j2 + 1] + a[j3 + 1];
            x3r = a[j2] - a[j3];
            x3i = a[j2 + 1] - a[j3 + 1];
            a[j] = x0r + x2r;
            a[j + 1] = x0i - x2i;
            a[j2] = x0r - x2r;
            a[j2 + 1] = x0i + x2i;
            a[j1] = x1r - x3i;
            a[j1 + 1] = x1i - x3r;
            a[j3] = x1r + x3i;
            a[j3 + 1] = x1i + x3r;
        }
    } else {
        for (j = 0; j < l; j += 2) {
            j1 = j + l;
            x0r = a[j] - a[j1];
            x0i = -a[j + 1] + a[j1 + 1];
            a[j] += a[j1];
            a[j + 1] = -a[j + 1] - a[j1 + 1];
            a[j1] = x0r;
            a[j1 + 1] = x0i;
        }
    }
}


void cft1st(int n, float *a, float *w)
{
    int j, k1, k2;
    float wk1r, wk1i, wk2r, wk2i, wk3r, wk3i;
    float x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;
    
    x0r = a[0] + a[2];
    x0i = a[1] + a[3];
    x1r = a[0] - a[2];
    x1i = a[1] - a[3];
    x2r = a[4] + a[6];
    x2i = a[5] + a[7];
    x3r = a[4] - a[6];
    x3i = a[5] - a[7];
    a[0] = x0r + x2r;
    a[1] = x0i + x2i;
    a[4] = x0r - x2r;
    a[5] = x0i - x2i;
    a[2] = x1r - x3i;
    a[3] = x1i + x3r;
    a[6] = x1r + x3i;
    a[7] = x1i - x3r;
    wk1r = w[2];
    x0r = a[8] + a[10];
    x0i = a[9] + a[11];
    x1r = a[8] - a[10];
    x1i = a[9] - a[11];
    x2r = a[12] + a[14];
    x2i = a[13] + a[15];
    x3r = a[12] - a[14];
    x3i = a[13] - a[15];
    a[8] = x0r + x2r;
    a[9] = x0i + x2i;
    a[12] = x2i - x0i;
    a[13] = x0r - x2r;
    x0r = x1r - x3i;
    x0i = x1i + x3r;
    a[10] = wk1r * (x0r - x0i);
    a[11] = wk1r * (x0r + x0i);
    x0r = x3i + x1r;
    x0i = x3r - x1i;
    a[14] = wk1r * (x0i - x0r);
    a[15] = wk1r * (x0i + x0r);
    k1 = 0;
    for (j = 16; j < n; j += 16) {
        k1 += 2;
        k2 = 2 * k1;
        wk2r = w[k1];
        wk2i = w[k1 + 1];
        wk1r = w[k2];
        wk1i = w[k2 + 1];
        wk3r = wk1r - 2 * wk2i * wk1i;
        wk3i = 2 * wk2i * wk1r - wk1i;
        x0r = a[j] + a[j + 2];
        x0i = a[j + 1] + a[j + 3];
        x1r = a[j] - a[j + 2];
        x1i = a[j + 1] - a[j + 3];
        x2r = a[j + 4] + a[j + 6];
        x2i = a[j + 5] + a[j + 7];
        x3r = a[j + 4] - a[j + 6];
        x3i = a[j + 5] - a[j + 7];
        a[j] = x0r + x2r;
        a[j + 1] = x0i + x2i;
        x0r -= x2r;
        x0i -= x2i;
        a[j + 4] = wk2r * x0r - wk2i * x0i;
        a[j + 5] = wk2r * x0i + wk2i * x0r;
        x0r = x1r - x3i;
        x0i = x1i + x3r;
        a[j + 2] = wk1r * x0r - wk1i * x0i;
        a[j + 3] = wk1r * x0i + wk1i * x0r;
        x0r = x1r + x3i;
        x0i = x1i - x3r;
        a[j + 6] = wk3r * x0r - wk3i * x0i;
        a[j + 7] = wk3r * x0i + wk3i * x0r;
        wk1r = w[k2 + 2];
        wk1i = w[k2 + 3];
        wk3r = wk1r - 2 * wk2r * wk1i;
        wk3i = 2 * wk2r * wk1r - wk1i;
        x0r = a[j + 8] + a[j + 10];
        x0i = a[j + 9] + a[j + 11];
        x1r = a[j + 8] - a[j + 10];
        x1i = a[j + 9] - a[j + 11];
        x2r = a[j + 12] + a[j + 14];
        x2i = a[j + 13] + a[j + 15];
        x3r = a[j + 12] - a[j + 14];
        x3i = a[j + 13] - a[j + 15];
        a[j + 8] = x0r + x2r;
        a[j + 9] = x0i + x2i;
        x0r -= x2r;
        x0i -= x2i;
        a[j + 12] = -wk2i * x0r - wk2r * x0i;
        a[j + 13] = -wk2i * x0i + wk2r * x0r;
        x0r = x1r - x3i;
        x0i = x1i + x3r;
        a[j + 10] = wk1r * x0r - wk1i * x0i;
        a[j + 11] = wk1r * x0i + wk1i * x0r;
        x0r = x1r + x3i;
        x0i = x1i - x3r;
        a[j + 14] = wk3r * x0r - wk3i * x0i;
        a[j + 15] = wk3r * x0i + wk3i * x0r;
    }
}


void cftmdl(int n, int l, float *a, float *w)
{
    int j, j1, j2, j3, k, k1, k2, m, m2;
    float wk1r, wk1i, wk2r, wk2i, wk3r, wk3i;
    float x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;
    
    m = l << 2;
    for (j = 0; j < l; j += 2) {
        j1 = j + l;
        j2 = j1 + l;
        j3 = j2 + l;
        x0r = a[j] + a[j1];
        x0i = a[j + 1] + a[j1 + 1];
        x1r = a[j] - a[j1];
        x1i = a[j + 1] - a[j1 + 1];
        x2r = a[j2] + a[j3];
        x2i = a[j2 + 1] + a[j3 + 1];
        x3r = a[j2] - a[j3];
        x3i = a[j2 + 1] - a[j3 + 1];
        a[j] = x0r + x2r;
        a[j + 1] = x0i + x2i;
        a[j2] = x0r - x2r;
        a[j2 + 1] = x0i - x2i;
        a[j1] = x1r - x3i;
        a[j1 + 1] = x1i + x3r;
        a[j3] = x1r + x3i;
        a[j3 + 1] = x1i - x3r;
    }
    wk1r = w[2];
    for (j = m; j < l + m; j += 2) {
        j1 = j + l;
        j2 = j1 + l;
        j3 = j2 + l;
        x0r = a[j] + a[j1];
        x0i = a[j + 1] + a[j1 + 1];
        x1r = a[j] - a[j1];
        x1i = a[j + 1] - a[j1 + 1];
        x2r = a[j2] + a[j3];
        x2i = a[j2 + 1] + a[j3 + 1];
        x3r = a[j2] - a[j3];
        x3i = a[j2 + 1] - a[j3 + 1];
        a[j] = x0r + x2r;
        a[j + 1] = x0i + x2i;
        a[j2] = x2i - x0i;
        a[j2 + 1] = x0r - x2r;
        x0r = x1r - x3i;
        x0i = x1i + x3r;
        a[j1] = wk1r * (x0r - x0i);
        a[j1 + 1] = wk1r * (x0r + x0i);
        x0r = x3i + x1r;
        x0i = x3r - x1i;
        a[j3] = wk1r * (x0i - x0r);
        a[j3 + 1] = wk1r * (x0i + x0r);
    }
    k1 = 0;
    m2 = 2 * m;
    for (k = m2; k < n; k += m2) {
        k1 += 2;
        k2 = 2 * k1;
        wk2r = w[k1];
        wk2i = w[k1 + 1];
        wk1r = w[k2];
        wk1i = w[k2 + 1];
        wk3r = wk1r - 2 * wk2i * wk1i;
        wk3i = 2 * wk2i * wk1r - wk1i;
        for (j = k; j < l + k; j += 2) {
            j1 = j + l;
            j2 = j1 + l;
            j3 = j2 + l;
            x0r = a[j] + a[j1];
            x0i = a[j + 1] + a[j1 + 1];
            x1r = a[j] - a[j1];
            x1i = a[j + 1] - a[j1 + 1];
            x2r = a[j2] + a[j3];
            x2i = a[j2 + 1] + a[j3 + 1];
            x3r = a[j2] - a[j3];
            x3i = a[j2 + 1] - a[j3 + 1];
            a[j] = x0r + x2r;
            a[j + 1] = x0i + x2i;
            x0r -= x2r;
            x0i -= x2i;
            a[j2] = wk2r * x0r - wk2i * x0i;
            a[j2 + 1] = wk2r * x0i + wk2i * x0r;
            x0r = x1r - x3i;
            x0i = x1i + x3r;
            a[j1] = wk1r * x0r - wk1i * x0i;
            a[j1 + 1] = wk1r * x0i + wk1i * x0r;
            x0r = x1r + x3i;
            x0i = x1i - x3r;
            a[j3] = wk3r * x0r - wk3i * x0i;
            a[j3 + 1] = wk3r * x0i + wk3i * x0r;
        }
        wk1r = w[k2 + 2];
        wk1i = w[k2 + 3];
        wk3r = wk1r - 2 * wk2r * wk1i;
        wk3i = 2 * wk2r * wk1r - wk1i;
        for (j = k + m; j < l + (k + m); j += 2) {
            j1 = j + l;
            j2 = j1 + l;
            j3 = j2 + l;
            x0r = a[j] + a[j1];
            x0i = a[j + 1] + a[j1 + 1];
            x1r = a[j] - a[j1];
            x1i = a[j + 1] - a[j1 + 1];
            x2r = a[j2] + a[j3];
            x2i = a[j2 + 1] + a[j3 + 1];
            x3r = a[j2] - a[j3];
            x3i = a[j2 + 1] - a[j3 + 1];
            a[j] = x0r + x2r;
            a[j + 1] = x0i + x2i;
            x0r -= x2r;
            x0i -= x2i;
            a[j2] = -wk2i * x0r - wk2r * x0i;
            a[j2 + 1] = -wk2i * x0i + wk2r * x0r;
            x0r = x1r - x3i;
            x0i = x1i + x3r;
            a[j1] = wk1r * x0r - wk1i * x0i;
            a[j1 + 1] = wk1r * x0i + wk1i * x0r;
            x0r = x1r + x3i;
            x0i = x1i - x3r;
            a[j3] = wk3r * x0r - wk3i * x0i;
            a[j3 + 1] = wk3r * x0i + wk3i * x0r;
        }
    }
}


void rftfsub(int n, float *a, int nc, float *c)
{
    int j, k, kk, ks, m;
    float wkr, wki, xr, xi, yr, yi;
    
    m = n >> 1;
    ks = 2 * nc / m;
    kk = 0;
    for (j = 2; j < m; j += 2) {
        k = n - j;
        kk += ks;
        wkr = 0.5 - c[nc - kk];
        wki = c[kk];
        xr = a[j] - a[k];
        xi = a[j + 1] + a[k + 1];
        yr = wkr * xr - wki * xi;
        yi = wkr * xi + wki * xr;
        a[j] -= yr;
        a[j + 1] -= yi;
        a[k] += yr;
        a[k + 1] -= yi;
    }
}


void rftbsub(int n, float *a, int nc, float *c)
{
    int j, k, kk, ks, m;
    float wkr, wki, xr, xi, yr, yi;
    
    a[1] = -a[1];
    m = n >> 1;
    ks = 2 * nc / m;
    kk = 0;
    for (j = 2; j < m; j += 2) {
        k = n - j;
        kk += ks;
        wkr = 0.5 - c[nc - kk];
        wki = c[kk];
        xr = a[j] - a[k];
        xi = a[j + 1] + a[k + 1];
        yr = wkr * xr + wki * xi;
        yi = wkr * xi - wki * xr;
        a[j] -= yr;
        a[j + 1] = yi - a[j + 1];
        a[k] += yr;
        a[k + 1] = yi - a[k + 1];
    }
    a[m + 1] = -a[m + 1];
}


void dctsub(int n, float *a, int nc, float *c)
{
    int j, k, kk, ks, m;
    float wkr, wki, xr;
    
    m = n >> 1;
    ks = nc / n;
    kk = 0;
    for (j = 1; j < m; j++) {
        k = n - j;
        kk += ks;
        wkr = c[kk] - c[nc - kk];
        wki = c[kk] + c[nc - kk];
        xr = wki * a[j] - wkr * a[k];
        a[j] = wkr * a[j] + wki * a[k];
        a[k] = xr;
    }
    a[m] *= c[0];
}


void dstsub(int n, float *a, int nc, float *c)
{
    int j, k, kk, ks, m;
    float wkr, wki, xr;
    
    m = n >> 1;
    ks = nc / n;
    kk = 0;
    for (j = 1; j < m; j++) {
        k = n - j;
        kk += ks;
        wkr = c[kk] - c[nc - kk];
        wki = c[kk] + c[nc - kk];
        xr = wki * a[k] - wkr * a[j];
        a[k] = wkr * a[k] + wki * a[j];
        a[j] = xr;
    }
    a[m] *= c[0];
}

