#pragma once

#include <cstring>

#include "gzn/fnd/pointers.hpp"
#include "gzn/fnd/utility.hpp"

namespace gzn::fnd::containers {

namespace mem {

template<class T>
using pure_pointer_t = std::add_pointer_t<std::remove_all_extents_t<T>>;

template<class T, class Ptr = pure_pointer_t<T>>
[[nodiscard]]
auto allocate(util::allocator_type auto &alloc, u32 count) -> Ptr {
  if (count == 0) { return nullptr; }
  u32 constexpr offset{}, flags{};
  return static_cast<Ptr>(
    alloc.allocate(count * sizeof(T), alignof(T), offset, flags)
  );
}

template<class T, class Ptr = pure_pointer_t<T>>
[[nodiscard]]
auto allocate_move(
  util::allocator_type auto &alloc,
  Ptr                        from,
  Ptr                        to,
  u32 const                  count
) -> Ptr {
  if (auto raw{ allocate<T, Ptr>(alloc, count) }; raw) {
    if constexpr (std::is_trivially_copyable_v<T>) {
      std::memcpy(raw, from, sizeof(T) * std::distance(from, to));
    } else {
      std::move(from, to, raw);
    }
    return raw;
  }
  return nullptr;
}

template<class T, class Ptr = pure_pointer_t<T>>
[[nodiscard]]
auto allocate_move(util::allocator_type auto &alloc, Ptr from, Ptr to) -> Ptr {
  return allocate_move<T, Ptr>(alloc, from, to, std::distance(from, to));
}

template<class T, class Ptr = pure_pointer_t<T>>
[[nodiscard]]
auto allocate_copy(
  util::allocator_type auto &alloc,
  util::iterator_type auto   from,
  util::iterator_type auto   to,
  u32 const                  count
) -> Ptr {
  if (auto raw{ allocate<T, Ptr>(alloc, count) }; raw) {
    if constexpr (std::is_trivially_copyable_v<T>) {
      std::memcpy(raw, from, sizeof(T) * std::distance(from, to));
    } else {
      std::copy(from, to, raw);
    }
    return raw;
  }
  return nullptr;
}

template<class T, class Ptr = pure_pointer_t<T>>
[[nodiscard]]
auto allocate_copy(util::allocator_type auto &alloc, Ptr from, Ptr to) -> Ptr {
  return allocate_copy<T, Ptr>(alloc, from, to, std::distance(from, to));
}

} // namespace mem

namespace algo {

[[nodiscard]]
constexpr auto get_last(util::range_type auto &range) {
  return std::data(range) + (std::size(range) - 1);
}

[[nodiscard]]
constexpr auto swap_erase(
  util::range_type auto   &range,
  util::iterator_type auto target
) {
  gzn_assertion(
    target < std::end(range) && target >= std::begin(range),
    "'target' is out of range"
  );

  std::iter_swap(get_last(range), target);
  return std::prev(std::end(range));
}

[[nodiscard]]
constexpr auto shift(
  util::range_type auto   &range,
  util::iterator_type auto target,
  u32                      shift = 1
) {
  gzn_assertion(
    target < std::end(range) && target >= std::begin(range),
    "'target' is out of range"
  );

  for (auto last{ get_last(range) }; last != target; --last) {
    std::iter_swap(last, last + shift);
  }
  return target;
}

[[nodiscard]]
constexpr auto shift_erase(
  util::range_type auto   &range,
  util::iterator_type auto from,
  util::iterator_type auto to
) {
  gzn_assertion(from < to, "The iterator pairs 'from' & 'to' are invalid");
  gzn_assertion(
    from < std::end(range) && from >= std::begin(range),
    "'from' is out of range"
  );
  gzn_assertion(
    to < std::end(range) && to >= std::begin(range), "'to' is out of range"
  );

  auto const last{ get_last(range) };
  auto const count{ static_cast<usize>(std::distance(from, to)) };
  for (auto tail{ std::distance(to, last) }; tail; ++from, --tail) {
    std::iter_swap(from, from + count);
  }
  return from;
}

} // namespace algo

} // namespace gzn::fnd::containers
