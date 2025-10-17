#pragma once
#include "x86/sse4.1.h"

namespace qwqdsp::psimd {
struct Int32x4 {
    static constexpr size_t kSize = 4;
    simde__m128i reg;

    struct alignas(16) Array {
        int32_t x[4];
        int32_t& operator[](size_t i) noexcept {
            return x[i];
        }
        int32_t operator[](size_t i) const noexcept {
            return x[i];
        }
    };

    Int32x4() noexcept = default;

    explicit Int32x4(simde__m128i v) noexcept {
        reg = v;
    }

    explicit Int32x4(int32_t v) noexcept {
        reg = simde_mm_set1_epi32(v);
    }

    static Int32x4 FromSingle(int32_t v) noexcept {
        return Int32x4{v};
    }

    explicit Int32x4(int32_t a, int32_t b, int32_t c, int32_t d) noexcept {
        reg = simde_mm_set_epi32(d, c, b, a);
    }

    void Load(int32_t const* v) noexcept {
        reg = simde_mm_loadu_si128(v);
    }

    Array ToArray() const noexcept {
        Array r;
        simde_mm_storeu_si128(r.x, reg);
        return r;
    }

    Int32x4& operator+=(Int32x4 const& a) noexcept {
        reg = simde_mm_add_epi32(reg, a.reg);
        return *this;
    }

    Int32x4& operator-=(Int32x4 const& a) noexcept {
        reg = simde_mm_sub_epi32(reg, a.reg);
        return *this;
    }

    Int32x4& operator*=(Int32x4 const& a) noexcept {
        reg = simde_mm_mullo_epi32(reg, a.reg);
        return *this;
    }

    Int32x4& operator&=(Int32x4 const& a) noexcept {
        reg = simde_mm_and_si128(reg, a.reg);
        return *this;
    }
};

[[maybe_unused]]
static Int32x4 operator+(const Int32x4& a, const Int32x4& b) noexcept {
    return Int32x4{simde_mm_add_epi32(a.reg, b.reg)};
}
[[maybe_unused]]
static Int32x4 operator-(const Int32x4& a, const Int32x4& b) noexcept {
    return Int32x4{simde_mm_sub_epi32(a.reg, b.reg)};
}
[[maybe_unused]]
static Int32x4 operator*(const Int32x4& a, const Int32x4& b) noexcept {
    return Int32x4{simde_mm_mullo_epi32(a.reg, b.reg)};
}
[[maybe_unused]]
static Int32x4 operator&(const Int32x4& a, const Int32x4& b) noexcept {
    return Int32x4{simde_mm_and_si128(a.reg, b.reg)};
}

struct Float32x4 {
    static constexpr size_t kSize = 4;
    using IntType = Int32x4;

    union {
        simde__m128 reg;
        float x[4];
    };

    struct alignas(16) Array {
        float reg[4];
        float& operator[](size_t i) noexcept {
            return reg[i];
        }
        float operator[](size_t i) const noexcept {
            return reg[i];
        }
    };

    Float32x4() noexcept = default;

    explicit Float32x4(simde__m128 v) noexcept {
        reg = v;
    }

    explicit Float32x4(float v) noexcept {
        reg = simde_mm_set1_ps(v);
    }

    explicit Float32x4(float a, float b, float c, float d) noexcept {
        reg = simde_mm_set_ps(d, c, b, a);
    }

    static Float32x4 FromSingle(float v) noexcept {
        return Float32x4{v};
    }

    void Load(float const* v) noexcept {
        reg = simde_mm_load_ps(v);
    }

    void Store(float* v) noexcept {
        simde_mm_store_ps(v, reg);
    }

    void LoadU(float const* v) noexcept {
        reg = simde_mm_loadu_ps(v);
    }

    void StoreU(float* v) noexcept {
        simde_mm_storeu_ps(v, reg);
    }

    Array ToArray() const noexcept {
        Array r;
        simde_mm_store_ps(r.reg, reg);
        return r;
    }

