#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace gzn {

using s8    = int8_t;
using u8    = uint8_t;

using s16   = int16_t;
using u16   = uint16_t;

using s32   = int32_t;
using u32   = uint32_t;

using s64   = int64_t;
using u64   = uint64_t;

using f32   = float;
using f64   = double;

using cstr  = char const *;
using byte  = std::byte;

using usize = u64;
using ssize = s64;

template<class T, usize Length>
using carr = T[Length];

[[nodiscard]]
consteval auto operator""_B(long double count) noexcept -> usize {
  return static_cast<usize>(count);
}

[[nodiscard]]
consteval auto operator""_B(unsigned long long count) noexcept -> usize {
  return static_cast<usize>(count);
}

[[nodiscard]]
consteval auto operator""_KB(long double count) noexcept -> usize {
  return static_cast<usize>(count * 1000.0l);
}

[[nodiscard]]
consteval auto operator""_KB(unsigned long long count) noexcept -> usize {
  return static_cast<usize>(count * 1000.0l);
}

[[nodiscard]]
consteval auto operator""_KiB(long double count) noexcept -> usize {
  return static_cast<usize>(count * 1024.0l);
}

[[nodiscard]]
consteval auto operator""_KiB(unsigned long long count) noexcept -> usize {
  return static_cast<usize>(count * 1024.0l);
}

[[nodiscard]]
consteval auto operator""_MB(long double count) noexcept -> usize {
  return static_cast<usize>(count * 1000.0l * 1000.0l);
}

[[nodiscard]]
consteval auto operator""_MB(unsigned long long count) noexcept -> usize {
  return static_cast<usize>(count * 1000.0l * 1000.0l);
}

[[nodiscard]]
consteval auto operator""_MiB(long double count) noexcept -> usize {
  return static_cast<usize>(count * 1024.0l * 1024.0l);
}

[[nodiscard]]
consteval auto operator""_MiB(unsigned long long count) noexcept -> usize {
  return static_cast<usize>(count * 1024.0l * 1024.0l);
}

[[nodiscard]]
consteval auto operator""_GB(long double count) noexcept -> usize {
  return static_cast<usize>(count * 1000.0l * 1000.0l) * 1000ull;
}

[[nodiscard]]
consteval auto operator""_GB(unsigned long long count) noexcept -> usize {
  return static_cast<usize>(count * 1000.0l * 1000.0l) * 1000ull;
}

[[nodiscard]]
consteval auto operator""_GiB(long double count) noexcept -> usize {
  return static_cast<usize>(count * 1024.0l * 1024.0l) * 1024ull;
}

[[nodiscard]]
consteval auto operator""_GiB(unsigned long long count) noexcept -> usize {
  return static_cast<usize>(count * 1024.0l * 1024.0l) * 1024ull;
}


} // namespace gzn
