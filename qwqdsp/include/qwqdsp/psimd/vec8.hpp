#pragma once
#include <cmath>

namespace qwqdsp_psimd {
// ---------------------------------------- 8int ----------------------------------------
struct alignas(32) Vec8i32 {
    static constexpr size_t kSize = 8;

    int x[8];

    static constexpr Vec8i32 FromSingle(int v) noexcept {
        Vec8i32 r;
        r.x[0] = v;
        r.x[1] = v;
        r.x[2] = v;
        r.x[3] = v;
        r.x[4] = v;
        r.x[5] = v;
        r.x[6] = v;
        r.x[7] = v;
        return r;
    }

    constexpr Vec8i32& operator+=(const Vec8i32& v) noexcept {
        x[0] += v.x[0];
        x[1] += v.x[1];
        x[2] += v.x[2];
        x[3] += v.x[3];
        x[4] += v.x[4];
        x[5] += v.x[5];
        x[6] += v.x[6];
        x[7] += v.x[7];
        return *this;
    }

    constexpr  Vec8i32& operator-=(const Vec8i32& v) noexcept {
        x[0] -= v.x[0];
        x[1] -= v.x[1];
        x[2] -= v.x[2];
        x[3] -= v.x[3];
        x[4] -= v.x[4];
        x[5] -= v.x[5];
        x[6] -= v.x[6];
        x[7] -= v.x[7];
        return *this;
    }

    constexpr  Vec8i32& operator*=(const Vec8i32& v) noexcept {
        x[0] *= v.x[0];
        x[1] *= v.x[1];
        x[2] *= v.x[2];
        x[3] *= v.x[3];
        x[4] *= v.x[4];
        x[5] *= v.x[5];
        x[6] *= v.x[6];
        x[7] *= v.x[7];
        return *this;
    }

    constexpr Vec8i32& operator/=(const Vec8i32& v) noexcept {
        x[0] /= v.x[0];
        x[1] /= v.x[1];
        x[2] /= v.x[2];
        x[3] /= v.x[3];
        x[4] /= v.x[4];
        x[5] /= v.x[5];
        x[6] /= v.x[6];
        x[7] /= v.x[7];
        return *this;
    }

    constexpr Vec8i32& operator&=(const Vec8i32& v) noexcept {
        x[0] &= v.x[0];
        x[1] &= v.x[1];
        x[2] &= v.x[2];
        x[3] &= v.x[3];
        x[4] &= v.x[4];
        x[5] &= v.x[5];
        x[6] &= v.x[6];
        x[7] &= v.x[7];
        return *this;
    }

    constexpr Vec8i32& FMA(const Vec8i32& mul, const Vec8i32& add) noexcept {
        x[0] = x[0] * mul.x[0] + add.x[0];
        x[1] = x[1] * mul.x[1] + add.x[1];
        x[2] = x[2] * mul.x[2] + add.x[2];
        x[3] = x[3] * mul.x[3] + add.x[3];
        x[4] = x[4] * mul.x[4] + add.x[4];
        x[5] = x[5] * mul.x[5] + add.x[5];
        x[6] = x[6] * mul.x[6] + add.x[6];
        x[7] = x[7] * mul.x[7] + add.x[7];
        return *this;
    }
};

static constexpr Vec8i32 operator+(const Vec8i32& a, const Vec8i32& b) noexcept {
    Vec8i32 r = a;
    r += b;
    return r;
}
static constexpr Vec8i32 operator-(const Vec8i32& a, const Vec8i32& b) noexcept {
    Vec8i32 r = a;
    r -= b;
    return r;
}
static constexpr Vec8i32 operator*(const Vec8i32& a, const Vec8i32& b) noexcept {
    Vec8i32 r = a;
    r *= b;
    return r;
}
static constexpr Vec8i32 operator/(const Vec8i32& a, const Vec8i32& b) noexcept {
    Vec8i32 r = a;
    r /= b;
    return r;
}
static constexpr Vec8i32 operator&(const Vec8i32& a, const Vec8i32& b) noexcept {
    Vec8i32 r = a;
    r &= b;
    return r;
}

// ---------------------------------------- 8float ----------------------------------------
struct alignas(32) Vec8f32 {
    static constexpr size_t kSize = 8;
    using IntType = Vec8i32;
    float x[8];

    static constexpr Vec8f32 FromSingle(float v) noexcept {
        return Vec8f32{
            v,
            v,
            v,
            v,
            v,
            v,
            v,
            v
        };
    }

    static constexpr Vec8f32 Cross(float left, float right) noexcept {
        return Vec8f32{
            left,
            right,
            left,
            right,
            left,
            right,
            left,
            right
        };
    }

    [[nodiscard]]
    constexpr Vec8f32 Decross() noexcept {
        return Vec8f32{
            x[0] + x[2] + x[4] + x[6],
            x[1] + x[3] + x[5] + x[7]
        };
    }

