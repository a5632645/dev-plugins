#include "qwqdsp/psimd/vec8.hpp"
#include <immintrin.h>

int main() {
    qwqdsp::psimd::Vec8f32 x{
        1,
        2,
        3,
        4,
        5,
        6,
        7,
        8,
    };

    auto vx = _mm256_load_ps(x.x);
    auto shuffle_mask = _mm256_set_epi32(6, 5, 4, 3, 2, 1, 0, 7);
    auto vy = _mm256_permutevar8x32_ps(vx, shuffle_mask);
    auto iwant = _mm256_set1_ps(0.0f);
    auto vvv = _mm256_blend_ps(vy, iwant, 0b00000001);
}