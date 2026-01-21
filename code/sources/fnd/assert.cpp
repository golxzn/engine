#include <cstdio>

#if defined(GZN_PLATFORM_WINDOWS)
#  if defined(GZN_COMPILER_MSVC)
#    include <crtdbg.h>
#  endif // defined(GZN_COMPILER_MSVC)

#  if !defined(WIN32_LEAN_AND_MEAN)
#    define WIN32_LEAN_AND_MEAN
#  endif // !defined(WIN32_LEAN_AND_MEAN)

#  include <Windows.h>
#elif defined(GZN_PLATFORM_ANDROID)
#  include <android/log.h>
#else
#  include <cstdio>
#endif

#include "gzn/fnd/assert.hpp"

namespace gzn::fnd {

#if defined(GZN_ASSERTIONS)

static void default_assertion_function(
  cstr                       expression,
  assertion_function_context context
);

namespace __uwu {
namespace {

assertion_function         g_assertion_function{ default_assertion_function };
assertion_function_context g_function_context{};

} // namespace
} // namespace __uwu

static void default_assertion_function(
  cstr expression,
  assertion_function_context
) {
#  if defined(GZN_PLATFORM_WINDOWS)
#    define _PRINT_EXPRESSION
  if (::IsDebuggerPresent()) { OutputDebugStringA(expression); }
#  elif defined(GZN_PLATFORM_ANDROID)
  __android_log_print(ANDROID_LOG_INFO, "PRINTF", "%s\n", expression);
#  else
#    define _PRINT_EXPRESSION
#  endif

#  if defined(_PRINT_EXPRESSION)
  std::fprintf(stderr, "%s\n", expression);
#    undef _PRINT_EXPRESSION
#  endif // defined(_PRINT_EXPRESSION)

  GZN_DEBUG_BREAK();
}

void set_assertion_function(
  assertion_function         assert_function,
  assertion_function_context assert_context
) {
  __uwu::g_assertion_function = assert_function;
  __uwu::g_function_context   = assert_context;
}

void on_assert_failure(cstr expression) {
  if (__uwu::g_assertion_function) {
    __uwu::g_assertion_function(expression, __uwu::g_function_context);
  }
}

#else

void set_assertion_function(assertion_function, assertion_function_context) {}

void on_assert_failure(cstr) {}

#endif // defined(GZN_ASSERTIONS)

} // namespace gzn::fnd
