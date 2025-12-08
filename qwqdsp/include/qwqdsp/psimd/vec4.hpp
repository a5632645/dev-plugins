#pragma once
#if _MSC_VER
    #if defined(__AVX__) || defined(__AVX2__)
        #define QWQDSP_HAS_SSE41 1
    #elif defined(__SSE4_1__)
        #define QWQDSP_HAS_SSE41 1
    #else
        #define QWQDSP_HAS_SSE41 0
#endif
#else
    #ifdef __SSE4_1__
        #define QWQDSP_HAS_SSE41 1
    #else
        #define QWQDSP_HAS_SSE41 0
    #endif
#endif

#if QWQDSP_HAS_SSE41
#include <cmath>
#endif

namespace qwqdsp_psimd {
// ---------------------------------------- 4int ----------------------------------------
struct alignas(16) Vec4i32 {
    static constexpr size_t kSize = 4;

    int x[4];

    static constexpr Vec4i32 FromSingle(int v) noexcept {
        Vec4i32 r;
        r.x[0] = v;
        r.x[1] = v;
        r.x[2] = v;
        r.x[3] = v;
        return r;
    }

    constexpr Vec4i32& operator+=(const Vec4i32& v) noexcept {
        x[0] += v.x[0];
        x[1] += v.x[1];
        x[2] += v.x[2];
        x[3] += v.x[3];
        return *this;
    }

    constexpr  Vec4i32& operator-=(const Vec4i32& v) noexcept {
        x[0] -= v.x[0];
        x[1] -= v.x[1];
        x[2] -= v.x[2];
        x[3] -= v.x[3];
        return *this;
    }

    constexpr  Vec4i32& operator*=(const Vec4i32& v) noexcept {
        x[0] *= v.x[0];
        x[1] *= v.x[1];
        x[2] *= v.x[2];
        x[3] *= v.x[3];
        return *this;
    }

    constexpr Vec4i32& operator/=(const Vec4i32& v) noexcept {
        x[0] /= v.x[0];
        x[1] /= v.x[1];
        x[2] /= v.x[2];
        x[3] /= v.x[3];
        return *this;
    }

    constexpr Vec4i32& operator&=(const Vec4i32& v) noexcept {
        x[0] &= v.x[0];
        x[1] &= v.x[1];
        x[2] &= v.x[2];
        x[3] &= v.x[3];
        return *this;
    }

    constexpr Vec4i32& FMA(const Vec4i32& mul, const Vec4i32& add) noexcept {
        x[0] = x[0] * mul.x[0] + add.x[0];
        x[1] = x[1] * mul.x[1] + add.x[1];
        x[2] = x[2] * mul.x[2] + add.x[2];
        x[3] = x[3] * mul.x[3] + add.x[3];
        return *this;
    }
};

static constexpr Vec4i32 operator+(const Vec4i32& a, const Vec4i32& b) noexcept {
    Vec4i32 r = a;
    r += b;
    return r;
}
static constexpr Vec4i32 operator-(const Vec4i32& a, const Vec4i32& b) noexcept {
    Vec4i32 r = a;
    r -= b;
    return r;
}
static constexpr Vec4i32 operator*(const Vec4i32& a, const Vec4i32& b) noexcept {
    Vec4i32 r = a;
    r *= b;
    return r;
}
static constexpr Vec4i32 operator/(const Vec4i32& a, const Vec4i32& b) noexcept {
    Vec4i32 r = a;
    r /= b;
    return r;
}
static constexpr Vec4i32 operator&(const Vec4i32& a, const Vec4i32& b) noexcept {
    Vec4i32 r = a;
    r &= b;
    return r;
}

// ---------------------------------------- 4float ----------------------------------------

struct alignas(16) Vec4f32 {
    static constexpr size_t kSize = 4;
    using IntType = Vec4i32;

    float x[4];

    static constexpr Vec4f32 FromSingle(float v) noexcept {
        return Vec4f32{
            v,
            v,
            v,
            v
        };
    }

