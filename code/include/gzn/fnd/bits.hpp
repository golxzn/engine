#pragma once

#include <bit>
#include <concepts>

#include "gzn/fnd/types.hpp"

namespace gzn::fnd::bits {

[[nodiscard]]
constexpr auto get_shift_from_power_of_2(
  std::unsigned_integral auto const value
) -> ssize {
  using value_type = std::remove_cvref_t<decltype(value)>;
  constexpr auto bits_count{ static_cast<ssize>(sizeof(value_type) * 8u) };

  for (ssize idx{ bits_count }; idx != 0; --idx) {
    if (value == static_cast<value_type>(1u << idx)) { return idx; }
  }

  return -1;
}

[[nodiscard]]
constexpr auto closest_upper_power_of_two(
  std::unsigned_integral auto value
) noexcept {
  --value;
  value |= value >> 1;
  value |= value >> 2;
  value |= value >> 4;
  if constexpr (sizeof(value) > 1) { value |= value >> 8; }
  if constexpr (sizeof(value) > 2) { value |= value >> 16; }
  if constexpr (sizeof(value) > 4) { value |= value >> 32; }
  ++value;
  return value;
}

[[nodiscard]]
constexpr auto closest_lower_power_of_two(
  std::unsigned_integral auto value
) noexcept {
  return std::has_single_bit(value) ? value
                                    : closest_upper_power_of_two(value) >> 1;
}

} // namespace gzn::fnd::bits