    template<int a, int b, int c, int d, int e, int f, int g, int h>
    [[nodiscard]]
    constexpr Vec8f32 Shuffle() noexcept {
        return Vec8f32{
            x[a],
            x[b],
            x[c],
            x[d],
            x[e],
            x[f],
            x[g],
            x[h]
        };
    }

    template<int a, int b, int c, int d, int e, int f, int g, int h>
    [[nodiscard]]
    constexpr Vec8f32 Blend(const Vec8f32& other) noexcept {
        return Vec8f32{
            a == 0 ? x[0] : other.x[0],
            b == 0 ? x[1] : other.x[1],
            c == 0 ? x[2] : other.x[2],
            d == 0 ? x[3] : other.x[3],
            e == 0 ? x[4] : other.x[4],
            f == 0 ? x[5] : other.x[5],
            g == 0 ? x[6] : other.x[6],
            h == 0 ? x[7] : other.x[7],
        };
    }

    constexpr Vec8f32& operator+=(const Vec8f32& v) noexcept {
        x[0] += v.x[0];
        x[1] += v.x[1];
        x[2] += v.x[2];
        x[3] += v.x[3];
        x[4] += v.x[4];
        x[5] += v.x[5];
        x[6] += v.x[6];
        x[7] += v.x[7];
        return *this;
    }

    constexpr Vec8f32& operator-=(const Vec8f32& v) noexcept {
        x[0] -= v.x[0];
        x[1] -= v.x[1];
        x[2] -= v.x[2];
        x[3] -= v.x[3];
        x[4] -= v.x[4];
        x[5] -= v.x[5];
        x[6] -= v.x[6];
        x[7] -= v.x[7];
        return *this;
    }

    constexpr Vec8f32& operator*=(const Vec8f32& v) noexcept {
        x[0] *= v.x[0];
        x[1] *= v.x[1];
        x[2] *= v.x[2];
        x[3] *= v.x[3];
        x[4] *= v.x[4];
        x[5] *= v.x[5];
        x[6] *= v.x[6];
        x[7] *= v.x[7];
        return *this;
    }

    constexpr Vec8f32& operator/=(const Vec8f32& v) noexcept {
        x[0] /= v.x[0];
        x[1] /= v.x[1];
        x[2] /= v.x[2];
        x[3] /= v.x[3];
        x[4] /= v.x[4];
        x[5] /= v.x[5];
        x[6] /= v.x[6];
        x[7] /= v.x[7];
        return *this;
    }

    constexpr Vec8f32& operator+=(float v) noexcept {
        x[0] += v;
        x[1] += v;
        x[2] += v;
        x[3] += v;
        x[4] += v;
        x[5] += v;
        x[6] += v;
        x[7] += v;
        return *this;
    }

    constexpr Vec8f32& operator-=(float v) noexcept {
        x[0] -= v;
        x[1] -= v;
        x[2] -= v;
        x[3] -= v;
        x[4] -= v;
        x[5] -= v;
        x[6] -= v;
        x[7] -= v;
        return *this;
    }

    constexpr Vec8f32& operator*=(float v) noexcept {
        x[0] *= v;
        x[1] *= v;
        x[2] *= v;
        x[3] *= v;
        x[4] *= v;
        x[5] *= v;
        x[6] *= v;
        x[7] *= v;
        return *this;
    }

    constexpr Vec8f32& operator/=(float v) noexcept {
        x[0] /= v;
        x[1] /= v;
        x[2] /= v;
        x[3] /= v;
        x[4] /= v;
        x[5] /= v;
        x[6] /= v;
        x[7] /= v;
        return *this;
    }

    Vec8f32 Frac() const noexcept {
        return Vec8f32{
            x[0] - std::floor(x[0]),
            x[1] - std::floor(x[1]),
            x[2] - std::floor(x[2]),
            x[3] - std::floor(x[3]),
            x[4] - std::floor(x[4]),
            x[5] - std::floor(x[5]),
            x[6] - std::floor(x[6]),
            x[7] - std::floor(x[7])
        };
    }

    constexpr Vec8f32 PositiveFrac() const noexcept {
        return Vec8f32{
            x[0] - static_cast<int>(x[0]),
            x[1] - static_cast<int>(x[1]),
            x[2] - static_cast<int>(x[2]),
            x[3] - static_cast<int>(x[3]),
            x[4] - static_cast<int>(x[4]),
            x[5] - static_cast<int>(x[5]),
            x[6] - static_cast<int>(x[6]),
            x[7] - static_cast<int>(x[7])
        };
    }

    constexpr Vec8i32 ToInt() const noexcept {
        Vec8i32 r;
        r.x[0] = static_cast<int>(x[0]);
        r.x[1] = static_cast<int>(x[1]);
        r.x[2] = static_cast<int>(x[2]);
        r.x[3] = static_cast<int>(x[3]);
        r.x[4] = static_cast<int>(x[4]);
        r.x[5] = static_cast<int>(x[5]);
        r.x[6] = static_cast<int>(x[6]);
        r.x[7] = static_cast<int>(x[7]);
        return r;
    }

