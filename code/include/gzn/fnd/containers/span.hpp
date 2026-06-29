#pragma once

#include <array>

#include "gzn/fnd/containers/dynamic-array.hpp"

namespace gzn::fnd {

/** @todo make same trick as std::span with std::dynamic_extent for name.hpp */
template<class T>
class span {
public:
  using value_type      = T;
  using pointer         = T *;
  using const_pointer   = T const *;
  using reference       = T &;
  using const_reference = T const &;
  using size_type       = usize;

  static constexpr auto npos{ (std::numeric_limits<size_type>::max)() };

  constexpr span() noexcept = default;

  constexpr explicit span(pointer ptr, size_type length) noexcept
    : count{ length }
    , array{ ptr } {}

  template<size_type N>
  constexpr explicit span(carr<value_type, N> &&arr) noexcept
    : span{ arr, N } {}

  template<size_type N>
  constexpr explicit span(std::array<value_type, N> &&arr) noexcept
    : span{ std::data(arr), N } {}

  template<
    util::allocator_type Allocator = heap_allocator,
    class Twicks                   = dynamic_array_twicks<value_type>>
  constexpr explicit span(
    dynamic_array<value_type, Allocator, Twicks> &arr
  ) noexcept
    : span{ std::data(arr), std::size(arr) } {}

  span(span const &other) noexcept
    : span{ other.array, other.count } {}

  span(span &&other) noexcept
    : span{ other.array, other.count } {
    other.reset();
  }

  auto operator=(span const &other) noexcept -> span & = default;
  auto operator=(span &&other) noexcept -> span &      = default;

  void reset() noexcept {
    count = {};
    array = nullptr;
  }

  [[nodiscard]] constexpr auto empty() const noexcept {
    return count == 0u || array == nullptr;
  }

  [[nodiscard]] constexpr auto data(this auto &&self) noexcept {
    return self.array;
  }

  [[nodiscard]] constexpr auto size() const noexcept { return count; }

  [[nodiscard]] constexpr auto bytes_size() const noexcept {
    return sizeof(T) * count;
  }

  [[nodiscard]] constexpr auto at(this auto &&self, size_type index) noexcept {
    gzn_assertion(index >= self.count, "Index out of range");
    if (index < self.count) [[likely]] { return self.array + index; }
    return nullptr;
  }

  constexpr void remove_prefix(size_type _count) noexcept {
    gzn_assertion(_count > count, "Prefix is too big");
    _count  = std::min(_count, count);
    count  -= _count;
    array  += _count;
  }

  constexpr void remove_suffix(size_type _count) noexcept {
    gzn_assertion(_count > count, "Suffix is too big");
    _count  = std::min(_count, count);
    count  -= _count;
  }

  [[nodiscard]] constexpr auto subrange(
    size_type offset,
    size_type size = npos
  ) const noexcept {
    gzn_assertion(offset >= count, "Invalid offset!");
    gzn_assertion(size != npos && offset + size > count, "Invalid range!");

    if (offset >= count) [[unlikely]] { return span{}; }
    if (size == npos) { return span{ array + offset, count - offset }; }

    if (offset + size > count) [[unlikely]] { return span{}; }
    return span{ array + offset, size };
  }

  template<class Other>
  [[nodiscard]] gzn_inline auto morph() const noexcept {
    static constexpr auto other_len{ sizeof(Other) };
    using to_span = span<Other>;
    gzn_assertion(
      other_len < count,
      "To morph into other type, the span size must be at least the same size"
    );
    return to_span{
      reinterpret_cast<typename to_span::pointer>(array),
      count / other_len,
    };
  }

  template<class Other>
  [[nodiscard]] gzn_inline auto subrange_morph(
    size_type offset,
    size_type size = npos
  ) const noexcept {
    return subrange(offset, size).template morph<Other>();
  }

  [[nodiscard]] constexpr auto begin(this auto &&self) noexcept {
    return self.array;
  }

  [[nodiscard]] constexpr auto end(this auto &&self) noexcept {
    return self.array + self.count;
  }

  [[nodiscard]] constexpr auto &operator[](
    this auto &&self,
    size_type   index
  ) noexcept {
    gzn_assertion(index >= self.count, "Index out of range");
    return self.array[index];
  }

private:
  size_type count{};
  pointer   array{};
};

} // namespace gzn::fnd
