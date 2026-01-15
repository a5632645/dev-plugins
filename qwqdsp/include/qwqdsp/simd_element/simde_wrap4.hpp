#pragma once

#include <x86/sse4.1.h>

#include <array>

namespace qwqdsp_simd_element {

struct V4i;

// ----------------------------------------
// float 128
// ----------------------------------------

struct V4f {
    simde__m128 m128;

    // -------------------- constructor --------------------
    V4f(simde__m128 m128) : m128(m128) {
    }

    V4f(float a, float b, float c, float d) : m128(simde_mm_set_ps(d, c, b, a)) {
    }

    V4f(float v) : m128(simde_mm_set1_ps(v)) {
    }

    // -------------------- load/store --------------------
    void Broadcast(float v) noexcept {
        m128 = simde_mm_set1_ps(v);
    }

    void Load(float const* ptr) noexcept {
        m128 = simde_mm_load_ps(ptr);
    }
    void Loadu(float const* ptr) noexcept {
        m128 = simde_mm_loadu_ps(ptr);
    }

    void Store(float* ptr) const noexcept {
        simde_mm_store_ps(ptr, m128);
    }
    void Storeu(float* ptr) const noexcept {
        simde_mm_storeu_ps(ptr, m128);
    }

    // -------------------- convert --------------------
    std::array<float, 4> ToArray() const noexcept {
        std::array<float, 4> arr{};
        simde_mm_storeu_ps(arr.data(), m128);
        return arr;
    }

    inline V4i ToInt() const noexcept;

    // -------------------- math --------------------
    static void Transpose4(V4f& a, V4f& b, V4f& c, V4f& d) noexcept {
        simde__m128 t0 = simde_mm_unpacklo_ps(a.m128, b.m128);
        simde__m128 t1 = simde_mm_unpackhi_ps(a.m128, b.m128);
        simde__m128 t2 = simde_mm_unpacklo_ps(c.m128, d.m128);
        simde__m128 t3 = simde_mm_unpackhi_ps(c.m128, d.m128);
        a.m128 = simde_mm_movelh_ps(t0, t2);
        b.m128 = simde_mm_movehl_ps(t2, t0);
        c.m128 = simde_mm_movelh_ps(t1, t3);
        d.m128 = simde_mm_movehl_ps(t3, t1);
    }
};

static inline V4f operator+(V4f a, V4f b) {
    return simde_mm_add_ps(a.m128, b.m128);
}

static inline V4f operator-(V4f a, V4f b) {
    return simde_mm_sub_ps(a.m128, b.m128);
}

static inline V4f operator*(V4f a, V4f b) {
    return simde_mm_mul_ps(a.m128, b.m128);
}

static inline V4f operator/(V4f a, V4f b) {
    return simde_mm_div_ps(a.m128, b.m128);
}

// ----------------------------------------
// int 128
// ----------------------------------------

struct V4i {
    simde__m128i m128i;

    // -------------------- constructor --------------------
    V4i(simde__m128i m128i) : m128i(m128i) {
    }

    V4i(int32_t a, int32_t b, int32_t c, int32_t d) : m128i(simde_mm_set_epi32(d, c, b, a)) {
    }

    V4i(int32_t v) : m128i(simde_mm_set1_epi32(v)) {
    }

    // -------------------- load/store --------------------
    void Broadcast(int32_t v) noexcept {
        m128i = simde_mm_set1_epi32(v);
    }

    void Load(int32_t const* ptr) noexcept {
        m128i = simde_mm_load_si128(reinterpret_cast<const simde__m128i*>(ptr));
    }

    void Loadu(int32_t const* ptr) noexcept {
        m128i = simde_mm_loadu_si128(reinterpret_cast<const simde__m128i*>(ptr));
    }

    void Store(int32_t* ptr) const noexcept {
        simde_mm_store_si128(reinterpret_cast<simde__m128i*>(ptr), m128i);
    }

    void Storeu(int32_t* ptr) const noexcept {
        simde_mm_storeu_si128(reinterpret_cast<simde__m128i*>(ptr), m128i);
    }

    // -------------------- convert --------------------
    std::array<int32_t, 4> ToArray() const noexcept {
        std::array<int32_t, 4> arr{};
        simde_mm_storeu_si128(reinterpret_cast<simde__m128i*>(arr.data()), m128i);
        return arr;
    }

    V4f ToFloat() const noexcept {
        return simde_mm_cvtepi32_ps(m128i);
    }

    // -------------------- math --------------------
};

static inline V4i operator+(V4i a, V4i b) {
    return simde_mm_add_epi32(a.m128i, b.m128i);
}

static inline V4i operator-(V4i a, V4i b) {
    return simde_mm_sub_epi32(a.m128i, b.m128i);
}

static inline V4i operator*(V4i a, V4i b) {
    return simde_mm_mul_epi32(a.m128i, b.m128i);
}

static inline V4i operator&(V4i a, V4i b) {
    return simde_mm_and_si128(a.m128i, b.m128i);
}

// ----------------------------------------
// convert
// ----------------------------------------

inline V4i V4f::ToInt() const noexcept {
    return simde_mm_cvtepi32_ps(m128);
}

}  // namespace qwqdsp_simd_element
