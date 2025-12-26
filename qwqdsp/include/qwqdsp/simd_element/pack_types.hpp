#pragma once
#include <algorithm>
#include <bit>
#include <cstdint>
#include <cmath>
#include "qwqdsp/extension_marcos.hpp"

namespace qwqdsp_simd_element {

template<class T, size_t N>
    requires (std::has_single_bit(N))
struct PackTypes {
    alignas(sizeof(T) * N) T data[N];

    QWQDSP_FORCE_INLINE
    void Broadcast(T v) noexcept {
        QWQDSP_AUTO_VECTORLIZE
        for (size_t i = 0; i < N; ++i) data[i] = v; 
    }

    QWQDSP_FORCE_INLINE
    [[nodiscard]]
    static constexpr PackTypes<T, N> vBroadcast(T v) noexcept {
        PackTypes<T, N> r;
        r.Broadcast(v);
        return r;
    }

    // []
    QWQDSP_FORCE_INLINE
    T& operator[](size_t i) noexcept { return data[i]; }

    // [] const
    QWQDSP_FORCE_INLINE
    T operator[](size_t i) const noexcept { return data[i]; }

    // covert to
    template<class OtherType>
    QWQDSP_FORCE_INLINE
    PackTypes<OtherType, N> To() const noexcept {
        PackTypes<OtherType, N> r;
        QWQDSP_AUTO_VECTORLIZE
        for (size_t i = 0; i < N; ++i) r.data[i] = static_cast<OtherType>(data[i]);
        return r;
    }

    // -------------------- all operations with pack --------------------
    // +=
    QWQDSP_FORCE_INLINE
    PackTypes<T, N>& operator+=(PackTypes<T, N> const& b) noexcept {
        QWQDSP_AUTO_VECTORLIZE
        for (size_t i = 0; i < N; ++i) data[i] += b.data[i];
        return *this;
    }
    // -=
    QWQDSP_FORCE_INLINE
    PackTypes<T, N>& operator-=(PackTypes<T, N> const& b) noexcept {
        QWQDSP_AUTO_VECTORLIZE
        for (size_t i = 0; i < N; ++i) data[i] -= b.data[i];
        return *this;
    }
    // *=
    QWQDSP_FORCE_INLINE
    PackTypes<T, N>& operator*=(PackTypes<T, N> const& b) noexcept {
        QWQDSP_AUTO_VECTORLIZE
        for (size_t i = 0; i < N; ++i) data[i] *= b.data[i];
        return *this;
    }
    // /=
    QWQDSP_FORCE_INLINE
    PackTypes<T, N>& operator/=(PackTypes<T, N> const& b) noexcept {
        QWQDSP_AUTO_VECTORLIZE
        for (size_t i = 0; i < N; ++i) data[i] /= b.data[i];
        return *this;
    }
    // &=
    QWQDSP_FORCE_INLINE
    PackTypes<T, N>& operator&=(PackTypes<T, N> const& b) noexcept {
        QWQDSP_AUTO_VECTORLIZE
        for (size_t i = 0; i < N; ++i) data[i] &= b.data[i];
        return *this;
    }
    // |=
    QWQDSP_FORCE_INLINE
    PackTypes<T, N>& operator|=(PackTypes<T, N> const& b) noexcept {
        QWQDSP_AUTO_VECTORLIZE
        for (size_t i = 0; i < N; ++i) data[i] |= b.data[i];
        return *this;
    }
    // ^=
    QWQDSP_FORCE_INLINE
    PackTypes<T, N>& operator^=(PackTypes<T, N> const& b) noexcept {
        QWQDSP_AUTO_VECTORLIZE
        for (size_t i = 0; i < N; ++i) data[i] ^= b.data[i];
        return *this;
    }
    // %=
    QWQDSP_FORCE_INLINE
    PackTypes<T, N>& operator%=(PackTypes<T, N> const& b) noexcept {
        QWQDSP_AUTO_VECTORLIZE
        for (size_t i = 0; i < N; ++i) data[i] %= b.data[i];
        return *this;
    }
    // ~
    QWQDSP_FORCE_INLINE
    PackTypes<T, N> operator~() const noexcept {
        PackTypes<T, N> r;
        QWQDSP_AUTO_VECTORLIZE
        for (size_t i = 0; i < N; ++i) r.data[i] = ~data[i];
        return r;
    }

