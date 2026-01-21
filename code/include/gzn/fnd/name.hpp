#pragma once

#include <algorithm>
#include <array>
#include <span>

#include "gzn/fnd/hash.hpp"
#include "gzn/fnd/internal/str-utils.hpp"

namespace gzn::fnd {

u64 inline constexpr MAX_NAME_LENGTH_BYTES{ 48 };

template<util::character_type Char>
class basic_name;

template<util::character_type Char>
class basic_name_view {
public:
  using value_type    = Char;
  using name_type     = basic_name<Char>;
  using pointer       = Char *;
  using pointer_const = Char const *;
  using size_type     = usize;
  using hash_type     = u64;
  using string        = std::basic_string<Char>;
  using string_view   = std::basic_string_view<Char>;

  static constexpr size_type max_length_bytes{ MAX_NAME_LENGTH_BYTES };
  static constexpr size_type max_length{ max_length_bytes / sizeof(Char) };

  static constexpr auto clamp_length(size_type const length) noexcept {
    return std::min(length, max_length);
  }

  template<usize Len>
    requires(Len > 1)
  constexpr explicit basic_name_view(
    c_array<value_type const, Len> &&arr
  ) noexcept
    : m{ bake<Len - 1>(arr) } {}

  constexpr explicit basic_name_view(pointer_const cstr) noexcept
    : m{ bake({ cstr, util::strlen_clamped(cstr, max_length) }) } {}

  constexpr explicit basic_name_view(string_view const str) noexcept
    : m{ bake({ std::data(str), clamp_length(std::size(str)) }) } {}

  constexpr explicit basic_name_view(name_type const &str) noexcept;

  [[nodiscard]]
  constexpr auto data() const noexcept -> pointer_const {
    return m.data;
  }

  [[nodiscard]]
  constexpr auto size() const noexcept -> size_type {
    return m.length;
  }

  [[nodiscard]]
  constexpr auto empty() const noexcept -> size_type {
    return m.length == 0;
  }

  [[nodiscard]]
  constexpr auto hash() const noexcept -> hash_type {
    return m.hash;
  }

  [[nodiscard]]
  constexpr auto str_view() const noexcept -> string_view {
    return string_view{ m.data, m.size };
  }

  /// @todo maybe we should provide allocator to this function
  [[nodiscard]]
  constexpr auto str() const noexcept -> string {
    return string{ m.data, m.size };
  }

private:
  struct internal {
    pointer_const data{};
    size_type     length{};
    hash_type     hash{};
  } m;

  template<usize Len = std::dynamic_extent>
  static constexpr auto bake(std::span<value_type const, Len> span) noexcept
    -> internal {
    if constexpr (Len != std::dynamic_extent) {
      gzn_static_assert(Len <= max_length, "Too long name");
    }
    gzn_assertion(std::size(span) <= max_length, "Too long name");

    return internal{ .data   = std::data(span),
                     .length = static_cast<size_type>(std::size(span)),
                     .hash   = fnd::hash<value_type>({
                         .key = span,
                     }) };
  }
};

template<util::character_type Char>
class basic_name {
public:
  using view_type     = basic_name_view<Char>;
  using value_type    = typename view_type::value_type;
  using pointer       = typename view_type::pointer;
  using pointer_const = typename view_type::pointer_const;
  using size_type     = typename view_type::size_type;
  using hash_type     = typename view_type::hash_type;
  using string        = typename view_type::string;
  using string_view   = typename view_type::string_view;

  static constexpr size_type max_length_bytes{ view_type::max_length_bytes };
  static constexpr size_type max_length{ view_type::max_length };

  using storage_type = std::array<value_type, max_length>;

  template<usize Len>
    requires(Len > 0)
  constexpr explicit basic_name(c_array<value_type const, Len> &&arr) noexcept
    : m{ bake<Len - 1>(arr) } {}

  constexpr explicit basic_name(pointer_const cstr) noexcept
    : m{ bake({ cstr, util::strlen_clamped(cstr, max_length) }) } {}

  constexpr explicit basic_name(string_view const str) noexcept
    : m{ bake({ std::data(str), view_type::clamp_length(std::size(str)) }) } {}

  constexpr basic_name(view_type const &n) noexcept;

  [[nodiscard]]
  constexpr auto data() const noexcept -> pointer_const {
    return std::data(m.data);
  }

  [[nodiscard]]
  constexpr auto size() const noexcept -> size_type {
    return m.length;
  }

