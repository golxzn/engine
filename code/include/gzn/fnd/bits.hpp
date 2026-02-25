#pragma once

#include <bit>
#include <concepts>

namespace gzn::fnd::bits {

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

constexpr auto closest_lower_power_of_two(
  std::unsigned_integral auto value
) noexcept {
  return std::has_single_bit(value) ? value
                                    : closest_upper_power_of_two(value) >> 1;
}

} // namespace gzn::fnd::bits