    static constexpr Vec4f32 Min(Vec4f32 const& a, Vec4f32 const& b) noexcept {
        return {
            a.x[0] < b.x[0] ? a.x[0] : b.x[0],
            a.x[1] < b.x[1] ? a.x[1] : b.x[1],
            a.x[2] < b.x[2] ? a.x[2] : b.x[2],
            a.x[3] < b.x[3] ? a.x[3] : b.x[3]
        };
    }

    static constexpr Vec4f32 Max(Vec4f32 const& a, Vec4f32 const& b) noexcept {
        return {
            a.x[0] > b.x[0] ? a.x[0] : b.x[0],
            a.x[1] > b.x[1] ? a.x[1] : b.x[1],
            a.x[2] > b.x[2] ? a.x[2] : b.x[2],
            a.x[3] > b.x[3] ? a.x[3] : b.x[3]
        };
    }

    static constexpr Vec4f32 Cross(float left, float right) noexcept {
        return Vec4f32{
            left,
            right,
            left,
            right
        };
    }

    [[nodiscard]]
    constexpr Vec4f32 Decross() noexcept {
        return Vec4f32{
            x[0] + x[2],
            x[1] + x[3]
        };
    }

    /**
     * @return x[a],x[b],x[c],x[d]
     */
    template<int a, int b, int c, int d>
    [[nodiscard]]
    constexpr Vec4f32 Shuffle() noexcept {
        return Vec4f32{
            x[a],
            x[b],
            x[c],
            x[d]
        };
    }

    /**
     * @brief tparam: 0 => self, 1 => other
     */
    template<int a, int b, int c, int d>
    [[nodiscard]]
    constexpr Vec4f32 Blend(const Vec4f32& other) noexcept {
        return Vec4f32{
            a == 0 ? x[0] : other.x[0],
            b == 0 ? x[1] : other.x[1],
            c == 0 ? x[2] : other.x[2],
            d == 0 ? x[3] : other.x[3],
        };
    }

    constexpr Vec4f32& operator+=(const Vec4f32& v) noexcept {
        x[0] += v.x[0];
        x[1] += v.x[1];
        x[2] += v.x[2];
        x[3] += v.x[3];
        return *this;
    }

    constexpr  Vec4f32& operator-=(const Vec4f32& v) noexcept {
        x[0] -= v.x[0];
        x[1] -= v.x[1];
        x[2] -= v.x[2];
        x[3] -= v.x[3];
        return *this;
    }

    constexpr  Vec4f32& operator*=(const Vec4f32& v) noexcept {
        x[0] *= v.x[0];
        x[1] *= v.x[1];
        x[2] *= v.x[2];
        x[3] *= v.x[3];
        return *this;
    }

    constexpr Vec4f32& operator/=(const Vec4f32& v) noexcept {
        x[0] /= v.x[0];
        x[1] /= v.x[1];
        x[2] /= v.x[2];
        x[3] /= v.x[3];
        return *this;
    }

    constexpr Vec4f32& operator+=(float v) noexcept {
        x[0] += v;
        x[1] += v;
        x[2] += v;
        x[3] += v;
        return *this;
    }

    constexpr  Vec4f32& operator-=(float v) noexcept {
        x[0] -= v;
        x[1] -= v;
        x[2] -= v;
        x[3] -= v;
        return *this;
    }

    constexpr  Vec4f32& operator*=(float v) noexcept {
        x[0] *= v;
        x[1] *= v;
        x[2] *= v;
        x[3] *= v;
        return *this;
    }

    constexpr Vec4f32& operator/=(float v) noexcept {
        x[0] /= v;
        x[1] /= v;
        x[2] /= v;
        x[3] /= v;
        return *this;
    }

    constexpr Vec4f32 Frac() const noexcept {
        #if QWQDSP_HAS_SSE41
        return Vec4f32{
            x[0] - std::floor(x[0]),
            x[1] - std::floor(x[1]),
            x[2] - std::floor(x[2]),
            x[3] - std::floor(x[3])
        };
        #else
        Vec4f32 r = PositiveFrac();
        Vec4f32 fix{
            x[0] < 0 ? 1.0f : 0.0f,
            x[1] < 0 ? 1.0f : 0.0f,
            x[2] < 0 ? 1.0f : 0.0f,
            x[3] < 0 ? 1.0f : 0.0f
        };
        r += fix;
        return r;
        #endif
    }