    // -------------------- all operations with scalar --------------------
    // +=
    QWQDSP_FORCE_INLINE
    PackTypes<T, N>& operator+=(T b) noexcept {
        QWQDSP_AUTO_VECTORLIZE
        for (size_t i = 0; i < N; ++i) data[i] += b;
        return *this;
    }
    // -=
    QWQDSP_FORCE_INLINE
    PackTypes<T, N>& operator-=(T b) noexcept {
        QWQDSP_AUTO_VECTORLIZE
        for (size_t i = 0; i < N; ++i) data[i] -= b;
        return *this;
    }
    // *=
    QWQDSP_FORCE_INLINE
    PackTypes<T, N>& operator*=(T b) noexcept {
        QWQDSP_AUTO_VECTORLIZE
        for (size_t i = 0; i < N; ++i) data[i] *= b;
        return *this;
    }
    // /=
    QWQDSP_FORCE_INLINE
    PackTypes<T, N>& operator/=(T b) noexcept {
        QWQDSP_AUTO_VECTORLIZE
        for (size_t i = 0; i < N; ++i) data[i] /= b;
        return *this;
    }
    // &=
    QWQDSP_FORCE_INLINE
    PackTypes<T, N>& operator&=(T b) noexcept {
        QWQDSP_AUTO_VECTORLIZE
        for (size_t i = 0; i < N; ++i) data[i] &= b;
        return *this;
    }
    // |=
    QWQDSP_FORCE_INLINE
    PackTypes<T, N>& operator|=(T b) noexcept {
        QWQDSP_AUTO_VECTORLIZE
        for (size_t i = 0; i < N; ++i) data[i] |= b;
        return *this;
    }
    // ^=
    QWQDSP_FORCE_INLINE
    PackTypes<T, N>& operator^=(T b) noexcept {
        QWQDSP_AUTO_VECTORLIZE
        for (size_t i = 0; i < N; ++i) data[i] ^= b;
        return *this;
    }
    // %=
    QWQDSP_FORCE_INLINE
    PackTypes<T, N>& operator%=(T b) noexcept {
        QWQDSP_AUTO_VECTORLIZE
        for (size_t i = 0; i < N; ++i) data[i] %= b;
        return *this;
    }
};

// -------------------- all operations with pack --------------------
// +
template<class T, size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackTypes<T, N> operator+(PackTypes<T, N> const& a, PackTypes<T, N> const& b) noexcept {
    PackTypes<T, N> r;
    QWQDSP_AUTO_VECTORLIZE
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] + b.data[i];
    return r;
}
// -
template<class T, size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackTypes<T, N> operator-(PackTypes<T, N> const& a, PackTypes<T, N> const& b) noexcept {
    PackTypes<T, N> r;
    QWQDSP_AUTO_VECTORLIZE
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] - b.data[i];
    return r;
}
// *
template<class T, size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackTypes<T, N> operator*(PackTypes<T, N> const& a, PackTypes<T, N> const& b) noexcept {
    PackTypes<T, N> r;
    QWQDSP_AUTO_VECTORLIZE
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] * b.data[i];
    return r;
}
// /
template<class T, size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackTypes<T, N> operator/(PackTypes<T, N> const& a, PackTypes<T, N> const& b) noexcept {
    PackTypes<T, N> r;
    QWQDSP_AUTO_VECTORLIZE
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] / b.data[i];
    return r;
}
// %
template<class T, size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackTypes<T, N> operator%(PackTypes<T, N> const& a, PackTypes<T, N> const& b) noexcept {
    PackTypes<T, N> r;
    QWQDSP_AUTO_VECTORLIZE
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] % b.data[i];
    return r;
}
// &
template<class T, size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackTypes<T, N> operator&(PackTypes<T, N> const& a, PackTypes<T, N> const& b) noexcept {
    PackTypes<T, N> r;
    QWQDSP_AUTO_VECTORLIZE
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] & b.data[i];
    return r;
}
// |
template<class T, size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackTypes<T, N> operator|(PackTypes<T, N> const& a, PackTypes<T, N> const& b) noexcept {
    PackTypes<T, N> r;
    QWQDSP_AUTO_VECTORLIZE
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] | b.data[i];
    return r;
}
// ^
template<class T, size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackTypes<T, N> operator^(PackTypes<T, N> const& a, PackTypes<T, N> const& b) noexcept {
    PackTypes<T, N> r;
    QWQDSP_AUTO_VECTORLIZE
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] ^ b.data[i];
    return r;
}

