#pragma once

namespace qwqdsp::psimd {
// ---------------------------------------- 16int ----------------------------------------
#ifndef __AVX512__
#ifndef __AVX__ || __AVX2__
struct alignas(16) Vec16i32 {
#else
struct alignas(32) Vec16i32 {
#endif
#else
struct alignas(64) Vec16i32 {
#endif
    static constexpr size_t kSize = 16;

    int x[16];

    static constexpr Vec16i32 FromSingle(int v) noexcept {
        Vec16i32 r;
        r.x[0]  = v;
        r.x[1]  = v;
        r.x[2]  = v;
        r.x[3]  = v;
        r.x[4]  = v;
        r.x[5]  = v;
        r.x[6]  = v;
        r.x[7]  = v;
        r.x[8]  = v;
        r.x[9]  = v;
        r.x[10] = v;
        r.x[11] = v;
        r.x[12] = v;
        r.x[13] = v;
        r.x[14] = v;
        r.x[15] = v;
        return r;
    }

    constexpr Vec16i32& operator+=(const Vec16i32& v) noexcept {
        x[0]  += v.x[0];
        x[1]  += v.x[1];
        x[2]  += v.x[2];
        x[3]  += v.x[3];
        x[4]  += v.x[4];
        x[5]  += v.x[5];
        x[6]  += v.x[6];
        x[7]  += v.x[7];
        x[8]  += v.x[8];
        x[9]  += v.x[9];
        x[10] += v.x[10];
        x[11] += v.x[11];
        x[12] += v.x[12];
        x[13] += v.x[13];
        x[14] += v.x[14];
        x[15] += v.x[15];
        return *this;
    }

    constexpr  Vec16i32& operator-=(const Vec16i32& v) noexcept {
        x[0]  -= v.x[0];
        x[1]  -= v.x[1];
        x[2]  -= v.x[2];
        x[3]  -= v.x[3];
        x[4]  -= v.x[4];
        x[5]  -= v.x[5];
        x[6]  -= v.x[6];
        x[7]  -= v.x[7];
        x[8]  -= v.x[8];
        x[9]  -= v.x[9];
        x[10] -= v.x[10];
        x[11] -= v.x[11];
        x[12] -= v.x[12];
        x[13] -= v.x[13];
        x[14] -= v.x[14];
        x[15] -= v.x[15];
        return *this;
    }

    constexpr  Vec16i32& operator*=(const Vec16i32& v) noexcept {
        x[0]  *= v.x[0];
        x[1]  *= v.x[1];
        x[2]  *= v.x[2];
        x[3]  *= v.x[3];
        x[4]  *= v.x[4];
        x[5]  *= v.x[5];
        x[6]  *= v.x[6];
        x[7]  *= v.x[7];
        x[8]  *= v.x[8];
        x[9]  *= v.x[9];
        x[10] *= v.x[10];
        x[11] *= v.x[11];
        x[12] *= v.x[12];
        x[13] *= v.x[13];
        x[14] *= v.x[14];
        x[15] *= v.x[15];
        return *this;
    }

    constexpr Vec16i32& operator/=(const Vec16i32& v) noexcept {
        x[0]  /= v.x[0];
        x[1]  /= v.x[1];
        x[2]  /= v.x[2];
        x[3]  /= v.x[3];
        x[4]  /= v.x[4];
        x[5]  /= v.x[5];
        x[6]  /= v.x[6];
        x[7]  /= v.x[7];
        x[8]  /= v.x[8];
        x[9]  /= v.x[9];
        x[10] /= v.x[10];
        x[11] /= v.x[11];
        x[12] /= v.x[12];
        x[13] /= v.x[13];
        x[14] /= v.x[14];
        x[15] /= v.x[15];
        return *this;
    }

    constexpr Vec16i32& operator&=(const Vec16i32& v) noexcept {
        x[0]  += v.x[0];
        x[1]  += v.x[1];
        x[2]  += v.x[2];
        x[3]  += v.x[3];
        x[4]  += v.x[4];
        x[5]  += v.x[5];
        x[6]  += v.x[6];
        x[7]  += v.x[7];
        x[8]  += v.x[8];
        x[9]  += v.x[9];
        x[10] += v.x[10];
        x[11] += v.x[11];
        x[12] += v.x[12];
        x[13] += v.x[13];
        x[14] += v.x[14];
        x[15] += v.x[15];
        return *this;
    }

