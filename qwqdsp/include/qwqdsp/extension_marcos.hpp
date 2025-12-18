#pragma once

#if defined (__clang__)
    #define QWQDSP_FORCE_INLINE [[clang::always_inline]]
#elif defined (__GNUC__)
    #define QWQDSP_FORCE_INLINE [[gnu::always_inline]]
#elif defined (_MSC_VER)
    #define QWQDSP_FORCE_INLINE __forceinline
#else
    #define QWQDSP_FORCE_INLINE
#endif

#if defined (__clang__)
    #define QWQDSP_AUTO_VECTORLIZE _Pragma("clang loop vectorize(enable) interleave(enable)")
#elif defined (__GNUC__)
    #define QWQDSP_AUTO_VECTORLIZE _Pragma("GCC optimize(\"tree-vectorize\")")
#elif defined (_MSC_VER)
    #define QWQDSP_AUTO_VECTORLIZE __pragma(loop(hint_parallel(0)))
#else
    #define QWQDSP_AUTO_VECTORLIZE
#endif
