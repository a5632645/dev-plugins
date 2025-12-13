#pragma once
#include <algorithm>
#include <bit>
#include <cstdint>
#include <cmath>
#include "qwqdsp/force_inline.hpp"

namespace qwqdsp_simd_element {
template<size_t N>  requires (std::has_single_bit(N))
struct PackInt32;

template<size_t N>  requires (std::has_single_bit(N))
struct PackBool {
    static constexpr size_t kSize = N;
    bool data[N];
};

// ----------------------------------------
// float pack
// ----------------------------------------
template<size_t N> requires (std::has_single_bit(N))
struct PackFloat {
    static constexpr size_t kSize = N;
    alignas(4 * N) float data[N];

    // broadcast
    QWQDSP_FORCE_INLINE
    void Broadcast(float v) noexcept {
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) data[i] = v; 
    }
    QWQDSP_FORCE_INLINE
    [[nodiscard]]
    static constexpr PackFloat<N> vBroadcast(float v) noexcept {
        PackFloat<N> r;
        r.Broadcast(v);
        return r;
    }
    // []
    QWQDSP_FORCE_INLINE
    float& operator[](size_t i) noexcept { return data[i]; }
    // [] const
    QWQDSP_FORCE_INLINE
    float operator[](size_t i) const noexcept { return data[i]; }
    // pack ops
    // +=
    QWQDSP_FORCE_INLINE
    PackFloat& operator+=(PackFloat const& b) noexcept {
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) data[i] += b.data[i];
        return *this;
    }
    // -=
    QWQDSP_FORCE_INLINE
    PackFloat& operator-=(PackFloat const& b) noexcept {
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) data[i] -= b.data[i];
        return *this;
    }
    // *=
    QWQDSP_FORCE_INLINE
    PackFloat& operator*=(PackFloat const& b) noexcept {
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) data[i] *= b.data[i];
        return *this;
    }
    // /=
    QWQDSP_FORCE_INLINE
    PackFloat& operator/=(PackFloat const& b) noexcept {
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) data[i] /= b.data[i];
        return *this;
    }
    // pack and scalar ops
    // +=
    QWQDSP_FORCE_INLINE
    PackFloat& operator+=(float b) noexcept {
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) data[i] += b;
        return *this;
    }
    // -=
    QWQDSP_FORCE_INLINE
    PackFloat& operator-=(float b) noexcept {
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) data[i] -= b;
        return *this;
    }
    // *=
    QWQDSP_FORCE_INLINE
    PackFloat& operator*=(float b) noexcept {
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) data[i] *= b;
        return *this;
    }
    // /=
    QWQDSP_FORCE_INLINE
    PackFloat& operator/=(float b) noexcept {
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) data[i] /= b;
        return *this;
    }
    // >
    QWQDSP_FORCE_INLINE
    PackBool<N> operator>(PackFloat const& b) const noexcept {
        PackBool<N> r;
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) r.data[i] = data[i] > b.data[i];
        return r;
    }
    // <
    QWQDSP_FORCE_INLINE
    PackBool<N> operator<(PackFloat const& b) const noexcept {
        PackBool<N> r;
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) r.data[i] = data[i] < b.data[i];
        return r;
    }
    // =
    QWQDSP_FORCE_INLINE
    PackBool<N> operator==(PackFloat const& b) const noexcept {
        PackBool<N> r;
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) r.data[i] = data[i] == b.data[i];
        return r;
    }
    // >=
    QWQDSP_FORCE_INLINE
    PackBool<N> operator>=(PackFloat const& b) const noexcept {
        PackBool<N> r;
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) r.data[i] = data[i] >= b.data[i];
        return r;
    }
    // <=
    QWQDSP_FORCE_INLINE
    PackBool<N> operator<=(PackFloat const& b) const noexcept {
        PackBool<N> r;
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) r.data[i] = data[i] <= b.data[i];
        return r;
    }
    // !=
    QWQDSP_FORCE_INLINE
    PackBool<N> operator!=(PackFloat const& b) const noexcept {
        PackBool<N> r;
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) r.data[i] = data[i] != b.data[i];
        return r;
    }
    // ToInt
    QWQDSP_FORCE_INLINE
    inline PackInt32<N> ToInt() const noexcept;
};