    constexpr Vec16i32& FMA(const Vec16i32& mul, const Vec16i32& add) noexcept {
        x[0]  = x[0]  * mul.x[0]  + add.x[0];
        x[1]  = x[1]  * mul.x[1]  + add.x[1];
        x[2]  = x[2]  * mul.x[2]  + add.x[2];
        x[3]  = x[3]  * mul.x[3]  + add.x[3];
        x[4]  = x[4]  * mul.x[4]  + add.x[4];
        x[5]  = x[5]  * mul.x[5]  + add.x[5];
        x[6]  = x[6]  * mul.x[6]  + add.x[6];
        x[7]  = x[7]  * mul.x[7]  + add.x[7];
        x[8]  = x[8]  * mul.x[8]  + add.x[8];
        x[9]  = x[9]  * mul.x[9]  + add.x[9];
        x[10] = x[10] * mul.x[10] + add.x[10];
        x[11] = x[11] * mul.x[11] + add.x[11];
        x[12] = x[12] * mul.x[12] + add.x[12];
        x[13] = x[13] * mul.x[13] + add.x[13];
        x[14] = x[14] * mul.x[14] + add.x[14];
        x[15] = x[15] * mul.x[15] + add.x[15];
        return *this;
    }
};

static constexpr Vec16i32 operator+(const Vec16i32& a, const Vec16i32& b) noexcept {
    Vec16i32 r = a;
    r += b;
    return r;
}
static constexpr Vec16i32 operator-(const Vec16i32& a, const Vec16i32& b) noexcept {
    Vec16i32 r = a;
    r -= b;
    return r;
}
static constexpr Vec16i32 operator*(const Vec16i32& a, const Vec16i32& b) noexcept {
    Vec16i32 r = a;
    r *= b;
    return r;
}
static constexpr Vec16i32 operator/(const Vec16i32& a, const Vec16i32& b) noexcept {
    Vec16i32 r = a;
    r /= b;
    return r;
}
static constexpr Vec16i32 operator&(const Vec16i32& a, const Vec16i32& b) noexcept {
    Vec16i32 r = a;
    r &= b;
    return r;
}

// ---------------------------------------- 16float ----------------------------------------
struct alignas(64) Vec16f32 {
    static constexpr size_t kSize = 4;
    using IntType = Vec16i32;

    float x[16];

    static constexpr Vec16f32 FromSingle(float v) noexcept {
        Vec16f32 r;
        r.x[0]  = v;
        r.x[1]  = v;
        r.x[2]  = v;
        r.x[3]  = v;
        r.x[4]  = v;
        r.x[5]  = v;
        r.x[6]  = v;
        r.x[7]  = v;
        r.x[8]  = v;
        r.x[9]  = v;
        r.x[10] = v;
        r.x[11] = v;
        r.x[12] = v;
        r.x[13] = v;
        r.x[14] = v;
        r.x[15] = v;
        return r;
    }

    constexpr Vec16f32& operator+=(const Vec16f32& v) noexcept {
        x[0]  += v.x[0];
        x[1]  += v.x[1];
        x[2]  += v.x[2];
        x[3]  += v.x[3];
        x[4]  += v.x[4];
        x[5]  += v.x[5];
        x[6]  += v.x[6];
        x[7]  += v.x[7];
        x[8]  += v.x[8];
        x[9]  += v.x[9];
        x[10] += v.x[10];
        x[11] += v.x[11];
        x[12] += v.x[12];
        x[13] += v.x[13];
        x[14] += v.x[14];
        x[15] += v.x[15];
        return *this;
    }

    constexpr Vec16f32& operator-=(const Vec16f32& v) noexcept {
        x[0]  -= v.x[0];
        x[1]  -= v.x[1];
        x[2]  -= v.x[2];
        x[3]  -= v.x[3];
        x[4]  -= v.x[4];
        x[5]  -= v.x[5];
        x[6]  -= v.x[6];
        x[7]  -= v.x[7];
        x[8]  -= v.x[8];
        x[9]  -= v.x[9];
        x[10] -= v.x[10];
        x[11] -= v.x[11];
        x[12] -= v.x[12];
        x[13] -= v.x[13];
        x[14] -= v.x[14];
        x[15] -= v.x[15];
        return *this;
    }

    constexpr Vec16f32& operator*=(const Vec16f32& v) noexcept {
        x[0]  *= v.x[0];
        x[1]  *= v.x[1];
        x[2]  *= v.x[2];
        x[3]  *= v.x[3];
        x[4]  *= v.x[4];
        x[5]  *= v.x[5];
        x[6]  *= v.x[6];
        x[7]  *= v.x[7];
        x[8]  *= v.x[8];
        x[9]  *= v.x[9];
        x[10] *= v.x[10];
        x[11] *= v.x[11];
        x[12] *= v.x[12];
        x[13] *= v.x[13];
        x[14] *= v.x[14];
        x[15] *= v.x[15];
        return *this;
    }

