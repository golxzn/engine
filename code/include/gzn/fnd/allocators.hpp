#pragma once

#include <array>
#include <concepts>
#include <string_view>
#include <utility>

#include "gzn/fnd/assert.hpp"
#include "gzn/fnd/definitions.hpp"

namespace gzn::fnd {

namespace util {

template<class T>
concept allocator_type = requires(T *allocator) {
  { allocator->allocate(u32{}, u32{}) } -> std::same_as<void *>;
  { allocator->allocate(u32{}, u32{}, u32{}, u32{}) } -> std::same_as<void *>;
  { allocator->deallocate((void *)0, u32{}) };
  { allocator->deallocate((void *)0, u32{}, u32{}) };
  { allocator->get_label() } -> std::convertible_to<std::string_view>;
} && std::constructible_from<cstr>;

constexpr auto memory_align(u32 const size, u32 const alignment) noexcept
  -> u32 {
  u32 const alignment_mask{ alignment - 1 };
  return (size + alignment_mask) & ~alignment_mask;
}

template<class T>
[[nodiscard]]
gzn_inline auto alloc(util::allocator_type auto &allocator) -> void * {
  return static_cast<T *>(allocator.allocate(sizeof(T), alignof(T), 0u, 0u));
}

template<class T>
gzn_inline void dealloc(util::allocator_type auto &allocator, T *ptr) {
  return allocator.deallocate(static_cast<void *>(ptr), sizeof(T), alignof(T));
}

template<class T>
[[nodiscard]]
gzn_inline auto alloc(util::allocator_type auto &allocator, usize const count)
  -> void * {
  usize const bytes_count{ sizeof(T) * count };
  return static_cast<T *>(allocator.allocate(bytes_count, alignof(T), 0u, 0u));
}

template<class T>
gzn_inline void dealloc(
  util::allocator_type auto &allocator,
  T                         *ptr,
  usize const                count
) {
  return allocator.deallocate(
    static_cast<void *>(ptr), sizeof(T) * count, alignof(T)
  );
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
  gzn_assertion(ptr != nullptr, "Attempt to destroy nullptr");
  if constexpr (!std::is_trivially_destructible_v<T>) { ptr->~T(); }
  dealloc(allocator, ptr);
}

template<class T>
struct default_deleter {
  void operator()(util::allocator_type auto &allocator, T *value) {
    destroy(allocator, value);
  }
};

} // namespace util

class dummy_allocator {
public:
  constexpr explicit dummy_allocator(
    [[maybe_unused]] cstr label = "dummy"
  ) noexcept {}

  dummy_allocator(dummy_allocator const &other)                     = default;
  dummy_allocator(dummy_allocator &&other) noexcept                 = default;

  auto operator=(dummy_allocator const &other) -> dummy_allocator & = default;
  auto operator=(dummy_allocator &&other) noexcept
    -> dummy_allocator & = default;

  [[nodiscard]]
  constexpr auto allocate(u32, u32) -> void * {
    return nullptr;
  }

  [[nodiscard]]
  constexpr auto allocate(u32, u32, u32, u32) -> void * {
    return nullptr;
  }

  constexpr void deallocate(void *, u32) {}

  constexpr void deallocate(void *, u32, u32) {}

  [[nodiscard]]
  constexpr auto get_label() const noexcept -> std::string_view {
    return "dummy";
  }
};

class base_allocator {
public:
  constexpr explicit base_allocator(cstr label = "base_allocator") noexcept
    : label{ label } {}

  base_allocator(base_allocator const &other)                     = default;
  base_allocator(base_allocator &&other) noexcept                 = default;

  auto operator=(base_allocator const &other) -> base_allocator & = default;
  auto operator=(base_allocator &&other) noexcept
    -> base_allocator & = default;

  [[nodiscard]]
  auto allocate(u32 bytes_count, u32 flags = 0) -> void *;

  [[nodiscard]]
  auto allocate(u32 bytes_count, u32 alignment, u32 offset, u32 flags = 0)
    -> void *;

  void deallocate(void *memory, u32 count);

  void deallocate(void *memory, u32 count, u32 alignment);

  [[nodiscard]]
  constexpr auto get_label() const noexcept -> std::string_view {
    return label;
  }

private:
  cstr label;
};

template<usize BytesCount>
class stack_arena_allocator {
public:
  constexpr explicit stack_arena_allocator(
    cstr const label = "monotonic_stack_allocator"
  ) noexcept
    : label{ label } {}

  constexpr stack_arena_allocator(stack_arena_allocator const &other) =
    default;
  constexpr stack_arena_allocator(stack_arena_allocator &&other) noexcept =
    default;

  constexpr auto operator=(stack_arena_allocator const &other)
    -> stack_arena_allocator & = default;
  constexpr auto operator=(stack_arena_allocator &&other) noexcept
    -> stack_arena_allocator & = default;

  [[nodiscard]]
  constexpr auto allocate(
    u32 const                  bytes_count,
    [[maybe_unused]] u32 const flags = 0
  ) -> void * {

    gzn_assertion(
      bytes_count + top >= BytesCount,
      "Not enough stack storage for this allocation"
    );
    if (bytes_count + top >= BytesCount) { return nullptr; }
    auto const old_top{ std::exchange(top, top + bytes_count) };
    return &data[old_top];
  }

  [[nodiscard]]
  constexpr auto allocate(
    u32 const                  bytes_count,
    u32 const                  alignment,
    u32 const                  offset,
    [[maybe_unused]] u32 const flags = 0
  ) -> void * {
    auto const aligned_count{ util::memory_align(bytes_count, alignment) };
    auto const total_memory_required{ offset + aligned_count };

    if (top + total_memory_required >= BytesCount) {
      gzn_do_assertion("Not enough stack storage for this allocation");
      return nullptr;
    }

    auto const old_top{ std::exchange(top, top + total_memory_required) };
    return &data[offset + old_top];
  }

  constexpr void deallocate(void *, u32) {}

  constexpr void deallocate(void *, u32, u32) {}

  constexpr void reset() { top = 0; }

  [[nodiscard]]
  constexpr auto available_bytes_count() const noexcept -> usize {
    return BytesCount - top;
  }

  [[nodiscard]]
  constexpr auto allocated_bytes_count() const noexcept -> usize {
    return top;
  }

  [[nodiscard]]
  constexpr auto get_label() const noexcept -> std::string_view {
    return label;
  }

private:
  std::array<byte, BytesCount> data{};
  usize                        top{};
  cstr                         label;
};


} // namespace gzn::fnd
