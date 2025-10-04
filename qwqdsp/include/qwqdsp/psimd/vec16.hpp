#pragma once

namespace qwqdsp::psimd {
// ---------------------------------------- 8int ----------------------------------------
struct alignas(64) Vec16i32 {
    int x[16];

    static constexpr Vec16i32 FromSingle(int v) {
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
        r.x[16] = v;
        return r;
    }

    constexpr Vec16i32& operator+=(const Vec16i32& v) {
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

    constexpr  Vec16i32& operator-=(const Vec16i32& v) {
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

    constexpr  Vec16i32& operator*=(const Vec16i32& v) {
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

    constexpr Vec16i32& operator/=(const Vec16i32& v) {
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

    constexpr Vec16i32& operator&=(const Vec16i32& v) {
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
};

static constexpr Vec16i32 operator+(const Vec16i32& a, const Vec16i32& b) {
    Vec16i32 r = a;
    r += b;
    return r;
}
static constexpr Vec16i32 operator-(const Vec16i32& a, const Vec16i32& b) {
    Vec16i32 r = a;
    r -= b;
    return r;
}
static constexpr Vec16i32 operator*(const Vec16i32& a, const Vec16i32& b) {
    Vec16i32 r = a;
    r *= b;
    return r;
}
static constexpr Vec16i32 operator/(const Vec16i32& a, const Vec16i32& b) {
    Vec16i32 r = a;
    r /= b;
    return r;
}
static constexpr Vec16i32 operator&(const Vec16i32& a, const Vec16i32& b) {
    Vec16i32 r = a;
    r &= b;
    return r;
}

// ---------------------------------------- 8float ----------------------------------------
struct alignas(64) Vec16f32 {
    float x[16];

    static constexpr Vec16f32 FromSingle(float v) {
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
        r.x[16] = v;
        return r;
    }

    constexpr Vec16f32& operator+=(const Vec16f32& v) {
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

    constexpr Vec16f32& operator-=(const Vec16f32& v) {
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

    constexpr Vec16f32& operator*=(const Vec16f32& v) {
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

    constexpr Vec16f32& operator/=(const Vec16f32& v) {
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
};

static constexpr Vec16f32 operator+(const Vec16f32& a, const Vec16f32& b) {
    Vec16f32 r = a;
    r += b;
    return r;
}
static constexpr Vec16f32 operator-(const Vec16f32& a, const Vec16f32& b) {
    Vec16f32 r = a;
    r -= b;
    return r;
}
static constexpr Vec16f32 operator*(const Vec16f32& a, const Vec16f32& b) {
    Vec16f32 r = a;
    r *= b;
    return r;
}
static constexpr Vec16f32 operator/(const Vec16f32& a, const Vec16f32& b) {
    Vec16f32 r = a;
    r /= b;
    return r;
}
}