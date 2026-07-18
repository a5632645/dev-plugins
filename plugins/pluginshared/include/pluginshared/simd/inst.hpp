#pragma once

namespace simd {
enum class Inst {
    Scalar,
    SSE2,
    SSE4,
    AVX,
    AVX2,
    FMA,
    NEON
};

struct Tag128 {};
struct Tag256 {};
static constexpr Tag128 tag128;
static constexpr Tag256 tag256;

} // namespace simd
