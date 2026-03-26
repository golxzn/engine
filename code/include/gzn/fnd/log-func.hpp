#pragma once

#include <algorithm>
#include <cstdarg>

#include "gzn/fnd/definitions.hpp"
#include "gzn/fnd/types.hpp"

namespace gzn::fnd {

enum class log_level : char {
  debug   = 'D',
  info    = 'I',
  warning = 'W',
  error   = 'E',
  fatal   = 'F'
};

using log_func_type =
  void (*)(void *user_data, log_level lvl, cstr module, cstr format, ...);

struct log_func {
  void         *user_data{};
  log_func_type func{};

  template<class... Args>
  gzn_inline void invoke_safe(
    log_level lvl,
    cstr      module,
    cstr      format,
    Args &&...args
  ) const {
    if (func && format) {
      func(user_data, lvl, module, format, std::forward<Args>(args)...);
    }
  }

  template<class... Args>
  gzn_inline void debug(cstr module, cstr format, Args &&...args) const {
    invoke_safe(log_level::debug, module, format, std::forward<Args>(args)...);
  }

  template<class... Args>
  gzn_inline void info(cstr module, cstr format, Args &&...args) const {
    invoke_safe(log_level::info, module, format, std::forward<Args>(args)...);
  }

  template<class... Args>
  gzn_inline void warn(cstr module, cstr format, Args &&...args) const {
    invoke_safe(
      log_level::warning, module, format, std::forward<Args>(args)...
    );
  }

  template<class... Args>
  gzn_inline void err(cstr module, cstr format, Args &&...args) const {
    invoke_safe(log_level::error, module, format, std::forward<Args>(args)...);
  }

  template<class... Args>
  gzn_inline void fatal(cstr module, cstr format, Args &&...args) const {
    invoke_safe(log_level::fatal, module, format, std::forward<Args>(args)...);
  }
};

} // namespace gzn::fnd