    constexpr Vec16f32& operator/=(const Vec16f32& v) noexcept {
        x[0]  /= v.x[0];
        x[1]  /= v.x[1];
        x[2]  /= v.x[2];
        x[3]  /= v.x[3];
        x[4]  /= v.x[4];
        x[5]  /= v.x[5];
        x[6]  /= v.x[6];
        x[7]  /= v.x[7];
        x[8]  /= v.x[8];
        x[9]  /= v.x[9];
        x[10] /= v.x[10];
        x[11] /= v.x[11];
        x[12] /= v.x[12];
        x[13] /= v.x[13];
        x[14] /= v.x[14];
        x[15] /= v.x[15];
        return *this;
    }

    constexpr Vec16f32 Frac() const noexcept {
        Vec16f32 r;
        r.x[0]  = x[0]  - static_cast<int>(x[0]);
        r.x[1]  = x[1]  - static_cast<int>(x[1]);
        r.x[2]  = x[2]  - static_cast<int>(x[2]);
        r.x[3]  = x[3]  - static_cast<int>(x[3]);
        r.x[4]  = x[4]  - static_cast<int>(x[4]);
        r.x[5]  = x[5]  - static_cast<int>(x[5]);
        r.x[6]  = x[6]  - static_cast<int>(x[6]);
        r.x[8]  = x[8]  - static_cast<int>(x[8]);
        r.x[9]  = x[9]  - static_cast<int>(x[9]);
        r.x[10] = x[10] - static_cast<int>(x[10]);
        r.x[11] = x[11] - static_cast<int>(x[11]);
        r.x[12] = x[12] - static_cast<int>(x[12]);
        r.x[13] = x[13] - static_cast<int>(x[13]);
        r.x[14] = x[14] - static_cast<int>(x[14]);
        r.x[15] = x[15] - static_cast<int>(x[15]);
        return r;
    }

    constexpr Vec16i32 ToInt() const noexcept {
        Vec16i32 r;
        r.x[0]  = static_cast<int>(x[0]);
        r.x[1]  = static_cast<int>(x[1]);
        r.x[2]  = static_cast<int>(x[2]);
        r.x[3]  = static_cast<int>(x[3]);
        r.x[4]  = static_cast<int>(x[4]);
        r.x[5]  = static_cast<int>(x[5]);
        r.x[6]  = static_cast<int>(x[6]);
        r.x[8]  = static_cast<int>(x[8]);
        r.x[9]  = static_cast<int>(x[9]);
        r.x[10] = static_cast<int>(x[10]);
        r.x[11] = static_cast<int>(x[11]);
        r.x[12] = static_cast<int>(x[12]);
        r.x[13] = static_cast<int>(x[13]);
        r.x[14] = static_cast<int>(x[14]);
        r.x[15] = static_cast<int>(x[15]);
        return r;
    }

    constexpr float ReduceAdd() const noexcept {
        return x[0] + x[1] + x[2] + x[3] + x[4] + x[5] + x[6] + x[7] + x[8] + x[9] + x[10] + x[11] + x[12] + x[13] + x[14] + x[15];
    }

    constexpr Vec16f32& FMA(const Vec16f32& mul, const Vec16f32& add) noexcept {
        x[0]  = x[0]  * mul.x[0]  + add.x[0];
        x[1]  = x[1]  * mul.x[1]  + add.x[1];
        x[2]  = x[2]  * mul.x[2]  + add.x[2];
        x[3]  = x[3]  * mul.x[3]  + add.x[3];
        x[4]  = x[4]  * mul.x[4]  + add.x[4];
        x[5]  = x[5]  * mul.x[5]  + add.x[5];
        x[6]  = x[6]  * mul.x[6]  + add.x[6];
        x[7]  = x[7]  * mul.x[7]  + add.x[7];
        x[8]  = x[8]  * mul.x[8]  + add.x[8];
        x[9]  = x[9]  * mul.x[9]  + add.x[9];
        x[10] = x[10] * mul.x[10] + add.x[10];
        x[11] = x[11] * mul.x[11] + add.x[11];
        x[12] = x[12] * mul.x[12] + add.x[12];
        x[13] = x[13] * mul.x[13] + add.x[13];
        x[14] = x[14] * mul.x[14] + add.x[14];
        x[15] = x[15] * mul.x[15] + add.x[15];
        return *this;
    }
};

static constexpr Vec16f32 operator+(const Vec16f32& a, const Vec16f32& b) noexcept {
    Vec16f32 r = a;
    r += b;
    return r;
}
static constexpr Vec16f32 operator-(const Vec16f32& a, const Vec16f32& b) noexcept {
    Vec16f32 r = a;
    r -= b;
    return r;
}
static constexpr Vec16f32 operator*(const Vec16f32& a, const Vec16f32& b) noexcept {
    Vec16f32 r = a;
    r *= b;
    return r;
}
static constexpr Vec16f32 operator/(const Vec16f32& a, const Vec16f32& b) noexcept {
    Vec16f32 r = a;
    r /= b;
    return r;
}
}