// pack ops
// add
template<size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackFloat<N> operator+(PackFloat<N> const& a, PackFloat<N> const& b) noexcept {
    PackFloat<N> r;
    #pragma clang loop unroll(full)
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] + b.data[i];
    return r;
}
// sub
template<size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackFloat<N> operator-(PackFloat<N> const& a, PackFloat<N> const& b) noexcept {
    PackFloat<N> r;
    #pragma clang loop unroll(full)
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] - b.data[i];
    return r;
}
// mul
template<size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackFloat<N> operator*(PackFloat<N> const& a, PackFloat<N> const& b) noexcept {
    PackFloat<N> r;
    #pragma clang loop unroll(full)
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] * b.data[i];
    return r;
}
// div
template<size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackFloat<N> operator/(PackFloat<N> const& a, PackFloat<N> const& b) noexcept {
    PackFloat<N> r;
    #pragma clang loop unroll(full)
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] / b.data[i];
    return r;
}
// pack with scalar
// add
template<size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackFloat<N> operator+(PackFloat<N> const& a, float b) noexcept {
    PackFloat<N> r;
    #pragma clang loop unroll(full)
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] + b;
    return r;
}
// sub
template<size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackFloat<N> operator-(PackFloat<N> const& a, float b) noexcept {
    PackFloat<N> r;
    #pragma clang loop unroll(full)
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] - b;
    return r;
}
// mul
template<size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackFloat<N> operator*(PackFloat<N> const& a, float b) noexcept {
    PackFloat<N> r;
    #pragma clang loop unroll(full)
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] * b;
    return r;
}
// div
template<size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackFloat<N> operator/(PackFloat<N> const& a, float b) noexcept {
    PackFloat<N> r;
    #pragma clang loop unroll(full)
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] / b;
    return r;
}
// scalar with pack
// add
template<size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackFloat<N> operator+(float a, PackFloat<N> const& b) noexcept {
    PackFloat<N> r;
    #pragma clang loop unroll(full)
    for (size_t i = 0; i < N; ++i) r.data[i] = a + b.data[i];
    return r;
}
// sub
template<size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackFloat<N> operator-(float a, PackFloat<N> const& b) noexcept {
    PackFloat<N> r;
    #pragma clang loop unroll(full)
    for (size_t i = 0; i < N; ++i) r.data[i] = a - b.data[i];
    return r;
}
// mul
template<size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackFloat<N> operator*(float a, PackFloat<N> const& b) noexcept {
    PackFloat<N> r;
    #pragma clang loop unroll(full)
    for (size_t i = 0; i < N; ++i) r.data[i] = a * b.data[i];
    return r;
}
// div
template<size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackFloat<N> operator/(float a, PackFloat<N> const& b) noexcept {
    PackFloat<N> r;
    #pragma clang loop unroll(full)
    for (size_t i = 0; i < N; ++i) r.data[i] = a / b.data[i];
    return r;
}

// ----------------------------------------
// int pack
// ----------------------------------------
template<size_t N> requires (std::has_single_bit(N))
struct PackInt32 {
    static constexpr size_t kSize = N;
    alignas(4 * N) int32_t data[N];

