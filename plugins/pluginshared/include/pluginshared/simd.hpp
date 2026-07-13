#pragma once
#include <array>
#include <bit>
#include <complex>
#include <cstddef>
#include <type_traits>

#include <x86/avx2.h>
#include <x86/sse4.1.h>

namespace simd {
#if defined(__clang__)
using Float128 = float __attribute__((ext_vector_type(4)));
using Int128 = int __attribute__((ext_vector_type(4)));
using Uint128 = unsigned int __attribute__((ext_vector_type(4)));

using Float256 = float __attribute__((ext_vector_type(8)));
using Int256 = int __attribute__((ext_vector_type(8)));
using Uint256 = unsigned int __attribute__((ext_vector_type(8)));
#else
using Float128 = float __attribute__((vector_size(16)));
using Int128 = int __attribute__((vector_size(16)));
using Uint128 = unsigned int __attribute__((vector_size(16)));

using Float256 = float __attribute__((vector_size(32)));
using Int256 = int __attribute__((vector_size(32)));
using Uint256 = unsigned int __attribute__((vector_size(32)));
#endif

template <class T, size_t N>
struct alignas(16) Array128 : public std::array<T, N> {};
template <class T, size_t N>
struct alignas(32) Array256 : public std::array<T, N> {};
template <class T, size_t N>
struct alignas(alignof(T)) Array : public std::array<T, N> {};

template <class T>
concept IsSimdFloat = std::is_same_v<T, Float128> || std::is_same_v<T, Float256>;

template <class T>
static constexpr size_t LaneSize = sizeof(T) / sizeof(float);

// clang-format off
static constexpr struct ISSE2 {} iSSE2;
static constexpr struct ISSE4 {} iSSE4;
static constexpr struct IAVX {} iAVX;
static constexpr struct IAVX2 {} iAVX2;
static constexpr struct INEON {} iNEON;
// clang-format on

// ----------------------------------------
// extension?
// ----------------------------------------

template <class T, int... Indices>
static inline T Shuffle(T a, T b) noexcept {
#if defined(__clang__)
    return __builtin_shufflevector(a, b, Indices...);
#else
    constexpr int count = sizeof...(Indices);
    using MaskType = typename std::conditional<count == 4, Int128, Int256>::type;
    return __builtin_shuffle(a, b, MaskType{Indices...});
#endif
}

static inline simde__m128 ToSimde(Float128 x) noexcept {
    return std::bit_cast<simde__m128>(x);
}
static inline simde__m256 ToSimde(Float256 x) noexcept {
    return std::bit_cast<simde__m256>(x);
}
static inline simde__m128i ToSimde(Int128 x) noexcept {
    return std::bit_cast<simde__m128i>(x);
}
static inline simde__m256i ToSimde(Int256 x) noexcept {
    return std::bit_cast<simde__m256i>(x);
}

static inline Float128 FromSimde(simde__m128 x) noexcept {
    return std::bit_cast<Float128>(x);
}
static inline Float256 FromSimde(simde__m256 x) noexcept {
    return std::bit_cast<Float256>(x);
}
static inline Int128 FromSimde(simde__m128i x) noexcept {
    return std::bit_cast<Int128>(x);
}
static inline Int256 FromSimde(simde__m256i x) noexcept {
    return std::bit_cast<Int256>(x);
}

static inline Int128 ToInt(Float128 x) noexcept {
    return __builtin_convertvector(x, Int128);
}
static inline Int256 ToInt(Float256 x) noexcept {
    return __builtin_convertvector(x, Int256);
}

static inline Float128 ToFloat(Int128 x) noexcept {
    return __builtin_convertvector(x, Float128);
}
static inline Float256 ToFloat(Int256 x) noexcept {
    return __builtin_convertvector(x, Float256);
}

static inline Float128 Frac(Float128 x_) noexcept {
#if defined(SIMDE_X86_SSE4_1_NATIVE) || defined(SIMDE_X86_AVX_NATIVE) || defined(SIMDE_ARM_NEON_A64V8_NATIVE)
    auto x = ToSimde(x_);
    return FromSimde(simde_mm_sub_ps(x, simde_mm_floor_ps(x)));
#elif defined(SIMDE_X86_SSE2_NATIVE) || defined(SIMDE_ARM_NEON_NATIVE)
    return x_ - ToFloat(ToInt(x_));
#else
    return Float128{x_[0] - floorf(x_[0]), x_[1] - floorf(x_[1]), x_[2] - floorf(x_[2]), x_[3] - floorf(x_[3])};
#endif
}
static inline Float256 Frac(Float256 x) noexcept {
    auto x_ = ToSimde(x);
    simde__m256 i = simde_mm256_floor_ps(x_);
    auto s = simde_mm256_sub_ps(x_, i);
    return FromSimde(s);
}

static inline Float128 Loadu128(const float* ptr) noexcept {
    return Float128{ptr[0], ptr[1], ptr[2], ptr[3]};
}
static inline Float256 Loadu256(const float* ptr) noexcept {
    return Float256{ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7]};
}