    static Vec8f32 Sqrt(Vec8f32 const& x) noexcept {
        return Vec8f32{
            std::sqrt(x.x[0]),
            std::sqrt(x.x[1]),
            std::sqrt(x.x[2]),
            std::sqrt(x.x[3]),
            std::sqrt(x.x[4]),
            std::sqrt(x.x[5]),
            std::sqrt(x.x[6]),
            std::sqrt(x.x[7]),
        };
    }

    static Vec8f32 Abs(Vec8f32 const& x) noexcept {
        return Vec8f32{
            std::abs(x.x[0]),
            std::abs(x.x[1]),
            std::abs(x.x[2]),
            std::abs(x.x[3]),
            std::abs(x.x[4]),
            std::abs(x.x[5]),
            std::abs(x.x[6]),
            std::abs(x.x[7]),
        };
    }

    constexpr float ReduceAdd() const noexcept {
        return x[0] + x[1] + x[2] + x[3] + x[4] + x[5] + x[6] + x[7];
    }

    static constexpr Vec8f32 FMA(Vec8f32 const& mula, Vec8f32 const& mulb, Vec8f32 const& add) noexcept {
        return Vec8f32{
            mula.x[0] * mulb.x[0] + add.x[0],
            mula.x[1] * mulb.x[1] + add.x[1],
            mula.x[2] * mulb.x[2] + add.x[2],
            mula.x[3] * mulb.x[3] + add.x[3],
            mula.x[4] * mulb.x[4] + add.x[4],
            mula.x[5] * mulb.x[5] + add.x[5],
            mula.x[6] * mulb.x[6] + add.x[6],
            mula.x[7] * mulb.x[7] + add.x[7]
        };
    }

    constexpr Vec8f32 Abs() const noexcept {
        return Vec8f32{
            std::abs(x[0]),
            std::abs(x[1]),
            std::abs(x[2]),
            std::abs(x[3]),
            std::abs(x[4]),
            std::abs(x[5]),
            std::abs(x[6]),
            std::abs(x[7])
        };
    }

    static constexpr Vec8f32 CopySign(Vec8f32 const& abs, Vec8f32 const& sign) noexcept {
        return Vec8f32{
            std::copysign(abs.x[0], sign.x[0]),
            std::copysign(abs.x[1], sign.x[1]),
            std::copysign(abs.x[2], sign.x[2]),
            std::copysign(abs.x[3], sign.x[3]),
            std::copysign(abs.x[4], sign.x[4]),
            std::copysign(abs.x[5], sign.x[5]),
            std::copysign(abs.x[6], sign.x[6]),
            std::copysign(abs.x[7], sign.x[7])
        };
    }
};

static constexpr Vec8f32 operator+(const Vec8f32& a, const Vec8f32& b) noexcept {
    Vec8f32 r = a;
    r += b;
    return r;
}
static constexpr Vec8f32 operator-(const Vec8f32& a, const Vec8f32& b) noexcept {
    Vec8f32 r = a;
    r -= b;
    return r;
}
static constexpr Vec8f32 operator*(const Vec8f32& a, const Vec8f32& b) noexcept {
    Vec8f32 r = a;
    r *= b;
    return r;
}
static constexpr Vec8f32 operator/(const Vec8f32& a, const Vec8f32& b) noexcept {
    Vec8f32 r = a;
    r /= b;
    return r;
}

static constexpr Vec8f32 operator+(const Vec8f32& a, float b) noexcept {
    Vec8f32 r = a;
    r += b;
    return r;
}
static constexpr Vec8f32 operator-(const Vec8f32& a, float b) noexcept {
    Vec8f32 r = a;
    r -= b;
    return r;
}
static constexpr Vec8f32 operator*(const Vec8f32& a, float b) noexcept {
    Vec8f32 r = a;
    r *= b;
    return r;
}
static constexpr Vec8f32 operator/(const Vec8f32& a, float b) noexcept {
    Vec8f32 r = a;
    r /= b;
    return r;
}

static constexpr Vec8f32 operator+(float a, const Vec8f32& b) noexcept {
    Vec8f32 r = Vec8f32::FromSingle(a);
    r += b;
    return r;
}
static constexpr Vec8f32 operator-(float a, const Vec8f32& b) noexcept {
    Vec8f32 r = Vec8f32::FromSingle(a);
    r -= b;
    return r;
}
static constexpr Vec8f32 operator*(float a, const Vec8f32& b) noexcept {
    Vec8f32 r = Vec8f32::FromSingle(a);
    r *= b;
    return r;
}
static constexpr Vec8f32 operator/(float a, const Vec8f32& b) noexcept {
    Vec8f32 r = Vec8f32::FromSingle(a);
    r /= b;
    return r;
}
}