    // broadcast
    QWQDSP_FORCE_INLINE
    void Broadcast(int32_t v) noexcept {
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) data[i] = v;
    }
    [[nodiscard]]
    QWQDSP_FORCE_INLINE
    static constexpr PackInt32<N> vBroadcast(int32_t v) noexcept {
        PackInt32<N> r;
        r.Broadcast(v);
        return r;
    }
    // []
    QWQDSP_FORCE_INLINE
    int32_t& operator[](size_t i) noexcept { return data[i]; }
    // [] const
    QWQDSP_FORCE_INLINE
    int32_t operator[](size_t i) const noexcept { return data[i]; }
    // pack ops
    // +=
    QWQDSP_FORCE_INLINE
    PackInt32& operator+=(PackInt32 const& b) noexcept {
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) data[i] += b.data[i];
        return *this;
    }
    // -=
    QWQDSP_FORCE_INLINE
    PackInt32& operator-=(PackInt32 const& b) noexcept {
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) data[i] -= b.data[i];
        return *this;
    }
    // *=
    QWQDSP_FORCE_INLINE
    PackInt32& operator*=(PackInt32 const& b) noexcept {
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) data[i] *= b.data[i];
        return *this;
    }
    // /=
    QWQDSP_FORCE_INLINE
    PackInt32& operator/=(PackInt32 const& b) noexcept {
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) data[i] /= b.data[i];
        return *this;
    }
    // %=
    QWQDSP_FORCE_INLINE
    PackInt32& operator%=(PackInt32 const& b) noexcept {
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) data[i] %= b.data[i];
        return *this;
    }
    // ^=
    QWQDSP_FORCE_INLINE
    PackInt32& operator^=(PackInt32 const& b) noexcept {
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) data[i] ^= b.data[i];
        return *this;
    }
    // |=
    QWQDSP_FORCE_INLINE
    PackInt32& operator|=(PackInt32 const& b) noexcept {
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) data[i] |= b.data[i];
        return *this;
    }
    // &=
    QWQDSP_FORCE_INLINE
    PackInt32& operator&=(PackInt32 const& b) noexcept {
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) data[i] &= b.data[i];
        return *this;
    }
    // pack and scalar ops
    // +=
    QWQDSP_FORCE_INLINE
    PackInt32& operator+=(int32_t b) noexcept {
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) data[i] += b;
        return *this;
    }
    // -=
    QWQDSP_FORCE_INLINE
    PackInt32& operator-=(int32_t b) noexcept {
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) data[i] -= b;
        return *this;
    }
    // *=
    QWQDSP_FORCE_INLINE
    PackInt32& operator*=(int32_t b) noexcept {
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) data[i] *= b;
        return *this;
    }
    // /=
    QWQDSP_FORCE_INLINE
    PackInt32& operator/=(int32_t b) noexcept {
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) data[i] /= b;
        return *this;
    }
    // %=
    QWQDSP_FORCE_INLINE
    PackInt32& operator%=(int32_t b) noexcept {
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) data[i] %= b;
        return *this;
    }
    // ^=
    QWQDSP_FORCE_INLINE
    PackInt32& operator^=(int32_t b) noexcept {
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) data[i] ^= b;
        return *this;
    }
    // |=
    QWQDSP_FORCE_INLINE
    PackInt32& operator|=(int32_t b) noexcept {
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) data[i] |= b;
        return *this;
    }
    // &=
    QWQDSP_FORCE_INLINE
    PackInt32& operator&=(int32_t b) noexcept {
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) data[i] &= b;
        return *this;
    }
    // ToFloat
    QWQDSP_FORCE_INLINE
    PackFloat<N> ToFloat() const noexcept {
        PackFloat<N> r;
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) r.data[i] = static_cast<float>(data[i]);
        return r;
    }
};

// float.ToInt
template<size_t N> requires (std::has_single_bit(N))
QWQDSP_FORCE_INLINE
inline PackInt32<N> PackFloat<N>::ToInt() const noexcept {
    PackInt32<N> r;
    #pragma clang loop unroll(full)
    for (size_t i = 0; i < N; ++i) r.data[i] = static_cast<int32_t>(data[i]);
    return r;
}

