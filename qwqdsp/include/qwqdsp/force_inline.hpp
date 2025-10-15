#pragma once

#if defined (__clang__)
    #define QWQDSP_FORCE_INLINE [[clang::always_inline]]
#elif defined (__GNUC__)
    #define QWQDSP_FORCE_INLINE [[gnu::always_inline]]
#elif defined (_MSC_VER)
    #define QWQDSP_FORCE_INLINE __forceinline
#endif