// -------------------- all operations with scalar --------------------
// +
template<class T, size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackTypes<T, N> operator+(PackTypes<T, N> const& a, T b) noexcept {
    PackTypes<T, N> r;
    QWQDSP_AUTO_VECTORLIZE
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] + b;
    return r;
}
// -
template<class T, size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackTypes<T, N> operator-(PackTypes<T, N> const& a, T b) noexcept {
    PackTypes<T, N> r;
    QWQDSP_AUTO_VECTORLIZE
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] - b;
    return r;
}
// *
template<class T, size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackTypes<T, N> operator*(PackTypes<T, N> const& a, T b) noexcept {
    PackTypes<T, N> r;
    QWQDSP_AUTO_VECTORLIZE
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] * b;
    return r;
}
// /
template<class T, size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackTypes<T, N> operator/(PackTypes<T, N> const& a, T b) noexcept {
    PackTypes<T, N> r;
    QWQDSP_AUTO_VECTORLIZE
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] / b;
    return r;
}
// %
template<class T, size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackTypes<T, N> operator%(PackTypes<T, N> const& a, T b) noexcept {
    PackTypes<T, N> r;
    QWQDSP_AUTO_VECTORLIZE
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] % b;
    return r;
}
// &
template<class T, size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackTypes<T, N> operator&(PackTypes<T, N> const& a, T b) noexcept {
    PackTypes<T, N> r;
    QWQDSP_AUTO_VECTORLIZE
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] & b;
    return r;
}
// |
template<class T, size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackTypes<T, N> operator|(PackTypes<T, N> const& a, T b) noexcept {
    PackTypes<T, N> r;
    QWQDSP_AUTO_VECTORLIZE
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] | b;
    return r;
}
// ^
template<class T, size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackTypes<T, N> operator^(PackTypes<T, N> const& a, T b) noexcept {
    PackTypes<T, N> r;
    QWQDSP_AUTO_VECTORLIZE
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] ^ b;
    return r;
}

// -------------------- all operations with scalar as left hand --------------------
// +
template<class T, size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackTypes<T, N> operator+(T a, PackTypes<T, N> const& b) noexcept {
    PackTypes<T, N> r;
    QWQDSP_AUTO_VECTORLIZE
    for (size_t i = 0; i < N; ++i) r.data[i] = a + b.data[i];
    return r;
}
// -
template<class T, size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackTypes<T, N> operator-(T a, PackTypes<T, N> const& b) noexcept {
    PackTypes<T, N> r;
    QWQDSP_AUTO_VECTORLIZE
    for (size_t i = 0; i < N; ++i) r.data[i] = a - b.data[i];
    return r;
}
// *
template<class T, size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackTypes<T, N> operator*(T a, PackTypes<T, N> const& b) noexcept {
    PackTypes<T, N> r;
    QWQDSP_AUTO_VECTORLIZE
    for (size_t i = 0; i < N; ++i) r.data[i] = a * b.data[i];
    return r;
}
// /
template<class T, size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackTypes<T, N> operator/(T a, PackTypes<T, N> const& b) noexcept {
    PackTypes<T, N> r;
    QWQDSP_AUTO_VECTORLIZE
    for (size_t i = 0; i < N; ++i) r.data[i] = a / b.data[i];
    return r;
}
// %
template<class T, size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackTypes<T, N> operator%(T a, PackTypes<T, N> const& b) noexcept {
    PackTypes<T, N> r;
    QWQDSP_AUTO_VECTORLIZE
    for (size_t i = 0; i < N; ++i) r.data[i] = a % b.data[i];
    return r;
}
// &
template<class T, size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackTypes<T, N> operator&(T a, PackTypes<T, N> const& b) noexcept {
    PackTypes<T, N> r;
    QWQDSP_AUTO_VECTORLIZE
    for (size_t i = 0; i < N; ++i) r.data[i] = a & b.data[i];
    return r;
}
// |
template<class T, size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackTypes<T, N> operator|(T a, PackTypes<T, N> const& b) noexcept {
    PackTypes<T, N> r;
    QWQDSP_AUTO_VECTORLIZE
    for (size_t i = 0; i < N; ++i) r.data[i] = a | b.data[i];
    return r;
}
// ^
template<class T, size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackTypes<T, N> operator^(T a, PackTypes<T, N> const& b) noexcept {
    PackTypes<T, N> r;
    QWQDSP_AUTO_VECTORLIZE
    for (size_t i = 0; i < N; ++i) r.data[i] = a ^ b.data[i];
    return r;
}

