#pragma once

#if defined(_MSC_VER)
#  define GZN_INLINE __forceinline
#elif defined(__GNUC__)
#  define gzn_inline inline __attribute__((__always_inline__))
#else
#  define gzn_inline inline
#endif

#if defined(GZN_DEBUG)
#  define gzn_if_debug(line) line
#else
#  define gzn_if_debug(line)
#endif // defined(GZN_DEBUG)
