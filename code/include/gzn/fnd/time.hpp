#pragma once

#include "gzn/fnd/types.hpp"

namespace gzn::fnd {

struct time {
  u64 timestamp_nanoseconds{};

  [[nodiscard]]
  static auto now() noexcept -> time;
};

} // namespace gz