// pack ops
// add
template<size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackInt32<N> operator+(PackInt32<N> const& a, PackInt32<N> const& b) noexcept {
    PackInt32<N> r;
    #pragma clang loop unroll(full)
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] + b.data[i];
    return r;
}
// sub
template<size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackInt32<N> operator-(PackInt32<N> const& a, PackInt32<N> const& b) noexcept {
    PackInt32<N> r;
    #pragma clang loop unroll(full)
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] - b.data[i];
    return r;
}
// mul
template<size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackInt32<N> operator*(PackInt32<N> const& a, PackInt32<N> const& b) noexcept {
    PackInt32<N> r;
    #pragma clang loop unroll(full)
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] * b.data[i];
    return r;
}
// div
template<size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackInt32<N> operator/(PackInt32<N> const& a, PackInt32<N> const& b) noexcept {
    PackInt32<N> r;
    #pragma clang loop unroll(full)
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] / b.data[i];
    return r;
}
// mod
template<size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackInt32<N> operator%(PackInt32<N> const& a, PackInt32<N> const& b) noexcept {
    PackInt32<N> r;
    #pragma clang loop unroll(full)
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] % b.data[i];
    return r;
}
// and
template<size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackInt32<N> operator&(PackInt32<N> const& a, PackInt32<N> const& b) noexcept {
    PackInt32<N> r;
    #pragma clang loop unroll(full)
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] & b.data[i];
    return r;
}
// or
template<size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackInt32<N> operator|(PackInt32<N> const& a, PackInt32<N> const& b) noexcept {
    PackInt32<N> r;
    #pragma clang loop unroll(full)
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] | b.data[i];
    return r;
}
// xor
template<size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackInt32<N> operator^(PackInt32<N> const& a, PackInt32<N> const& b) noexcept {
    PackInt32<N> r;
    #pragma clang loop unroll(full)
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] ^ b.data[i];
    return r;
}
// pack and scalar ops
// add
template<size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackInt32<N> operator+(PackInt32<N> const& a, int32_t b) noexcept {
    PackInt32<N> r;
    #pragma clang loop unroll(full)
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] + b;
    return r;
}
// sub
template<size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackInt32<N> operator-(PackInt32<N> const& a, int32_t b) noexcept {
    PackInt32<N> r;
    #pragma clang loop unroll(full)
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] - b;
    return r;
}
// mul
template<size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackInt32<N> operator*(PackInt32<N> const& a, int32_t b) noexcept {
    PackInt32<N> r;
    #pragma clang loop unroll(full)
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] * b;
    return r;
}
// div
template<size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackInt32<N> operator/(PackInt32<N> const& a, int32_t b) noexcept {
    PackInt32<N> r;
    #pragma clang loop unroll(full)
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] / b;
    return r;
}
// mod
template<size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackInt32<N> operator%(PackInt32<N> const& a, int32_t b) noexcept {
    PackInt32<N> r;
    #pragma clang loop unroll(full)
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] % b;
    return r;
}
// and
template<size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackInt32<N> operator&(PackInt32<N> const& a, int32_t b) noexcept {
    PackInt32<N> r;
    #pragma clang loop unroll(full)
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] & b;
    return r;
}
// or
template<size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackInt32<N> operator|(PackInt32<N> const& a, int32_t b) noexcept {
    PackInt32<N> r;
    #pragma clang loop unroll(full)
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] | b;
    return r;
}
// xor
template<size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackInt32<N> operator^(PackInt32<N> const& a, int32_t b) noexcept {
    PackInt32<N> r;
    #pragma clang loop unroll(full)
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] ^ b;
    return r;
}
// scalar and pack ops
// add
template<size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackInt32<N> operator+(int32_t a, PackInt32<N> const& b) noexcept {
    PackInt32<N> r;
    #pragma clang loop unroll(full)
    for (size_t i = 0; i < N; ++i) r.data[i] = a + b.data[i];
    return r;
}
// sub
template<size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackInt32<N> operator-(int32_t a, PackInt32<N> const& b) noexcept {
    PackInt32<N> r;
    #pragma clang loop unroll(full)
    for (size_t i = 0; i < N; ++i) r.data[i] = a - b.data[i];
    return r;
}
// mul
template<size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackInt32<N> operator*(int32_t a, PackInt32<N> const& b) noexcept {
    PackInt32<N> r;
    #pragma clang loop unroll(full)
    for (size_t i = 0; i < N; ++i) r.data[i] = a * b.data[i];
    return r;
}
// div
template<size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackInt32<N> operator/(int32_t a, PackInt32<N> const& b) noexcept {
    PackInt32<N> r;
    #pragma clang loop unroll(full)
    for (size_t i = 0; i < N; ++i) r.data[i] = a / b.data[i];
    return r;
}
// mod
template<size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackInt32<N> operator%(int32_t a, PackInt32<N> const& b) noexcept {
    PackInt32<N> r;
    #pragma clang loop unroll(full)
    for (size_t i = 0; i < N; ++i) r.data[i] = a % b.data[i];
    return r;
}
// and
template<size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackInt32<N> operator&(int32_t a, PackInt32<N> const& b) noexcept {
    PackInt32<N> r;
    #pragma clang loop unroll(full)
    for (size_t i = 0; i < N; ++i) r.data[i] = a & b.data[i];
    return r;
}
// or
template<size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackInt32<N> operator|(int32_t a, PackInt32<N> const& b) noexcept {
    PackInt32<N> r;
    #pragma clang loop unroll(full)
    for (size_t i = 0; i < N; ++i) r.data[i] = a | b.data[i];
    return r;
}
// xor
template<size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackInt32<N> operator^(int32_t a, PackInt32<N> const& b) noexcept {
    PackInt32<N> r;
    #pragma clang loop unroll(full)
    for (size_t i = 0; i < N; ++i) r.data[i] = a ^ b.data[i];
    return r;
}

