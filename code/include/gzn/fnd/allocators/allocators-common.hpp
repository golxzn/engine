#pragma once

#include <algorithm>
#include <concepts>
#include <source_location>

#include "gzn/fnd/assert.hpp"
#include "gzn/fnd/pointers.hpp"

namespace gzn::fnd {

struct memory_block_size {
  usize bytes_count{};
  usize alignment{ 1u };
};

struct memory_block {
  void             *address{ nullptr };
  memory_block_size size{};

  [[nodiscard]]
  gzn_inline constexpr decltype(auto) as_bytes(this auto &&self) noexcept {
    return reinterpret_cast<byte *>(self.address);
  }

  template<class T>
  [[nodiscard]]
  gzn_inline constexpr auto as() noexcept -> T * {
    return size.bytes_count >= sizeof(T) ? reinterpret_cast<T *>(address)
                                         : nullptr;
  }

  [[nodiscard]]
  constexpr operator bool() const noexcept {
    return address != nullptr;
  }
};

namespace util {

template<class T>
concept allocator_type = requires(T *allocator, memory_block b) {
  {
    allocator->allocate(memory_block_size{}, std::source_location{})
  } -> std::same_as<memory_block>;
  {
    allocator->expand(b, usize{}, std::source_location{})
  } -> std::same_as<bool>;
  { allocator->deallocate(b, std::source_location::current()) };
  { allocator->owns(memory_block{}) } -> std::same_as<bool>;
};

struct allocator_pointers {
  void *allocator{ nullptr };

  auto (*_allocate)(void *, memory_block_size, std::source_location)
    -> memory_block;
  auto (*_expand)(void *, memory_block &, usize, std::source_location) -> bool;
  void (*_deallocate)(void *, memory_block, std::source_location);
  auto (*_owns)(void *, memory_block) -> bool;

  template<allocator_type AllocType>
  [[nodiscard]]
  static constexpr auto make(AllocType &&alloc) -> allocator_pointers {
    return allocator_pointers{
      .allocator = &alloc,
      ._allocate{ [](auto alloc, auto sz, auto loc) {
        return static_cast<AllocType *>(alloc)->allocate(sz, loc);
      } },
      ._expand{ [](auto alloc, auto &memblock, auto sz, auto loc) {
        return static_cast<AllocType *>(alloc)->expand(memblock, sz, loc);
      } },
      ._deallocate{ [](auto alloc, auto memblock, auto loc) {
        return static_cast<AllocType *>(alloc)->deallocate(memblock, loc);
      } },
      ._owns{ [](auto alloc, auto memblock) {
        return static_cast<AllocType *>(alloc)->owns(memblock);
      } },
    };
  }

  [[nodiscard]]
  constexpr auto is_valid() const noexcept {
    return allocator == nullptr || _allocate == nullptr ||
           _expand == nullptr || _deallocate == nullptr || _owns == nullptr;
  }

  [[nodiscard]] operator bool() const noexcept { return is_valid(); }

  [[nodiscard]]
  constexpr auto allocate(
    memory_block_size const    size,
    std::source_location const loc = std::source_location::current()
  ) noexcept -> memory_block {
    gzn_assertion(
      !allocator || !_allocate, "FATAL! Invalid allocator pointer!"
    );
    return _allocate(allocator, size, loc);
  }

  [[nodiscard]]
  constexpr auto expand(
    memory_block              &block,
    usize const                additional_bytes,
    std::source_location const loc = std::source_location::current()
  ) noexcept -> bool {
    gzn_assertion(!allocator || !_expand, "FATAL! Invalid allocator pointer!");
    return _expand(allocator, block, additional_bytes, loc);
  }

  constexpr void deallocate(
    memory_block const         block,
    std::source_location const loc = std::source_location::current()
  ) {
    gzn_assertion(
      !allocator || !_deallocate, "FATAL! Invalid allocator pointer!"
    );
    _deallocate(allocator, block, loc);
  }

  [[nodiscard]]
  constexpr auto owns(memory_block const block) noexcept -> bool {
    gzn_assertion(!allocator || !_owns, "FATAL! Invalid allocator pointer!");
    return _owns(allocator, block);
  }
};

constexpr auto good_size(memory_block_size const size) noexcept -> usize {
  usize const alignment_mask{ std::min<usize>(size.alignment, 1u) - 1 };
  return (size.bytes_count + alignment_mask) & ~alignment_mask;
}

constexpr auto good_size_and_offset(memory_block_size const size) noexcept {
  usize const good{ good_size(size) };
  return std::make_pair(good, good - size.bytes_count);
}

constexpr auto alignment_offset(memory_block_size const size) {
  return good_size(size) - size.bytes_count;
}

template<class T>
[[nodiscard]]
constexpr auto size_of(usize const count = 1u) -> memory_block_size {
  if constexpr (!std::is_void_v<T>) {
    return memory_block_size{
      .bytes_count = sizeof(T) * count,
      .alignment   = alignof(T),
    };
  } else {
    return {};
  }
}

[[nodiscard]]
constexpr auto size_of(auto const &obj, usize const count = 1u)
  -> memory_block_size {
  using obj_type = std::remove_cvref_t<decltype(obj)>;
  return size_of<obj_type>(count);
}

template<class T>
[[nodiscard]]
constexpr auto block_of(not_null<T> ptr, usize const count = 1u) {
  return memory_block{
    .address = static_cast<void *>(ptr),
    .size    = size_of(ptr, count),
  };
}

template<class T>
[[nodiscard]]
gzn_inline auto alloc(
  util::allocator_type auto &allocator,
  usize const                count = 1u,
  std::source_location const loc   = std::source_location::current()
) -> void * {
  return allocator.allocate(size_of<T>(count), loc).template as<T>();
}

template<class T>
gzn_inline void dealloc(
  util::allocator_type auto &allocator,
  T                         *ptr,
  usize const                count = 1u,
  std::source_location const loc   = std::source_location::current()
) {
  return allocator.deallocate(block_of(ptr, count), loc);
}

template<class T, class... Args>
  requires std::constructible_from<T, Args &&...>
[[nodiscard]] gzn_inline auto construct(
  util::allocator_type auto &allocator,
  Args &&...args
) noexcept(std::is_nothrow_constructible_v<T, Args &&...>)
  -> std::add_pointer_t<std::decay_t<T>> {
  if (auto memory{ alloc<T>(allocator) }; memory) {
    return new (memory) T{ std::forward<Args>(args)... };
  }
  return nullptr;
}

template<class T>
gzn_inline void destroy(util::allocator_type auto &allocator, T *ptr) noexcept(
  std::is_nothrow_destructible_v<T>
) {
  gzn_assertion(ptr == nullptr, "Attempt to destroy nullptr");
  if constexpr (!std::is_trivially_destructible_v<T>) { ptr->~T(); }
  dealloc(allocator, ptr);
}

template<class T>
[[nodiscard]] constexpr decltype(auto) make_deletion_function(
  util::allocator_type auto &allocator
) {
  return [alloc{ &allocator }](T *ptr) {
    if (ptr == nullptr) { return; }
    auto ptr_block{ block_of<T>(not_null<T>{ ptr }) };
    alloc->deallocate(ptr_block);
  };
}

} // namespace util

} // namespace gzn::fnd
