#include "gzn/fnd/memory.hpp"

#if defined(GZN_PLATFORM_LINUX) || defined(GZN_PLATFORM_DARWIN) || \
  defined(GZN_PLATFORM_ANDROID) || defined(GZN_PLATFORM_SWITCH) || \
  defined(GZN_PLATFORM_PS4) || defined(GZN_PLATFORM_PS5)
#  include <unistd.h>
#  define __GZN_POSIX_IMPL

#elif defined(GZN_PLATFORM_WINDOWS)

#  include <windows.h>

#endif

namespace gzn::fnd::mem {

auto get_page_size() -> usize {
#if defined(__GZN_POSIX_IMPL)

  if (long const size{ sysconf(_SC_PAGESIZE) }; size > 0) [[likely]] {
    return static_cast<usize>(size);
  }

  return DEFAULT_PAGE_SIZE;

#elif defined(GZN_PLATFORM_WINDOWS)

  SYSTEM_INFO si;
  GetNativeSystemInfo(&si);
  return static_cast<usize>(si.dwPageSize);

#endif
}

} // namespace gzn::fnd::mem

#undef __GZN_POSIX_IMPL
