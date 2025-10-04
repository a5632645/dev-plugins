#pragma once

namespace qwqdsp::psimd {
// ---------------------------------------- 4int ----------------------------------------
struct alignas(16) Vec4i32 {
    int x[4];

    static constexpr Vec4i32 FromSingle(int v) {
        Vec4i32 r;
        r.x[0] = v;
        r.x[1] = v;
        r.x[2] = v;
        r.x[3] = v;
        return r;
    }

    constexpr Vec4i32& operator+=(const Vec4i32& v) {
        x[0] += v.x[0];
        x[1] += v.x[1];
        x[2] += v.x[2];
        x[3] += v.x[3];
        return *this;
    }

    constexpr  Vec4i32& operator-=(const Vec4i32& v) {
        x[0] -= v.x[0];
        x[1] -= v.x[1];
        x[2] -= v.x[2];
        x[3] -= v.x[3];
        return *this;
    }

    constexpr  Vec4i32& operator*=(const Vec4i32& v) {
        x[0] *= v.x[0];
        x[1] *= v.x[1];
        x[2] *= v.x[2];
        x[3] *= v.x[3];
        return *this;
    }

    constexpr Vec4i32& operator/=(const Vec4i32& v) {
        x[0] /= v.x[0];
        x[1] /= v.x[1];
        x[2] /= v.x[2];
        x[3] /= v.x[3];
        return *this;
    }

    constexpr Vec4i32& operator&=(const Vec4i32& v) {
        x[0] &= v.x[0];
        x[1] &= v.x[1];
        x[2] &= v.x[2];
        x[3] &= v.x[3];
        return *this;
    }
};

static constexpr Vec4i32 operator+(const Vec4i32& a, const Vec4i32& b) {
    Vec4i32 r = a;
    r += b;
    return r;
}
static constexpr Vec4i32 operator-(const Vec4i32& a, const Vec4i32& b) {
    Vec4i32 r = a;
    r -= b;
    return r;
}
static constexpr Vec4i32 operator*(const Vec4i32& a, const Vec4i32& b) {
    Vec4i32 r = a;
    r *= b;
    return r;
}
static constexpr Vec4i32 operator/(const Vec4i32& a, const Vec4i32& b) {
    Vec4i32 r = a;
    r /= b;
    return r;
}
static constexpr Vec4i32 operator&(const Vec4i32& a, const Vec4i32& b) {
    Vec4i32 r = a;
    r &= b;
    return r;
}

// ---------------------------------------- 4float ----------------------------------------

struct alignas(16) Vec4f32 {
    float x[4];

    static constexpr Vec4f32 FromSingle(float v) {
        Vec4f32 r;
        r.x[0] = v;
        r.x[1] = v;
        r.x[2] = v;
        r.x[3] = v;
        return r;
    }

    constexpr Vec4f32& operator+=(const Vec4f32& v) {
        x[0] += v.x[0];
        x[1] += v.x[1];
        x[2] += v.x[2];
        x[3] += v.x[3];
        return *this;
    }

    constexpr  Vec4f32& operator-=(const Vec4f32& v) {
        x[0] -= v.x[0];
        x[1] -= v.x[1];
        x[2] -= v.x[2];
        x[3] -= v.x[3];
        return *this;
    }

    constexpr  Vec4f32& operator*=(const Vec4f32& v) {
        x[0] *= v.x[0];
        x[1] *= v.x[1];
        x[2] *= v.x[2];
        x[3] *= v.x[3];
        return *this;
    }

    constexpr Vec4f32& operator/=(const Vec4f32& v) {
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
};

static constexpr Vec4f32 operator+(const Vec4f32& a, const Vec4f32& b) {
    Vec4f32 r = a;
    r += b;
    return r;
}
static constexpr Vec4f32 operator-(const Vec4f32& a, const Vec4f32& b) {
    Vec4f32 r = a;
    r -= b;
    return r;
}
static constexpr Vec4f32 operator*(const Vec4f32& a, const Vec4f32& b) {
    Vec4f32 r = a;
    r *= b;
    return r;
}
static constexpr Vec4f32 operator/(const Vec4f32& a, const Vec4f32& b) {
    Vec4f32 r = a;
    r /= b;
    return r;
}
}