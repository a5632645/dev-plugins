#pragma once

namespace qwqdsp::psimd {
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

    constexpr Vec4f32 Frac() const noexcept {
        Vec4f32 r;
        r.x[0] = x[0] - static_cast<int>(x[0]);
        r.x[1] = x[1] - static_cast<int>(x[1]);
        r.x[2] = x[2] - static_cast<int>(x[2]);
        r.x[3] = x[3] - static_cast<int>(x[3]);
        return r;
    }

    constexpr Vec4i32 ToInt() const noexcept {
        Vec4i32 r;
        r.x[0] = static_cast<int>(x[0]);
        r.x[1] = static_cast<int>(x[1]);
        r.x[2] = static_cast<int>(x[2]);
        r.x[3] = static_cast<int>(x[3]);
        return r;
    }

    constexpr float ReduceAdd() const noexcept {
        return x[0] + x[1] + x[2] + x[3];
    }

    constexpr Vec4f32& FMA(const Vec4f32& mul, const Vec4f32& add) noexcept {
        x[0] = x[0] * mul.x[0] + add.x[0];
        x[1] = x[1] * mul.x[1] + add.x[1];
        x[2] = x[2] * mul.x[2] + add.x[2];
        x[3] = x[3] * mul.x[3] + add.x[3];
        return *this;
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
}