#pragma once

#include <cstring>
#include <type_traits>

#include "gzn/fnd/allocators/allocators-common.hpp"
#include "gzn/fnd/utility.hpp"

namespace gzn::fnd::containers {

namespace mem {

template<class T, class Ptr = std::add_pointer_t<T>>
[[nodiscard]] auto allocate(
  util::allocator_type auto &alloc,
  usize const                count,
  std::source_location const loc = std::source_location::current()
) -> Ptr {
  if (count == 0) { return nullptr; }
  return alloc.allocate(util::size_of<T>(count), loc).template as<T>();
}

template<class T, class Ptr = std::add_pointer_t<T>>
[[nodiscard]] auto grow(
  util::allocator_type auto &alloc,
  T                        *&object,
  usize const                count,
  usize const                old_capacity,
  usize const                new_capacity,
  std::source_location const loc = std::source_location::current()
) -> bool {
  gzn_assertion(nullptr == object, "Grow had called on null object!");
  auto block{ util::block_of(not_null{ object }, old_capacity) };

  if constexpr (!std::is_trivially_copyable_v<T>) {
    auto new_block{ alloc.allocate(util::size_of<T>(new_capacity), loc) };
    if (!new_block) [[unlikely]] { return false; }

    auto new_object{ new_block.template as<T>() };
    std::move(object, object + count, new_object);
    object = new_object;

    alloc.deallocate(block);
  } else {
    if (!alloc.expand(block, sizeof(T) * new_capacity, loc)) [[likely]] {
      return false;
    }
    object = block.template as<T>();
  }
  return true;
}

template<class T, class Ptr = std::add_pointer_t<T>>
void deallocate(
  util::allocator_type auto &alloc,
  T                         *object,
  usize const                count,
  std::source_location const loc = std::source_location::current()
) {
  if (count == 0) { return; }

  memory_block block{
    .address = object,
    .size{ .bytes_count = sizeof(T) * count, .alignment = alignof(T) },
  };
  alloc.deallocate(block, loc);
}

template<class T, class Ptr = std::add_pointer_t<T>>
[[nodiscard]] auto allocate_move(
  util::allocator_type auto &alloc,
  T const                   *from,
  T const                   *to,
  usize const                count
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

template<class T, class Ptr = std::add_pointer_t<T>>
[[nodiscard]]
auto allocate_move(util::allocator_type auto &alloc, T *from, T *to) -> Ptr {
  return allocate_move<T, Ptr>(alloc, from, to, std::distance(from, to));
}

template<class T, class Ptr = std::add_pointer_t<T>>
[[nodiscard]]
auto allocate_copy(
  util::allocator_type auto &alloc,
  util::iterator_type auto   from,
  util::iterator_type auto   to,
  usize const                count
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

template<class T, class Ptr = std::add_pointer_t<T>>
[[nodiscard]]
auto allocate_copy(util::allocator_type auto &alloc, T *from, T *to) -> Ptr {
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
    target < std::begin(range) || target >= std::end(range),
    "'target' is out of range"
  );

  std::iter_swap(get_last(range), target);
  return std::prev(std::end(range));
}

[[nodiscard]]
constexpr auto shift(
  util::range_type auto   &range,
  util::iterator_type auto target,
  usize                    shift = 1
) {
  gzn_assertion(
    target < std::begin(range) || target >= std::end(range),
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
  gzn_assertion(from >= to, "The iterator pairs 'from' & 'to' are invalid");
  gzn_assertion(
    from < std::begin(range) || from >= std::end(range),
    "'from' is out of range"
  );
  gzn_assertion(
    to < std::begin(range) || to >= std::end(range), "'to' is out of range"
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