  [[nodiscard]]
  constexpr auto empty() const noexcept -> size_type {
    return m.length == 0;
  }

  [[nodiscard]]
  constexpr auto hash() const noexcept -> hash_type {
    return m.hash;
  }

  [[nodiscard]]
  constexpr auto view() const noexcept -> view_type {
    return view_type{ data(), size(), hash() };
  }

  [[nodiscard]]
  constexpr auto str_view() const noexcept -> string_view {
    return string_view{ m.data, m.size };
  }

  /// @todo maybe we should provide allocator to this function
  [[nodiscard]]
  constexpr auto str() const noexcept -> string {
    return string{ m.data, m.size };
  }

private:
  struct internal {
    storage_type data{};
    size_type    length;
    hash_type    hash{};
  } m;

  static constexpr auto make_data(view_type const &v) noexcept {
    return make_data({ std::data(v), std::size(v) });
  }

  template<usize Len = std::dynamic_extent>
  static constexpr auto make_data(
    std::span<value_type const, Len> span
  ) noexcept {
    storage_type data{};
    std::ranges::copy(span, std::begin(data));
    return data;
  }

  template<usize Len = std::dynamic_extent>
    requires(Len >= 0)
  static constexpr auto bake(std::span<value_type const, Len> span) noexcept
    -> internal {
    if constexpr (Len != std::dynamic_extent) {
      gzn_static_assert(Len <= max_length, "Too long name");
    }
    gzn_assertion(std::size(span) <= max_length_bytes, "Too long name");

    return internal{ .data{ make_data(span) },
                     .length = static_cast<size_type>(std::size(span)),
                     .hash   = fnd::hash<value_type>({
                         .key = span,
                     }) };
  }
};

template<util::character_type Char>
constexpr basic_name_view<Char>::basic_name_view(name_type const &n) noexcept
  : m{ .data{ std::data(n) }, .length{ std::size(n) }, .hash = n.hash() } {}

template<util::character_type Char>
constexpr basic_name<Char>::basic_name(view_type const &n) noexcept
  : m{ .data{ make_data(n) }, .length{ std::size(n) }, .hash = n.hash() } {}

template<util::character_type Char>
static constexpr auto operator==(
  basic_name<Char> const &lhv,
  basic_name<Char> const &rhv
) noexcept {
  return lhv.hash() == rhv.hash();
}

template<util::character_type Char>
static constexpr auto operator==(
  basic_name<Char> const      &lhv,
  basic_name_view<Char> const &rhv
) noexcept {
  return lhv.hash() == rhv.hash();
}

template<util::character_type Char>
static constexpr auto operator==(
  basic_name_view<Char> const &lhv,
  basic_name<Char> const      &rhv
) noexcept {
  return lhv.hash() == rhv.hash();
}

template<util::character_type Char>
static constexpr auto operator==(
  basic_name_view<Char> const &lhv,
  basic_name_view<Char> const &rhv
) noexcept {
  return lhv.hash() == rhv.hash();
}

using s8name_view  = basic_name_view<char>;
using u8name_view  = basic_name_view<char8_t>;
using u16name_view = basic_name_view<char16_t>;

using s8name       = basic_name<char>;
using u8name       = basic_name<char8_t>;
using u16name      = basic_name<char16_t>;

namespace name_literals {

static consteval auto operator""_name_hash(
  char const  *name,
  size_t const len
) noexcept {
  return s8name_view{
    s8name_view::string_view{ name, len }
  }.hash();
}

static consteval auto operator""_name_hash(
  char8_t const *name,
  size_t const   len
) noexcept {
  return u8name_view{
    u8name_view::string_view{ name, len }
  }.hash();
}

static consteval auto operator""_name_hash(
  char16_t const *name,
  size_t const    len
) noexcept {
  return u16name_view{
    u16name_view::string_view{ name, len }
  }.hash();
}

static consteval auto operator""_name(
  char const  *name,
  size_t const len
) noexcept {
  return s8name_view{
    s8name_view::string_view{ name, len }
  };
}

static consteval auto operator""_name(
  char8_t const *name,
  size_t const   len
) noexcept {
  return u8name_view{
    u8name_view::string_view{ name, len }
  };
}

static consteval auto operator""_name(
  char16_t const *name,
  size_t const    len
) noexcept {
  return u16name_view{
    u16name_view::string_view{ name, len }
  };
}

} // namespace name_literals

} // namespace gzn::fnd
