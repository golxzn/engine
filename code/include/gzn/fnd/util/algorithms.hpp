#pragma once

#include "gzn/fnd/containers/span.hpp"

namespace gzn::fnd::util {

template<class T>
[[nodiscard]]
constexpr auto is_sorted(fnd::span<T> const sequence) noexcept -> bool {
  std::remove_const_t<T> prev{};
  for (auto const block_size : sequence) {
    if (prev > block_size) { return false; }
    prev = block_size;
  }
  return true;
}

} // namespace gzn::fnd::util
