#pragma once

#include <algorithm>
#include <concepts>
#include <limits>

#include "gzn/fnd/definitions.hpp"

namespace gzn::fnd {

template<
  std::unsigned_integral T,
  T                      Minimum = std::numeric_limits<T>::min(),
  T                      Maximum = std::numeric_limits<T>::max()>
class alignas(T) clamped {
public:
  constexpr clamped() = default;

  constexpr explicit clamped(T value) noexcept
    : m_value{ std::clamp(value, Minimum, Maximum) } {}

  constexpr clamped(clamped const &)                         = default;
  constexpr clamped(clamped &&) noexcept                     = default;
  constexpr auto operator=(clamped const &) -> clamped &     = default;
  constexpr auto operator=(clamped &&) noexcept -> clamped & = default;

  constexpr auto operator=(T value) noexcept -> clamped & {
    m_value = std::clamp(value, Minimum, Maximum);
    return *this;
  }

  [[nodiscard]]
  gzn_inline constexpr auto value() const noexcept -> T {
    return m_value;
  }

  gzn_inline constexpr operator T() const noexcept { return m_value; }

  gzn_inline constexpr auto operator==(clamped const &o) const noexcept
    -> bool {
    return o.m_value == m_value;
  }

  gzn_inline constexpr auto operator<=>(clamped const &o) const noexcept {
    return o.m_value <=> m_value;
  }

private:
  T m_value{ Minimum };
};

} // namespace gzn::fnd
