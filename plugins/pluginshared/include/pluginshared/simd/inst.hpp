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

} // namespace simd