static inline Float128 Max(Float128 a, Float128 b) noexcept {
    return a > b ? a : b;
}
static inline Float256 Max(Float256 a, Float256 b) noexcept {
    return a > b ? a : b;
}

static inline Float128 Min(Float128 a, Float128 b) noexcept {
    return a < b ? a : b;
}
static inline Float256 Min(Float256 a, Float256 b) noexcept {
    return a < b ? a : b;
}

static inline Float128 BroadcastF128(float i) noexcept {
    return Float128{i, i, i, i};
}
static inline Float256 BroadcastF256(float i) noexcept {
    return Float256{i, i, i, i, i, i, i, i};
}

static inline Float256 Combine(Float128 lo, Float128 hi) noexcept {
    return Float256{lo[0], lo[1], lo[2], lo[3], hi[0], hi[1], hi[2], hi[3]};
}
static inline std::array<Float128, 2> Break(Float256 x) noexcept {
    return {
        Float128{x[0], x[1], x[2], x[3]},
        Float128{x[4], x[5], x[6], x[7]}
    };
}

static inline std::array<Float128, 4> Transpose(Float128 x0, Float128 x1, Float128 x2, Float128 x3) noexcept {
    Float128 tmp0 = Shuffle<Float128, 0, 4, 1, 5>(x0, x1); // row0[0], row1[0], row0[1], row1[1]
    Float128 tmp1 = Shuffle<Float128, 2, 6, 3, 7>(x0, x1); // row0[2], row1[2], row0[3], row1[3]
    Float128 tmp2 = Shuffle<Float128, 0, 4, 1, 5>(x2, x3); // row2[0], row3[0], row2[1], row3[1]
    Float128 tmp3 = Shuffle<Float128, 2, 6, 3, 7>(x2, x3); // row2[2], row3[2], row2[3], row3[3]

    Float128 row0 = Shuffle<Float128, 0, 1, 4, 5>(tmp0, tmp2); // [r0c0, r1c0, r2c0, r3c0]
    Float128 row1 = Shuffle<Float128, 2, 3, 6, 7>(tmp0, tmp2); // [r0c1, r1c1, r2c1, r3c1]
    Float128 row2 = Shuffle<Float128, 0, 1, 4, 5>(tmp1, tmp3); // [r0c2, r1c1, r2c2, r3c2]
    Float128 row3 = Shuffle<Float128, 2, 3, 6, 7>(tmp1, tmp3); // [r0c3, r1c3, r2c3, r3c3]

    return {row0, row1, row2, row3};
}
static inline std::array<Float256, 4> Transpose256(Float128 a, Float128 b, Float128 c, Float128 d, Float128 e,
                                                   Float128 f, Float128 g, Float128 h) noexcept {
    Float256 x0 = Combine(a, e);
    Float256 x1 = Combine(b, f);
    Float256 x2 = Combine(c, g);
    Float256 x3 = Combine(d, h);

    Float256 m0 = Shuffle<Float256, 0, 8, 1, 9, 4, 12, 5, 13>(x0, x1);
    Float256 m1 = Shuffle<Float256, 2, 10, 3, 11, 6, 14, 7, 15>(x0, x1);
    Float256 m2 = Shuffle<Float256, 0, 8, 1, 9, 4, 12, 5, 13>(x2, x3);
    Float256 m3 = Shuffle<Float256, 2, 10, 3, 11, 6, 14, 7, 15>(x2, x3);

    Float256 out0 = Shuffle<Float256, 0, 1, 8, 9, 4, 5, 12, 13>(m0, m2);
    Float256 out1 = Shuffle<Float256, 2, 3, 10, 11, 6, 7, 14, 15>(m0, m2);
    Float256 out2 = Shuffle<Float256, 0, 1, 8, 9, 4, 5, 12, 13>(m1, m3);
    Float256 out3 = Shuffle<Float256, 2, 3, 10, 11, 6, 7, 14, 15>(m1, m3);

    return {out0, out1, out2, out3};
}

static inline float ReduceAdd(Float128 x) noexcept {
    return x[0] + x[1] + x[2] + x[3];
}
static inline float ReduceAdd(Float256 x) noexcept {
    return x[0] + x[1] + x[2] + x[3] + x[4] + x[5] + x[6] + x[7];
}

static inline Float128 Tan(Float128 x) noexcept {
    return Float128{std::tan(x[0]), std::tan(x[1]), std::tan(x[2]), std::tan(x[3])};
}
static inline Float256 Tan(Float256 x) noexcept {
    return Float256{std::tan(x[0]), std::tan(x[1]), std::tan(x[2]), std::tan(x[3]),
                    std::tan(x[4]), std::tan(x[5]), std::tan(x[6]), std::tan(x[7])};
}

// ----------------------------------------
// complex
// ----------------------------------------

template <class T>
struct SimdComplex {
    T re;
    T im;

