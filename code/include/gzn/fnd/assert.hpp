#pragma once

#include "gzn/fnd/definitions.hpp"
#include "gzn/fnd/types.hpp"

namespace gzn::fnd {

using assertion_function_context = void *;
using assertion_function =
  void (*)(cstr expression, assertion_function_context context);

void set_assertion_function(
  assertion_function         assert_function,
  assertion_function_context assert_context
);

void on_assert_failure(cstr expression);

[[noreturn]] inline void unreachable() {
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
  __assume(false);
#else  // GCC, Clang
  __builtin_unreachable();
#endif // defined(_MSC_VER) && !defined(__clang__) // MSVC
}

#if defined(GZN_ASSERTIONS)
#  define gzn_do_assertion(message)                                 \
    ((void)(gzn::fnd::on_assert_failure(__FILE__ ": " message), 0))

#  define gzn_assertion(expression, message)                          \
    ((void)((expression) ||                                           \
            (gzn::fnd::on_assert_failure(__FILE__ ": " message), 0)))
#else
#  define gzn_do_assertion(message)
#  define gzn_assertion(expression, message)
#endif // defined(GZN_ASSERTIONS)

#define gzn_static_assert         static_assert
#define gzn_do_static_assert(msg) static_assert(true, msg)

/* This code was taken from EASTL (https://github.com/electronicarts/EASTL)
 * and originally implemented by rparolin. Thanks, rparolin!
 * Exact source:
 * https://github.com/electronicarts/EASTL/blob/e323f67e5d93fdadcd44f7998069d804697b1945/include/EASTL/internal/config.h#L630C1-L671C7
 */
#if !defined(GZN_DEBUG_BREAK_OVERRIDE)
#  if !defined(GZN_DEBUG_BREAK)
#    if defined(_MSC_VER) && (_MSC_VER >= 1300)
#      define GZN_DEBUG_BREAK()                                          \
        __debugbreak() // This is a compiler intrinsic which will map to
                       // appropriate inlined asm for the platform.
#    elif defined(GZN_PLATFORM_NINTENDO)
#      define GZN_DEBUG_BREAK()                                  \
        __builtin_debugtrap() // Consider using the CLANG define
#    elif (defined(GZN_PROCESSOR_ARM) && !defined(GZN_PROCESSOR_ARM64)) && \
      defined(__APPLE__)
#      define GZN_DEBUG_BREAK() asm("trap")
#    elif defined(GZN_PROCESSOR_ARM64) && defined(__APPLE__)
#      include <signal.h>
#      include <unistd.h>
#      define GZN_DEBUG_BREAK() kill(getpid(), SIGINT)
#    elif defined(GZN_PROCESSOR_ARM64) && defined(__GNUC__)
#      define GZN_DEBUG_BREAK() asm("brk 10")
#    elif defined(GZN_PROCESSOR_ARM) && defined(__GNUC__)
#      define GZN_DEBUG_BREAK()                                       \
        asm("BKPT 10") // The 10 is arbitrary. It's just a unique id.
#    elif defined(GZN_PROCESSOR_ARM) && defined(__ARMCC_VERSION)
#      define GZN_DEBUG_BREAK() __breakpoint(10)
#    elif defined(GZN_PROCESSOR_POWERPC) // Generic PowerPC.
#      define GZN_DEBUG_BREAK()                                         \
        asm(                                                            \
          ".long 0"                                                     \
        ) // This triggers an exception by executing opcode 0x00000000.
#    elif (defined(GZN_PROCESSOR_X86) || defined(GZN_PROCESSOR_X86_64)) && \
      defined(GZN_ASM_DIALECT_INTEL)
#      define GZN_DEBUG_BREAK() __asm__("int 3")
#    elif (defined(GZN_PROCESSOR_X86) || defined(GZN_PROCESSOR_X86_64)) && \
      (defined(GZN_ASM_DIALECT_ATT) || defined(__GNUC__))
#      define GZN_DEBUG_BREAK() asm("int3")
#    else
void GZN_DEBUG_BREAK(); // User must define this externally.
#    endif
#  else
void GZN_DEBUG_BREAK(); // User must define this externally.
#  endif
#else
#  ifndef GZN_DEBUG_BREAK
#    if GZN_DEBUG_BREAK_OVERRIDE == 1
// define an empty callable to satisfy the call site.
#      define GZN_DEBUG_BREAK ([] {})
#    else
#      define GZN_DEBUG_BREAK GZN_DEBUG_BREAK_OVERRIDE
#    endif
#  else
#error GZN_DEBUG_BREAK is already defined yet you would like to override it. Please ensure no other headers are already defining GZN_DEBUG_BREAK before this header (config.h) is included
#  endif
#endif

gzn_inline void breakpoint() { GZN_DEBUG_BREAK(); }

} // namespace gzn::fnd