// ----------------------------------------
// packed ops
// ----------------------------------------
struct PackOps {
    #ifdef QWQDSP_HAS_SSE41
    static constexpr bool kHasSSE4 = true;
    #else
    #if _MSC_VER
        #if defined(__AVX__) || defined(__AVX2__)
            static constexpr bool kHasSSE4 = true;
        #elif defined(__SSE4_1__)
            static constexpr bool kHasSSE4 = true;
        #else
            static constexpr bool kHasSSE4 = false;
    #endif
    #else
        #ifdef __SSE4_1__
            static constexpr bool kHasSSE4 = true;
        #else
            static constexpr bool kHasSSE4 = false;
        #endif
    #endif
    #endif

    // float.floor
    template<size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackFloat<N> Floor(PackFloat<N> const& a) noexcept {
        PackFloat<N> r;
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) r.data[i] = std::floor(a.data[i]);
        return r;
    }
    // float.fma
    template<size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackFloat<N> Fma(PackFloat<N> const& a, PackFloat<N> const& b, PackFloat<N> const& c) noexcept {
        PackFloat<N> r;
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] * b.data[i] + c.data[i];
        return r;
    }
    // float.broadcast
    template<size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackFloat<N> Broadcast(float v) noexcept {
        PackFloat<N> r;
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) r.data[i] = v;
        return r;
    }
    // float.min
    template<size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackFloat<N> Min(PackFloat<N> const& a, PackFloat<N> const& b) noexcept {
        PackFloat<N> r;
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) r.data[i] = std::min(a.data[i], b.data[i]);
        return r;
    }
    // float.max
    template<size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackFloat<N> Max(PackFloat<N> const& a, PackFloat<N> const& b) noexcept {
        PackFloat<N> r;
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) r.data[i] = std::max(a.data[i], b.data[i]);
        return r;
    }
    // float.clamp
    template<size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackFloat<N> Clamp(PackFloat<N> const& a, PackFloat<N> const& min, PackFloat<N> const& max) noexcept {
        PackFloat<N> r;
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) r.data[i] = std::clamp(a.data[i], min.data[i], max.data[i]);
        return r;
    }
    // float.shuffle
    // for 4
    template<size_t a, size_t b, size_t c, size_t d>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackFloat<4> Shuffle(PackFloat<4> const& reg) noexcept {
        return PackFloat<4> {
            reg.data[a],
            reg.data[b],
            reg.data[c],
            reg.data[d]
        };
    }
    // for 8
    template<size_t a, size_t b, size_t c, size_t d, size_t e, size_t f, size_t g, size_t h>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackFloat<8> Shuffle(PackFloat<8> const& reg) noexcept {
        return PackFloat<8> {
            reg.data[a],
            reg.data[b],
            reg.data[c],
            reg.data[d],
            reg.data[e],
            reg.data[f],
            reg.data[g],
            reg.data[h]
        };
    }
    // for 16
    template<size_t a, size_t b, size_t c, size_t d, size_t e, size_t f, size_t g, size_t h, size_t i, size_t j, size_t k, size_t l, size_t m, size_t n, size_t o, size_t p>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackFloat<16> Shuffle(PackFloat<16> const& reg) noexcept {
        return PackFloat<16> {
            reg.data[a],
            reg.data[b],
            reg.data[c],
            reg.data[d],
            reg.data[e],
            reg.data[f],
            reg.data[g],
            reg.data[h],
            reg.data[i],
            reg.data[j],
            reg.data[k],
            reg.data[l],
            reg.data[m],
            reg.data[n],
            reg.data[o],
            reg.data[p]
        };
    }
    // float.blend
    // for 4
    template<size_t ab0, size_t ab1, size_t ab2, size_t ab3>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackFloat<4> Blend(PackFloat<4> const& zero_reg, PackFloat<4> const& one_reg) noexcept {
        return PackFloat<4> {
            ab0 == 0 ? zero_reg.data[0] : one_reg.data[0],
            ab1 == 1 ? zero_reg.data[1] : one_reg.data[1],
            ab2 == 2 ? zero_reg.data[2] : one_reg.data[2],
            ab3 == 3 ? zero_reg.data[3] : one_reg.data[3]
        };
    }
    // for 8
    template<size_t ab0, size_t ab1, size_t ab2, size_t ab3, size_t ab4, size_t ab5, size_t ab6, size_t ab7>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackFloat<8> Blend(PackFloat<8> const& zero_reg, PackFloat<8> const& one_reg) noexcept {
        return PackFloat<8> {
            ab0 == 0 ? zero_reg.data[0] : one_reg.data[0],
            ab1 == 1 ? zero_reg.data[1] : one_reg.data[1],
            ab2 == 2 ? zero_reg.data[2] : one_reg.data[2],
            ab3 == 3 ? zero_reg.data[3] : one_reg.data[3],
            ab4 == 4 ? zero_reg.data[4] : one_reg.data[4],
            ab5 == 5 ? zero_reg.data[5] : one_reg.data[5],
            ab6 == 6 ? zero_reg.data[6] : one_reg.data[6],
            ab7 == 7 ? zero_reg.data[7] : one_reg.data[7]
        };
    }
    // for 16
    template<size_t ab0, size_t ab1, size_t ab2, size_t ab3, size_t ab4, size_t ab5, size_t ab6, size_t ab7,
             size_t ab8, size_t ab9, size_t ab10, size_t ab11, size_t ab12, size_t ab13, size_t ab14, size_t ab15>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackFloat<16> Blend(PackFloat<16> const& zero_reg, PackFloat<16> const& one_reg) noexcept {
        return PackFloat<16> {
            ab0 == 0 ? zero_reg.data[0] : one_reg.data[0],
            ab1 == 1 ? zero_reg.data[1] : one_reg.data[1],
            ab2 == 2 ? zero_reg.data[2] : one_reg.data[2],
            ab3 == 3 ? zero_reg.data[3] : one_reg.data[3],
            ab4 == 4 ? zero_reg.data[4] : one_reg.data[4],
            ab5 == 5 ? zero_reg.data[5] : one_reg.data[5],
            ab6 == 6 ? zero_reg.data[6] : one_reg.data[6],
            ab7 == 7 ? zero_reg.data[7] : one_reg.data[7],
            ab8 == 8 ? zero_reg.data[8] : one_reg.data[8],
            ab9 == 9 ? zero_reg.data[9] : one_reg.data[9],
            ab10 == 10 ? zero_reg.data[10] : one_reg.data[10],
            ab11 == 11 ? zero_reg.data[11] : one_reg.data[11],
            ab12 == 12 ? zero_reg.data[12] : one_reg.data[12],
            ab13 == 13 ? zero_reg.data[13] : one_reg.data[13],
            ab14 == 14 ? zero_reg.data[14] : one_reg.data[14],
            ab15 == 15 ? zero_reg.data[15] : one_reg.data[15]
        };
    }
    // float.toInt
    template<size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackInt32<N> ToInt(PackFloat<N> const& x) noexcept {
        PackInt32<N> r;
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) r.data[i] = static_cast<int>(x.data[i]);
        return r;
    }
    // float.positiveFrac
    template<size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackFloat<N> PositiveFrac(PackFloat<N> const& x) noexcept {
        if constexpr (N == 4 && !kHasSSE4) {
            return {
                x[0] - static_cast<int>(x[0]),
                x[1] - static_cast<int>(x[1]),
                x[2] - static_cast<int>(x[2]),
                x[3] - static_cast<int>(x[3])
            };
        }
        else {
            PackFloat<N> r;
            #pragma clang loop unroll(full)
            for (size_t i = 0; i < N; ++i) r.data[i] = x.data[i] - std::floor(x.data[i]);
            return r;
        }
    }
    // float.frac
    template<size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackFloat<N> Frac(PackFloat<N> const& x) noexcept {
        if constexpr (N == 4 && !kHasSSE4) {
            auto r = PositiveFrac(x);
            PackFloat<N> fix{
                x[0] < 0 ? 1.0f : 0.0f,
                x[1] < 0 ? 1.0f : 0.0f,
                x[2] < 0 ? 1.0f : 0.0f,
                x[3] < 0 ? 1.0f : 0.0f
            };
            r += fix;
            return r;
        }
        else {
            PackFloat<N> r;
            #pragma clang loop unroll(full)
            for (size_t i = 0; i < N; ++i) r.data[i] = x.data[i] - std::floor(x.data[i]);
            return r;
        }
    }
    // float.reduceAdd
    template<size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr float ReduceAdd(PackFloat<N> const& x) noexcept {
        float sum = 0.0f;
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) sum += x.data[i];
        return sum;
    }
    // float.sqrt
    template<size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackFloat<N> Sqrt(PackFloat<N> const& x) noexcept {
        PackFloat<N> r;
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) r.data[i] = std::sqrt(x.data[i]);
        return r;
    }
    // float.abs
    template<size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackFloat<N> Abs(PackFloat<N> const& x) noexcept {
        PackFloat<N> r;
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) r.data[i] = std::abs(x.data[i]);
        return r;
    }
    // float.tan
    template<size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackFloat<N> Tan(PackFloat<N> const& x) noexcept {
        PackFloat<N> r;
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) r.data[i] = std::tan(x.data[i]);
        return r;
    }
    // float.atan
    template<size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackFloat<N> Atan(PackFloat<N> const& x) noexcept {
        PackFloat<N> r;
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) r.data[i] = std::atan(x.data[i]);
        return r;
    }
    // float.asinh
    template<size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackFloat<N> Asinh(PackFloat<N> const& x) noexcept {
        PackFloat<N> r;
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) r.data[i] = std::asinh(x.data[i]);
        return r;
    }
    // float.cosh
    template<size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackFloat<N> Cosh(PackFloat<N> const& x) noexcept {
        PackFloat<N> r;
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) r.data[i] = std::cosh(x.data[i]);
        return r;
    }
    // float.log
    template<size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackFloat<N> Log(PackFloat<N> const& x) noexcept {
        PackFloat<N> r;
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) r.data[i] = std::log(x.data[i]);
        return r;
    }
    // float.log10
    template<size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackFloat<N> Log10(PackFloat<N> const& x) noexcept {
        PackFloat<N> r;
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) r.data[i] = std::log10(x.data[i]);
        return r;
    }
    // float.exp2
    template<size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackFloat<N> Exp2(PackFloat<N> const& x) noexcept {
        PackFloat<N> r;
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) r.data[i] = std::exp2(x.data[i]);
        return r;
    }
    // float.exp
    template<size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackFloat<N> Exp(PackFloat<N> const& x) noexcept {
        PackFloat<N> r;
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) r.data[i] = std::exp(x.data[i]);
        return r;
    }
    // float.pow
    template<size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackFloat<N> Pow(PackFloat<N> const& x, PackFloat<N> const& y) noexcept {
        PackFloat<N> r;
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) r.data[i] = std::pow(x.data[i], y.data[i]);
        return r;
    }
    // select
    template<size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackFloat<N> Select(PackBool<N> const& mask, PackFloat<N> const& true_val, PackFloat<N> const& false_val) noexcept {
        PackFloat<N> r;
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) r.data[i] = mask.data[i] ? true_val.data[i] : false_val.data[i];
        return r;
    }
    template<size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackFloat<N> Select(PackBool<N> const& mask, float true_val, float false_val) noexcept {
        PackFloat<N> r;
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) r.data[i] = mask.data[i] ? true_val : false_val;
        return r;
    }
    // lerp
    template<size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackFloat<N> Lerp(PackFloat<N> const& x, PackFloat<N> const& y, PackFloat<N> const& t) noexcept {
        PackFloat<N> r;
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) r.data[i] = x.data[i] + (y.data[i] - x.data[i]) * t.data[i];
        return r;
    }
    // lerp scalar
    template<size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackFloat<N> Lerp(PackFloat<N> const& x, PackFloat<N> const& y, float t) noexcept {
        PackFloat<N> r;
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) r.data[i] = x.data[i] + (y.data[i] - x.data[i]) * t;
        return r;
    }
    // float.Cos
    template<size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackFloat<N> Cos(PackFloat<N> const& x) noexcept {
        PackFloat<N> r;
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) r.data[i] = std::cos(x.data[i]);
        return r;
    }
    // float.Sin
    template<size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackFloat<N> Sin(PackFloat<N> const& x) noexcept {
        PackFloat<N> r;
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) r.data[i] = std::sin(x.data[i]);
        return r;
    }
    // float.X2
    template<size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackFloat<N> X2(PackFloat<N> const& x) noexcept {
        return x * x;
    }

    // -------------------- pack int32 --------------------
    // int.toFloat
    template<size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackFloat<N> ToFloat(PackInt32<N> const& x) noexcept {
        PackFloat<N> r;
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) r.data[i] = static_cast<float>(x.data[i]);
        return r;
    }
    // int.Min
    template<size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackInt32<N> Min(PackInt32<N> const& a, PackInt32<N> const& b) noexcept {
        PackInt32<N> r;
        #pragma clang loop unroll(full)
        for (size_t i = 0; i < N; ++i) r.data[i] = std::min(a.data[i], b.data[i]);
        return r;
    }
};

template<size_t N>
using PackFloatCRef = PackFloat<N> const&;

template<size_t N>
using PackInt32CRef = PackInt32<N>&;
}
