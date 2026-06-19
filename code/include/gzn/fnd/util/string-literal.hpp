#pragma once

#include <algorithm>
#include <array>
#include <string_view>

#include "gzn/fnd/types.hpp"

namespace gzn::fnd {

template<usize N>
struct string_literal {
  using value_type = char;
  static inline constexpr usize length{ N - 1uz };

  std::array<value_type, N> value;

  constexpr string_literal(auto const... _chars)
    requires(std::is_same_v<decltype(_chars), char const> && ...)
    : value{ _chars..., '\0' } {}

  constexpr string_literal(std::array<char, N> const _arr)
    : value(_arr) {}

  constexpr string_literal(char const (&_str)[N]) {
    std::copy_n(_str, N, std::begin(value));
  }

  template<class T>
    requires(std::is_same_v<T, char const *> || std::is_same_v<T, char *>)
  explicit constexpr string_literal(T _data) {
    std::copy_n(_data, N, std::begin(value));
  }

  [[nodiscard]]
  constexpr auto size() const noexcept -> usize {
    return length;
  }

  [[nodiscard]]
  constexpr auto data() const noexcept {
    return std::data(value);
  }

  [[nodiscard]]
  constexpr auto view() const noexcept -> std::string_view {
    return { std::data(value), length };
  }

  [[nodiscard]]
  constexpr operator std::string_view() const noexcept {
    return view();
  }
};

template<usize N1, usize N2>
constexpr inline auto operator==(
  string_literal<N1> const &_first,
  string_literal<N2> const &_second
) -> bool {
  if constexpr (N1 != N2) { return false; }
  return _first.string_view() == _second.string_view();
}

template<usize N1, usize N2>
constexpr inline auto operator!=(
  string_literal<N1> const &_first,
  string_literal<N2> const &_second
) -> bool {
  return !(_first == _second);
}

} // namespace gzn::fnd