    static Float32x4 Min(Float32x4 const& a, Float32x4 const& b) noexcept {
        return Float32x4{simde_mm_min_ps(a.reg, b.reg)};
    }

    static Float32x4 Max(Float32x4 const& a, Float32x4 const& b) noexcept {
        return Float32x4{simde_mm_max_ps(a.reg, b.reg)};
    }

    /**
     * @return reg[a],reg[b],reg[c],reg[d]
     */
    template<int a, int b, int c, int d>
    Float32x4 Shuffle() noexcept {
        constexpr int mask = (a << 6) | (b << 4) | (c << 2) | d;
        return Float32x4{simde_mm_shuffle_ps(reg, reg, mask)};
    }

    /**
     * @brief tparam: 0 => self, 1 => other
     */
    template<int a, int b, int c, int d>
    Float32x4 Blend(const Float32x4& other) noexcept {
        constexpr int mask = (a << 0) | (b << 1) | (c << 2) | (d << 3);
        return Float32x4{simde_mm_blend_ps(reg, other.reg, mask)};
    }

    Float32x4& operator+=(const Float32x4& v) noexcept {
        reg = simde_mm_add_ps(reg, v.reg);
        return *this;
    }

    Float32x4& operator-=(const Float32x4& v) noexcept {
        reg = simde_mm_sub_ps(reg, v.reg);
        return *this;
    }

    Float32x4& operator*=(const Float32x4& v) noexcept {
        reg = simde_mm_mul_ps(reg, v.reg);
        return *this;
    }

    Float32x4& operator/=(const Float32x4& v) noexcept {
        reg = simde_mm_div_ps(reg, v.reg);
        return *this;
    }

    Float32x4 Frac() const noexcept {
        auto vfloor = simde_mm_floor_ps(reg);
        return Float32x4{simde_mm_sub_ps(reg, vfloor)};
    }

    Int32x4 ToInt() const noexcept {
        return Int32x4{simde_mm_cvttps_epi32(reg)};
    }

    float ReduceAdd() const noexcept {
        auto const a = ToArray();
        return a[0] + a[1] + a[2] + a[3];
    }

    static Float32x4 FMA(const Float32x4& mula, const Float32x4& mulb, const Float32x4& add) noexcept {
        return Float32x4{simde_mm_add_ps(simde_mm_mul_ps(mula.reg, mulb.reg), add.reg)};
    }

    static void Transpose(Float32x4& row0, Float32x4& row1, Float32x4& row2, Float32x4& row3) noexcept {
        simde__m128 tmp0, tmp1, tmp2, tmp3;
        tmp0 = simde_mm_unpacklo_ps(row0.reg, row1.reg);
        tmp2 = simde_mm_unpacklo_ps(row2.reg, row3.reg);
        tmp1 = simde_mm_unpackhi_ps(row0.reg, row1.reg);
        tmp3 = simde_mm_unpackhi_ps(row2.reg, row3.reg);
        row0.reg = simde_mm_movelh_ps(tmp0, tmp2);
        row1.reg = simde_mm_movehl_ps(tmp2, tmp0);
        row2.reg = simde_mm_movelh_ps(tmp1, tmp3);
        row3.reg = simde_mm_movehl_ps(tmp3, tmp1);
    }
};

[[maybe_unused]]
static Float32x4 operator+(const Float32x4& a, const Float32x4& b) noexcept {
    return Float32x4{simde_mm_add_ps(a.reg, b.reg)};
}
[[maybe_unused]]
static Float32x4 operator-(const Float32x4& a, const Float32x4& b) noexcept {
    return Float32x4{simde_mm_sub_ps(a.reg, b.reg)};
}
[[maybe_unused]]
static Float32x4 operator*(const Float32x4& a, const Float32x4& b) noexcept {
    return Float32x4{simde_mm_mul_ps(a.reg, b.reg)};
}
[[maybe_unused]]
static Float32x4 operator/(const Float32x4& a, const Float32x4& b) noexcept {
    return Float32x4{simde_mm_div_ps(a.reg, b.reg)};
}
}