#pragma once
#include "x86/avx2.h"
#include "x86/fma.h"

namespace qwqdsp::psimd {
struct Int32x8 {
    static constexpr size_t kSize = 8;
    simde__m256i reg;

    struct alignas(32) Array {
        int32_t x[8];
        int32_t& operator[](size_t i) noexcept {
            return x[i];
        }
        int32_t operator[](size_t i) const noexcept {
            return x[i];
        }
    };

    Int32x8() noexcept = default;

    explicit Int32x8(simde__m256i v) noexcept {
        reg = v;
    }

    explicit Int32x8(int32_t v) noexcept {
        reg = simde_mm256_set1_epi32(v);
    }

    explicit Int32x8(int32_t a, int32_t b, int32_t c, int32_t d, int32_t e, int32_t f, int32_t g, int32_t h) noexcept {
        reg = simde_mm256_set_epi32(h, g, f, e, d, c, b, a);
    }

    static Int32x8 FromSingle(int32_t v) noexcept {
        return Int32x8{v};
    }

    void Load(int32_t const* v) noexcept {
        reg = simde_mm256_loadu_si256(v);
    }

    Array ToArray() const noexcept {
        Array r;
        simde_mm256_storeu_si256(r.x, reg);
        return r;
    }

    Int32x8& operator+=(Int32x8 const& a) noexcept {
        reg = simde_mm256_add_epi32(reg, a.reg);
        return *this;
    }

    Int32x8& operator-=(Int32x8 const& a) noexcept {
        reg = simde_mm256_sub_epi32(reg, a.reg);
        return *this;
    }

    Int32x8& operator*=(Int32x8 const& a) noexcept {
        reg = simde_mm256_mullo_epi32(reg, a.reg);
        return *this;
    }

    Int32x8& operator&=(Int32x8 const& a) noexcept {
        reg = simde_mm256_and_si256(reg, a.reg);
        return *this;
    }
};

[[maybe_unused]]
static Int32x8 operator+(const Int32x8& a, const Int32x8& b) noexcept {
    return Int32x8{simde_mm256_add_epi32(a.reg, b.reg)};
}
[[maybe_unused]]
static Int32x8 operator-(const Int32x8& a, const Int32x8& b) noexcept {
    return Int32x8{simde_mm256_sub_epi32(a.reg, b.reg)};
}
[[maybe_unused]]
static Int32x8 operator*(const Int32x8& a, const Int32x8& b) noexcept {
    return Int32x8{simde_mm256_mullo_epi32(a.reg, b.reg)};
}
[[maybe_unused]]
static Int32x8 operator&(const Int32x8& a, const Int32x8& b) noexcept {
    return Int32x8{simde_mm256_and_si256(a.reg, b.reg)};
}

struct Float32x8 {
    static constexpr size_t kSize = 8;
    using IntType = Int32x8;

    union {
        simde__m256 reg;
        float x[16];
    };

    struct alignas(32) Array {
        float x[8];
        float& operator[](size_t i) noexcept {
            return x[i];
        }
        float operator[](size_t i) const noexcept {
            return x[i];
        }
    };

    Float32x8() noexcept = default;

    explicit Float32x8(simde__m256 v) noexcept {
        reg = v;
    }

    explicit Float32x8(float v) noexcept {
        reg = simde_mm256_set1_ps(v);
    }

    explicit Float32x8(float a, float b, float c, float d, float e, float f, float g, float h) noexcept {
        reg = simde_mm256_set_ps(h, g, f, e, d, c, b, a);
    }

    static Float32x8 FromSingle(float v) noexcept {
        return Float32x8{v};
    }

    void Load(float const* v) noexcept {
        reg = simde_mm256_load_ps(v);
    }

    void LoadU(float const* v) noexcept {
        reg = simde_mm256_loadu_ps(v);
    }

    Array ToArray() const noexcept {
        Array r;
        simde_mm256_store_ps(r.x, reg);
        return r;
    }

    static Float32x8 Min(Float32x8 const& a, Float32x8 const& b) noexcept {
        return Float32x8{simde_mm256_min_ps(a.reg, b.reg)};
    }

    static Float32x8 Max(Float32x8 const& a, Float32x8 const& b) noexcept {
        return Float32x8{simde_mm256_max_ps(a.reg, b.reg)};
    }

    /**
     * @return reg[a],reg[b],reg[c],reg[d], reg[e], reg[f], reg[g], reg[h]
     */
    template<int a, int b, int c, int d, int e, int f, int g, int h>
    Float32x8 Shuffle() noexcept {
       return Float32x8(simde_mm256_permutevar8x32_ps(reg, simde_mm256_set_epi32(h, g, f, e, d, c, b, a)));
    }

    /**
     * @brief tparam: 0 => self, 1 => other
     */
    template<int a, int b, int c, int d, int e, int f, int g, int h>
    Float32x8 Blend(const Float32x8& other) noexcept {
        constexpr int mask = (a << 0) | (b << 1) | (c << 2) | (d << 3) | (e << 4) | (f << 5) | (g << 6) | (h << 7);
        return Float32x8{simde_mm256_blend_ps(reg, other.reg, mask)};
    }

    Float32x8& operator+=(const Float32x8& v) noexcept {
        reg = simde_mm256_add_ps(reg, v.reg);
        return *this;
    }

    Float32x8& operator-=(const Float32x8& v) noexcept {
        reg = simde_mm256_sub_ps(reg, v.reg);
        return *this;
    }

    Float32x8& operator*=(const Float32x8& v) noexcept {
        reg = simde_mm256_mul_ps(reg, v.reg);
        return *this;
    }

    Float32x8& operator/=(const Float32x8& v) noexcept {
        reg = simde_mm256_div_ps(reg, v.reg);
        return *this;
    }

     Float32x8 Frac() const noexcept {
        auto vfloor = simde_mm256_floor_ps(reg);
        return Float32x8{simde_mm256_sub_ps(reg, vfloor)};
    }

    Int32x8 ToInt() const noexcept {
        return Int32x8{simde_mm256_cvttps_epi32(reg)};
    }

    float ReduceAdd() const noexcept {
        auto const a = ToArray();
        return a[0] + a[1] + a[2] + a[3] + a[4] + a[5] + a[6] + a[7];
    }

    static Float32x8 FMA(const Float32x8& mula, const Float32x8& mulb, const Float32x8& add) noexcept {
        return Float32x8{simde_mm256_fmadd_ps(mula.reg, mulb.reg, add.reg)};
    }
};

[[maybe_unused]]
static Float32x8 operator+(const Float32x8& a, const Float32x8& b) noexcept {
    return Float32x8{simde_mm256_add_ps(a.reg, b.reg)};
}
[[maybe_unused]]
static Float32x8 operator-(const Float32x8& a, const Float32x8& b) noexcept {
    return Float32x8{simde_mm256_sub_ps(a.reg, b.reg)};
}
[[maybe_unused]]
static Float32x8 operator*(const Float32x8& a, const Float32x8& b) noexcept {
    return Float32x8{simde_mm256_mul_ps(a.reg, b.reg)};
}
[[maybe_unused]]
static Float32x8 operator/(const Float32x8& a, const Float32x8& b) noexcept {
    return Float32x8{simde_mm256_div_ps(a.reg, b.reg)};
}
}