    // -------------------- 复数与复数 --------------------
    inline SimdComplex operator+(const SimdComplex& o) const noexcept {
        return {re + o.re, im + o.im};
    }
    inline SimdComplex operator-(const SimdComplex& o) const noexcept {
        return {re - o.re, im - o.im};
    }
    inline SimdComplex operator*(const SimdComplex& o) const noexcept {
        return {re * o.re - im * o.im, re * o.im + im * o.re};
    }
    inline SimdComplex operator/(const SimdComplex& o) const noexcept {
        T den = o.re * o.re + o.im * o.im;
        return {(re * o.re + im * o.im) / den, (im * o.re - re * o.im) / den};
    }

    // -------------------- 复合赋值 --------------------
    inline SimdComplex& operator+=(const SimdComplex& o) noexcept {
        re += o.re;
        im += o.im;
        return *this;
    }
    inline SimdComplex& operator-=(const SimdComplex& o) noexcept {
        re -= o.re;
        im -= o.im;
        return *this;
    }
    inline SimdComplex& operator*=(const SimdComplex& o) noexcept {
        T new_re = re * o.re - im * o.im;
        im = re * o.im + im * o.re;
        re = new_re;
        return *this;
    }
    inline SimdComplex& operator/=(const SimdComplex& o) noexcept {
        T den = o.re * o.re + o.im * o.im;
        T new_re = (re * o.re + im * o.im) / den;
        im = (im * o.re - re * o.im) / den;
        re = new_re;
        return *this;
    }

    // -------------------- 复数与实数 --------------------
    inline SimdComplex operator+(T s) const noexcept {
        return {re + s, im};
    }
    inline SimdComplex operator-(T s) const noexcept {
        return {re - s, im};
    }
    inline SimdComplex operator*(T s) const noexcept {
        return {re * s, im * s};
    }
    inline SimdComplex operator/(T s) const noexcept {
        return {re / s, im / s};
    }

    // -------------------- 复数与实数赋值 --------------------
    inline SimdComplex& operator+=(T s) noexcept {
        re += s;
        return *this;
    }
    inline SimdComplex& operator-=(T s) noexcept {
        re -= s;
        return *this;
    }
    inline SimdComplex& operator*=(T s) noexcept {
        re *= s;
        im *= s;
        return *this;
    }
    inline SimdComplex& operator/=(T s) noexcept {
        re /= s;
        im /= s;
        return *this;
    }

    // -------------------- 实数与复数 --------------------
    friend inline SimdComplex operator+(T s, const SimdComplex& v) noexcept {
        return {s + v.re, v.im};
    }
    friend inline SimdComplex operator-(T s, const SimdComplex& v) noexcept {
        return {s - v.re, -v.im};
    }
    friend inline SimdComplex operator*(T s, const SimdComplex& v) noexcept {
        return {s * v.re, s * v.im};
    }
    friend inline SimdComplex operator/(T s, const SimdComplex& v) noexcept {
        T den = v.re * v.re + v.im * v.im;
        return {(s * v.re) / den, (-s * v.im) / den};
    }

    // -------------------- std复数与复数 --------------------
    friend inline SimdComplex operator+(std::complex<float> s, const SimdComplex& v) noexcept {
        return {s.real() + v.re, s.imag() + v.im};
    }
    friend inline SimdComplex operator-(std::complex<float> s, const SimdComplex& v) noexcept {
        return {s.real() - v.re, s.imag() - v.im};
    }
    friend inline SimdComplex operator*(std::complex<float> s, const SimdComplex& v) noexcept {
        float sr = s.real();
        float si = s.imag();
        return {sr * v.re - si * v.im, sr * v.im + si * v.re};
    }
    friend inline SimdComplex operator/(std::complex<float> s, const SimdComplex& v) noexcept {
        float sr = s.real();
        float si = s.imag();
        T den = v.re * v.re + v.im * v.im;
        return {(sr * v.re + si * v.im) / den, (si * v.re - sr * v.im) / den};
    }

    // -------------------- 复数与std复数 --------------------
    friend inline SimdComplex operator+(const SimdComplex& v, std::complex<float> s) noexcept {
        return {v.re + s.real(), v.im + s.imag()};
    }
    friend inline SimdComplex operator-(const SimdComplex& v, std::complex<float> s) noexcept {
        return {v.re - s.real(), v.im - s.imag()};
    }
    friend inline SimdComplex operator*(const SimdComplex& v, std::complex<float> s) noexcept {
        float sr = s.real();
        float si = s.imag();
        return {v.re * sr - v.im * si, v.re * si + v.im * sr};
    }
    friend inline SimdComplex operator/(const SimdComplex& v, std::complex<float> s) noexcept {
        float sr = s.real();
        float si = s.imag();
        T den = sr * sr + si * si;
        return {(v.re * sr + v.im * si) / den, (v.im * sr - v.re * si) / den};
    }

    // -------------------- 辅助 --------------------
    inline SimdComplex conj() const noexcept {
        return {re, -im};
    }

    inline std::complex<float> ReduceAdd() const noexcept {
        return {simd::ReduceAdd(re), simd::ReduceAdd(im)};
    }
};
using Complex128 = SimdComplex<Float128>;
using Complex256 = SimdComplex<Float256>;

} // namespace simd