// -------------------- all compare operations --------------------
namespace internal {
template<class T>
struct MakeMaskType {
    using type = std::make_unsigned<T>;
    static constexpr type kTrue = std::numeric_limits<type>::max();
    static constexpr type kFalse = 0;
};
template<>
struct MakeMaskType<float> {
    using type = uint32_t;
    static constexpr type kTrue = std::numeric_limits<type>::max();
    static constexpr type kFalse = 0;
};
template<>
struct MakeMaskType<double> {
    using type = uint64_t;
    static constexpr type kTrue = std::numeric_limits<type>::max();
    static constexpr type kFalse = 0;
};
}

template<class T, size_t N>
using PackTypesMask = PackTypes<typename internal::MakeMaskType<T>::type, N>;;

// >
template<class T, size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackTypesMask<T, N> operator>(PackTypes<T, N> const& a, PackTypes<T, N> const& b) noexcept {
    PackTypesMask<T, N> r;
    QWQDSP_AUTO_VECTORLIZE
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] > b.data[i] ? internal::MakeMaskType<T>::kTrue : internal::MakeMaskType<T>::kFalse;
    return r;
}
// <
template<class T, size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackTypesMask<T, N> operator<(PackTypes<T, N> const& a, PackTypes<T, N> const& b) noexcept {
    PackTypesMask<T, N> r;
    QWQDSP_AUTO_VECTORLIZE
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] < b.data[i] ? internal::MakeMaskType<T>::kTrue : internal::MakeMaskType<T>::kFalse;
    return r;
}
// ==
template<class T, size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackTypesMask<T, N> operator==(PackTypes<T, N> const& a, PackTypes<T, N> const& b) noexcept {
    PackTypesMask<T, N> r;
    QWQDSP_AUTO_VECTORLIZE
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] == b.data[i] ? internal::MakeMaskType<T>::kTrue : internal::MakeMaskType<T>::kFalse;
    return r;
}
// <=
template<class T, size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackTypesMask<T, N> operator<=(PackTypes<T, N> const& a, PackTypes<T, N> const& b) noexcept {
    PackTypesMask<T, N> r;
    QWQDSP_AUTO_VECTORLIZE
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] <= b.data[i] ? internal::MakeMaskType<T>::kTrue : internal::MakeMaskType<T>::kFalse;
    return r;
}
// >=
template<class T, size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackTypesMask<T, N> operator>=(PackTypes<T, N> const& a, PackTypes<T, N> const& b) noexcept {
    PackTypesMask<T, N> r;
    QWQDSP_AUTO_VECTORLIZE
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] >= b.data[i] ? internal::MakeMaskType<T>::kTrue : internal::MakeMaskType<T>::kFalse;
    return r;
}
// !=
template<class T, size_t N>
QWQDSP_FORCE_INLINE
static inline constexpr PackTypesMask<T, N> operator!=(PackTypes<T, N> const& a, PackTypes<T, N> const& b) noexcept {
    PackTypesMask<T, N> r;
    QWQDSP_AUTO_VECTORLIZE
    for (size_t i = 0; i < N; ++i) r.data[i] = a.data[i] != b.data[i] ? internal::MakeMaskType<T>::kTrue : internal::MakeMaskType<T>::kFalse;
    return r;
}

