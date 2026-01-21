#pragma once

#include <algorithm>
#include <iterator>

#include "gzn/fnd/types.hpp"

namespace gzn::fnd {

class raw_data {
public:
  using size_type = u64;

  explicit constexpr raw_data(
    byte const *const data   = nullptr,
    size_type const   size   = 0,
    size_type const   stride = sizeof(byte)
  ) noexcept
    : m_data{ data }
    , m_stride{ stride }
    , m_size{ size } {}

  template<class T, usize Length>
  constexpr explicit raw_data(c_array<T const, Length> &&array) noexcept
    : m_data{ reinterpret_cast<byte const *>(array) }
    , m_stride{ sizeof(T) }
    , m_size{ Length * m_stride } {}

  constexpr raw_data(raw_data const &other) noexcept = default;
  raw_data(raw_data &&other) noexcept                = default;

  constexpr auto operator=(raw_data const &other) noexcept
    -> raw_data &                                         = default;
  auto operator=(raw_data &&other) noexcept -> raw_data & = default;

  [[nodiscard]]
  constexpr auto data() const noexcept -> byte const * {
    return m_data;
  }

  [[nodiscard]]
  constexpr auto size() const noexcept -> size_type {
    return m_size;
  }

  [[nodiscard]]
  constexpr auto stride() const noexcept -> size_type {
    return m_stride;
  }

  [[nodiscard]]
  constexpr auto is_same_as(raw_data const other) const noexcept -> bool {
    return m_data == other.m_data && m_size == other.m_size &&
           m_stride == other.m_stride;
  }

  [[nodiscard]]
  constexpr auto is_similar_to(raw_data const other) const noexcept -> bool {
    if (m_size != other.m_size) { return false; }
    return std::equal(m_data, std::next(m_data, m_size), other.m_data);
  }

private:
  byte const *m_data{ nullptr };
  size_type   m_stride{};
  size_type   m_size{};
};

} // namespace gzn::fnd
