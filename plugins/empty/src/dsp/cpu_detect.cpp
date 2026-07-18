#include "phaser_simd.hpp"

//==============================================================================
//  CPU feature detection
//==============================================================================

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#  define CPU_X86
#elif defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(_M_ARM64) || defined(__aarch64__)
#  define CPU_ARM
#endif

// ---- x86 CPUID ----
#ifdef CPU_X86
#  if defined(_MSC_VER)
#    include <intrin.h>
static inline void cpuid(int info[4], int leaf) {
    __cpuid(info, leaf);
}
static inline void cpuidex(int info[4], int leaf, int subleaf) {
    __cpuidex(info, leaf, subleaf);
}
#  elif defined(__GNUC__) || defined(__clang__)
#    include <cpuid.h>
static inline void cpuid(int info[4], int leaf) {
    __cpuid_count(leaf, 0, info[0], info[1], info[2], info[3]);
}
static inline void cpuidex(int info[4], int leaf, int subleaf) {
    __cpuid_count(leaf, subleaf, info[0], info[1], info[2], info[3]);
}
#  endif
#endif

// ---- x86-64 xgetbv helper ----
#if defined(CPU_X86)
#  if defined(_MSC_VER)
static inline uint64_t xgetbv(uint32_t ctrl) {
    return _xgetbv(ctrl);
}
#  else
static inline uint64_t xgetbv(uint32_t ctrl) {
    uint32_t eax, edx;
    __asm__ volatile("xgetbv" : "=a"(eax), "=d"(edx) : "c"(ctrl));
    return (static_cast<uint64_t>(edx) << 32) | eax;
}
#  endif
#endif

namespace cpu {

bool hasSSE2() noexcept {
#if defined(CPU_X86)
    int info[4] = {};
    cpuid(info, 1);
    return (info[3] & (1 << 26)) != 0;
#elif defined(CPU_ARM)
    return false;
#else
    return false;
#endif
}

bool hasSSE41() noexcept {
#if defined(CPU_X86)
    int info[4] = {};
    cpuid(info, 1);
    return (info[2] & (1 << 19)) != 0;
#else
    return false;
#endif
}

bool hasAVX() noexcept {
#if defined(CPU_X86)
    int info[4] = {};
    cpuid(info, 1);
    // Check both AVX (bit 28) and OSXSAVE (bit 27)
    if ((info[2] & ((1 << 28) | (1 << 27))) != ((1 << 28) | (1 << 27)))
        return false;
    // Check OS has enabled XMM and YMM state
    return (xgetbv(0) & 6) == 6;
#else
    return false;
#endif
}

bool hasAVX2() noexcept {
#if defined(CPU_X86)
    if (!hasAVX()) return false;
    int info[4] = {};
    cpuidex(info, 7, 0);
    return (info[1] & (1 << 5)) != 0;
#else
    return false;
#endif
}

bool hasFMA() noexcept {
#if defined(CPU_X86)
    if (!hasAVX()) return false;
    int info[4] = {};
    cpuid(info, 1);
    return (info[2] & (1 << 12)) != 0;
#else
    return false;
#endif
}

bool hasNEON() noexcept {
#if defined(CPU_ARM)
    return true;
#else
    return false;
#endif
}

} // namespace cpu
