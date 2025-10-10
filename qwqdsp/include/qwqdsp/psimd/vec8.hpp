#pragma once

namespace qwqdsp::psimd {
// ---------------------------------------- 8int ----------------------------------------
#ifndef __AVX__ || __AVX2__
struct alignas(16) Vec8i32 {
#else
struct alignas(32) Vec8i32 {
#endif
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
#ifndef __AVX__ || __AVX2__
struct alignas(16) Vec8f32 {
#else
struct alignas(32) Vec8f32 {
#endif
    static constexpr size_t kSize = 8;
    using IntType = Vec8i32;
    float x[8];

    static constexpr Vec8f32 FromSingle(float v) noexcept {
        Vec8f32 r;
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

    constexpr Vec8f32 Frac() const noexcept {
        Vec8f32 r;
        r.x[0] = x[0] - static_cast<int>(x[0]);
        r.x[1] = x[1] - static_cast<int>(x[1]);
        r.x[2] = x[2] - static_cast<int>(x[2]);
        r.x[3] = x[3] - static_cast<int>(x[3]);
        r.x[4] = x[4] - static_cast<int>(x[4]);
        r.x[5] = x[5] - static_cast<int>(x[5]);
        r.x[6] = x[6] - static_cast<int>(x[6]);
        r.x[7] = x[7] - static_cast<int>(x[7]);
        return r;
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

    constexpr float ReduceAdd() const noexcept {
        return x[0] + x[1] + x[2] + x[3] + x[4] + x[5] + x[6] + x[7];
    }

    constexpr Vec8f32& FMA(const Vec8f32& mul, const Vec8f32& add) noexcept {
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
}