    constexpr Vec4f32 PositiveFrac() const noexcept {
        #if QWQDSP_HAS_SSE41
        return Vec4f32{
            x[0] - std::floor(x[0]),
            x[1] - std::floor(x[1]),
            x[2] - std::floor(x[2]),
            x[3] - std::floor(x[3])
        };
        #else
        return Vec4f32{
            x[0] - static_cast<int>(x[0]),
            x[1] - static_cast<int>(x[1]),
            x[2] - static_cast<int>(x[2]),
            x[3] - static_cast<int>(x[3])
        };
        #endif
    }

    constexpr Vec4i32 ToInt() const noexcept {
        return Vec4i32{
            static_cast<int>(x[0]),
            static_cast<int>(x[1]),
            static_cast<int>(x[2]),
            static_cast<int>(x[3])
        };
    }

    constexpr float ReduceAdd() const noexcept {
        return x[0] + x[1] + x[2] + x[3];
    }

    static Vec4f32 Sqrt(Vec4f32 const& x) noexcept {
        return Vec4f32{
            std::sqrt(x.x[0]),
            std::sqrt(x.x[1]),
            std::sqrt(x.x[2]),
            std::sqrt(x.x[3]),
        };
    }

    static Vec4f32 Abs(Vec4f32 const& x) noexcept {
        return Vec4f32{
            std::abs(x.x[0]),
            std::abs(x.x[1]),
            std::abs(x.x[2]),
            std::abs(x.x[3]),
        };
    }

    static constexpr Vec4f32 FMA(const Vec4f32& mula, const Vec4f32& mulb, const Vec4f32& add) noexcept {
        return Vec4f32{
            mula.x[0] * mulb.x[0] + add.x[0],
            mula.x[1] * mulb.x[1] + add.x[1],
            mula.x[2] * mulb.x[2] + add.x[2],
            mula.x[3] * mulb.x[3] + add.x[3]
        };
    }
};

static constexpr Vec4f32 operator+(const Vec4f32& a, const Vec4f32& b) noexcept {
    Vec4f32 r = a;
    r += b;
    return r;
}
static constexpr Vec4f32 operator-(const Vec4f32& a, const Vec4f32& b) noexcept {
    Vec4f32 r = a;
    r -= b;
    return r;
}
static constexpr Vec4f32 operator*(const Vec4f32& a, const Vec4f32& b) noexcept {
    Vec4f32 r = a;
    r *= b;
    return r;
}
static constexpr Vec4f32 operator/(const Vec4f32& a, const Vec4f32& b) noexcept {
    Vec4f32 r = a;
    r /= b;
    return r;
}

static constexpr Vec4f32 operator+(const Vec4f32& a, float b) noexcept {
    Vec4f32 r = a;
    r += b;
    return r;
}
static constexpr Vec4f32 operator-(const Vec4f32& a, float b) noexcept {
    Vec4f32 r = a;
    r -= b;
    return r;
}
static constexpr Vec4f32 operator*(const Vec4f32& a, float b) noexcept {
    Vec4f32 r = a;
    r *= b;
    return r;
}
static constexpr Vec4f32 operator/(const Vec4f32& a, float b) noexcept {
    Vec4f32 r = a;
    r /= b;
    return r;
}

static constexpr Vec4f32 operator+(float a, const Vec4f32& b) noexcept {
    Vec4f32 r = Vec4f32::FromSingle(a);
    r += b;
    return r;
}
static constexpr Vec4f32 operator-(float a, const Vec4f32& b) noexcept {
    Vec4f32 r = Vec4f32::FromSingle(a);
    r -= b;
    return r;
}
static constexpr Vec4f32 operator*(float a, const Vec4f32& b) noexcept {
    Vec4f32 r = Vec4f32::FromSingle(a);
    r *= b;
    return r;
}
static constexpr Vec4f32 operator/(float a, const Vec4f32& b) noexcept {
    Vec4f32 r = Vec4f32::FromSingle(a);
    r /= b;
    return r;
}
}
