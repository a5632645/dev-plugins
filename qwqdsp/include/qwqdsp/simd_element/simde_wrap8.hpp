#pragma once

#include <x86/avx2.h>

#include <array>

namespace qwqdsp_simd_element {

struct V8i;

// ----------------------------------------
// float 256
// ----------------------------------------

struct V8f {
    simde__m256 m128;

    // -------------------- constructor --------------------
    V8f(simde__m256 m128) : m128(m128) {
    }

    V8f(float a, float b, float c, float d, float e, float f, float g, float h) : m128(simde_mm256_set_ps(h, g, f, e, d, c, b, a)) {
    }

    V8f(float v) : m128(simde_mm256_set1_ps(v)) {
    }

    // -------------------- load/store --------------------
    void Broadcast(float v) noexcept {
        m128 = simde_mm256_set1_ps(v);
    }

    void Load(float const* ptr) noexcept {
        m128 = simde_mm256_load_ps(ptr);
    }
    void Loadu(float const* ptr) noexcept {
        m128 = simde_mm256_loadu_ps(ptr);
    }

    void Store(float* ptr) const noexcept {
        simde_mm256_store_ps(ptr, m128);
    }
    void Storeu(float* ptr) const noexcept {
        simde_mm256_storeu_ps(ptr, m128);
    }

    // -------------------- convert --------------------
    std::array<float, 8> ToArray() const noexcept {
        std::array<float, 8> arr{};
        simde_mm256_storeu_ps(arr.data(), m128);
        return arr;
    }

    inline V8i ToInt() const noexcept;

    // -------------------- math --------------------
};

static inline V8f operator+(V8f a, V8f b) {
    return simde_mm256_add_ps(a.m128, b.m128);
}

static inline V8f operator-(V8f a, V8f b) {
    return simde_mm256_sub_ps(a.m128, b.m128);
}

static inline V8f operator*(V8f a, V8f b) {
    return simde_mm256_mul_ps(a.m128, b.m128);
}

static inline V8f operator/(V8f a, V8f b) {
    return simde_mm256_div_ps(a.m128, b.m128);
}

// ----------------------------------------
// int 128
// ----------------------------------------

struct V8i {
    simde__m256i m128i;

    // -------------------- constructor --------------------
    V8i(simde__m256i m128i) : m128i(m128i) {
    }

    V8i(int32_t a, int32_t b, int32_t c, int32_t d, int32_t e, int32_t f, int32_t g, int32_t h) : m128i(simde_mm256_set_epi32(h, g, f, e, d, c, b, a)) {
    }

    V8i(int32_t v) : m128i(simde_mm256_set1_epi32(v)) {
    }

    // -------------------- load/store --------------------
    void Broadcast(int32_t v) noexcept {
        m128i = simde_mm256_set1_epi32(v);
    }

    void Load(int32_t const* ptr) noexcept {
        m128i = simde_mm256_load_si256(reinterpret_cast<const simde__m256i*>(ptr));
    }

    void Loadu(int32_t const* ptr) noexcept {
        m128i = simde_mm256_loadu_si256(reinterpret_cast<const simde__m256i*>(ptr));
    }

    void Store(int32_t* ptr) const noexcept {
        simde_mm256_store_si256(reinterpret_cast<simde__m256i*>(ptr), m128i);
    }

    void Storeu(int32_t* ptr) const noexcept {
        simde_mm256_storeu_si256(reinterpret_cast<simde__m256i*>(ptr), m128i);
    }

    // -------------------- convert --------------------
    std::array<int32_t, 8> ToArray() const noexcept {
        std::array<int32_t, 8> arr{};
        simde_mm256_storeu_si256(reinterpret_cast<simde__m256i*>(arr.data()), m128i);
        return arr;
    }

    V8f ToFloat() const noexcept {
        return simde_mm256_cvtepi32_ps(m128i);
    }

    // -------------------- math --------------------
};

static inline V8i operator+(V8i a, V8i b) {
    return simde_mm256_add_epi32(a.m128i, b.m128i);
}

static inline V8i operator-(V8i a, V8i b) {
    return simde_mm256_sub_epi32(a.m128i, b.m128i);
}

static inline V8i operator*(V8i a, V8i b) {
    return simde_mm256_mullo_epi32(a.m128i, b.m128i);
}

static inline V8i operator&(V8i a, V8i b) {
    return simde_mm256_and_si256(a.m128i, b.m128i);
}

// ----------------------------------------
// convert
// ----------------------------------------

inline V8i V8f::ToInt() const noexcept {
    return simde_mm256_cvttps_epi32(m128);
}

}  // namespace qwqdsp_simd_element
