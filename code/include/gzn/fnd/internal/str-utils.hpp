#pragma once

#include <string>
#include <string_view>

#include "gzn/fnd/utility.hpp"

namespace gzn::fnd::util {

template<class T>
concept character_type = std::is_same_v<T, char> ||
                         std::is_same_v<T, char8_t> ||
                         std::is_same_v<T, char16_t> ||
                         std::is_same_v<T, char32_t> ||
                         std::is_same_v<T, wchar_t>;

static constexpr auto strlen_clamped(
  character_type auto const *ptr,
  usize const                max_len
) noexcept {
  usize length{};
  while (*ptr != usize{ 0 } && length < max_len) {
    ++ptr;
    ++length;
  }
  return length - static_cast<usize>(length == max_len);
}

} // namespace gzn::fnd::util