// ----------------------------------------
// ops
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
    template<std::floating_point T, size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackTypes<T, N> Floor(PackTypes<T, N> const& a) noexcept {
        PackTypes<T, N> r;
        QWQDSP_AUTO_VECTORLIZE
        for (size_t i = 0; i < N; ++i) r.data[i] = std::floor(a.data[i]);
        return r;
    }

    // fma
    template<class T, size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackTypes<T, N> Fma(PackTypes<T, N> const& mula, PackTypes<T, N> const& mulb, PackTypes<T, N> const& addc) noexcept {
        PackTypes<T, N> r;
        QWQDSP_AUTO_VECTORLIZE
        for (size_t i = 0; i < N; ++i) r.data[i] = mula.data[i] * mulb.data[i] + addc.data[i];
        return r;
    }

    // min
    template<class T, size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackTypes<T, N> Min(PackTypes<T, N> const& a, PackTypes<T, N> const& b) noexcept {
        PackTypes<T, N> r;
        QWQDSP_AUTO_VECTORLIZE
        for (size_t i = 0; i < N; ++i) r.data[i] = std::min(a.data[i], b.data[i]);
        return r;
    }

    // max
    template<class T, size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackTypes<T, N> Max(PackTypes<T, N> const& a, PackTypes<T, N> const& b) noexcept {
        PackTypes<T, N> r;
        QWQDSP_AUTO_VECTORLIZE
        for (size_t i = 0; i < N; ++i) r.data[i] = std::max(a.data[i], b.data[i]);
        return r;
    }

    // clamp
    template<class T, size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackTypes<T, N> Clamp(PackTypes<T, N> const& a, PackTypes<T, N> const& min, PackTypes<T, N> const& max) noexcept {
        PackTypes<T, N> r;
        QWQDSP_AUTO_VECTORLIZE
        for (size_t i = 0; i < N; ++i) r.data[i] = std::clamp(a.data[i], min.data[i], max.data[i]);
        return r;
    }

    // shuffle
    // for 4
    template<class T, size_t a, size_t b, size_t c, size_t d>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackTypes<T, 4> Shuffle(PackTypes<T, 4> const& reg) noexcept {
        return PackTypes<T, 4> {
            reg.data[a],
            reg.data[b],
            reg.data[c],
            reg.data[d]
        };
    }
    // for 8
    template<class T, size_t a, size_t b, size_t c, size_t d, size_t e, size_t f, size_t g, size_t h>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackTypes<T, 8> Shuffle(PackTypes<T, 8> const& reg) noexcept {
        return PackTypes<T, 8> {
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
    
    // blend
    // for 4
    template<class T, size_t ab0, size_t ab1, size_t ab2, size_t ab3>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackTypes<T, 4> Blend(PackTypes<T, 4> const& zero_reg, PackTypes<T, 4> const& one_reg) noexcept {
        return PackTypes<T, 4> {
            ab0 == 0 ? zero_reg.data[0] : one_reg.data[0],
            ab1 == 1 ? zero_reg.data[1] : one_reg.data[1],
            ab2 == 2 ? zero_reg.data[2] : one_reg.data[2],
            ab3 == 3 ? zero_reg.data[3] : one_reg.data[3]
        };
    }
    // for 8
    template<class T, size_t ab0, size_t ab1, size_t ab2, size_t ab3, size_t ab4, size_t ab5, size_t ab6, size_t ab7>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackTypes<T, 8> Blend(PackTypes<T, 8> const& zero_reg, PackTypes<T, 8> const& one_reg) noexcept {
        return PackTypes<T, 8> {
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

    // float.positiveFrac
    template<class T, size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackTypes<T, N> PositiveFrac(PackTypes<T, N> const& x) noexcept {
        if constexpr (N == 4 && !kHasSSE4) {
            return {
                x[0] - static_cast<int>(x[0]),
                x[1] - static_cast<int>(x[1]),
                x[2] - static_cast<int>(x[2]),
                x[3] - static_cast<int>(x[3])
            };
        }
        else {
            PackTypes<T, N> r;
            QWQDSP_AUTO_VECTORLIZE
            for (size_t i = 0; i < N; ++i) r.data[i] = x.data[i] - std::floor(x.data[i]);
            return r;
        }
    }

    // float.frac
    template<class T, size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackTypes<T, N> Frac(PackTypes<T, N> const& x) noexcept {
        if constexpr (N == 4 && !kHasSSE4) {
            auto r = PositiveFrac(x);
            PackTypes<T, N> fix{
                x[0] < 0 ? 1.0f : 0.0f,
                x[1] < 0 ? 1.0f : 0.0f,
                x[2] < 0 ? 1.0f : 0.0f,
                x[3] < 0 ? 1.0f : 0.0f
            };
            r += fix;
            return r;
        }
        else {
            PackTypes<T, N> r;
            QWQDSP_AUTO_VECTORLIZE
            for (size_t i = 0; i < N; ++i) r.data[i] = x.data[i] - std::floor(x.data[i]);
            return r;
        }
    }

    // reduceAdd
    template<class T, size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr T ReduceAdd(PackTypes<T, N> const& x) noexcept {
        float sum = 0.0f;
        QWQDSP_AUTO_VECTORLIZE
        for (size_t i = 0; i < N; ++i) sum += x.data[i];
        return sum;
    }

    // float.sqrt
    template<class T, size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackTypes<T, N> Sqrt(PackTypes<T, N> const& x) noexcept {
        PackTypes<T, N> r;
        QWQDSP_AUTO_VECTORLIZE
        for (size_t i = 0; i < N; ++i) r.data[i] = std::sqrt(x.data[i]);
        return r;
    }

    // float.abs
    template<class T, size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackTypes<T, N> Abs(PackTypes<T, N> const& x) noexcept {
        PackTypes<T, N> r;
        QWQDSP_AUTO_VECTORLIZE
        for (size_t i = 0; i < N; ++i) r.data[i] = std::abs(x.data[i]);
        return r;
    }

    // float.tan
    template<class T, size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackTypes<T, N> Tan(PackTypes<T, N> const& x) noexcept {
        PackTypes<T, N> r;
        QWQDSP_AUTO_VECTORLIZE
        for (size_t i = 0; i < N; ++i) r.data[i] = std::tan(x.data[i]);
        return r;
    }

    // float.atan
    template<class T, size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackTypes<T, N> Atan(PackTypes<T, N> const& x) noexcept {
        PackTypes<T, N> r;
        QWQDSP_AUTO_VECTORLIZE
        for (size_t i = 0; i < N; ++i) r.data[i] = std::atan(x.data[i]);
        return r;
    }

    // float.asinh
    template<class T, size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackTypes<T, N> Asinh(PackTypes<T, N> const& x) noexcept {
        PackTypes<T, N> r;
        QWQDSP_AUTO_VECTORLIZE
        for (size_t i = 0; i < N; ++i) r.data[i] = std::asinh(x.data[i]);
        return r;
    }

    // float.cosh
    template<class T, size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackTypes<T, N> Cosh(PackTypes<T, N> const& x) noexcept {
        PackTypes<T, N> r;
        QWQDSP_AUTO_VECTORLIZE
        for (size_t i = 0; i < N; ++i) r.data[i] = std::cosh(x.data[i]);
        return r;
    }

    // float.log
    template<class T, size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackTypes<T, N> Log(PackTypes<T, N> const& x) noexcept {
        PackTypes<T, N> r;
        QWQDSP_AUTO_VECTORLIZE
        for (size_t i = 0; i < N; ++i) r.data[i] = std::log(x.data[i]);
        return r;
    }

    // float.log10
    template<class T, size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackTypes<T, N> Log10(PackTypes<T, N> const& x) noexcept {
        PackTypes<T, N> r;
        QWQDSP_AUTO_VECTORLIZE
        for (size_t i = 0; i < N; ++i) r.data[i] = std::log10(x.data[i]);
        return r;
    }

    // float.exp2
    template<class T, size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackTypes<T, N> Exp2(PackTypes<T, N> const& x) noexcept {
        PackTypes<T, N> r;
        QWQDSP_AUTO_VECTORLIZE
        for (size_t i = 0; i < N; ++i) r.data[i] = std::exp2(x.data[i]);
        return r;
    }

    // float.exp
    template<class T, size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackTypes<T, N> Exp(PackTypes<T, N> const& x) noexcept {
        PackTypes<T, N> r;
        QWQDSP_AUTO_VECTORLIZE
        for (size_t i = 0; i < N; ++i) r.data[i] = std::exp(x.data[i]);
        return r;
    }

    // float.pow
    template<class T, size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackTypes<T, N> Pow(PackTypes<T, N> const& x, PackTypes<T, N> const& y) noexcept {
        PackTypes<T, N> r;
        QWQDSP_AUTO_VECTORLIZE
        for (size_t i = 0; i < N; ++i) r.data[i] = std::pow(x.data[i], y.data[i]);
        return r;
    }

    // select
    template<class T, size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackTypes<T, N> Select(PackTypesMask<T, N> const& mask, PackTypes<T, N> const& true_val, PackTypes<T, N> const& false_val) noexcept {
        PackTypes<T, N> r;
        QWQDSP_AUTO_VECTORLIZE
        for (size_t i = 0; i < N; ++i) r.data[i] = mask.data[i] == internal::MakeMaskType<T>::kTrue ? true_val.data[i] : false_val.data[i];
        return r;
    }

    template<class T, size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackTypes<T, N> Select(PackTypesMask<T, N> const& mask, float true_val, float false_val) noexcept {
        PackTypes<T, N> r;
        QWQDSP_AUTO_VECTORLIZE
        for (size_t i = 0; i < N; ++i) r.data[i] = mask.data[i] == internal::MakeMaskType<T>::kTrue ? true_val : false_val;
        return r;
    }

    // lerp
    template<class T, size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackTypes<T, N> Lerp(PackTypes<T, N> const& x, PackTypes<T, N> const& y, PackTypes<T, N> const& t) noexcept {
        PackTypes<T, N> r;
        QWQDSP_AUTO_VECTORLIZE
        for (size_t i = 0; i < N; ++i) r.data[i] = x.data[i] + (y.data[i] - x.data[i]) * t.data[i];
        return r;
    }

    // lerp scalar
    template<class T, size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackTypes<T, N> Lerp(PackTypes<T, N> const& x, PackTypes<T, N> const& y, float t) noexcept {
        PackTypes<T, N> r;
        QWQDSP_AUTO_VECTORLIZE
        for (size_t i = 0; i < N; ++i) r.data[i] = x.data[i] + (y.data[i] - x.data[i]) * t;
        return r;
    }

    // float.Cos
    template<class T, size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackTypes<T, N> Cos(PackTypes<T, N> const& x) noexcept {
        PackTypes<T, N> r;
        QWQDSP_AUTO_VECTORLIZE
        for (size_t i = 0; i < N; ++i) r.data[i] = std::cos(x.data[i]);
        return r;
    }

    // float.Sin
    template<class T, size_t N>
    QWQDSP_FORCE_INLINE
    static inline constexpr PackTypes<T, N> Sin(PackTypes<T, N> const& x) noexcept {
        PackTypes<T, N> r;
        QWQDSP_AUTO_VECTORLIZE
        for (size_t i = 0; i < N; ++i) r.data[i] = std::sin(x.data[i]);
        return r;
    }
};

template<size_t N>
using PackFloat = PackTypes<float, N>;
template<size_t N>
using PackInt32 = PackTypes<int32_t, N>;
template<size_t N>
using PackIndex = PackTypes<uint32_t, N>;

}
