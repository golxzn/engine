#pragma once

#include <mutex>

#include "gzn/fnd/allocators/allocators-common.hpp"
#include "gzn/fnd/threading/mutex-common.hpp"

namespace gzn::fnd {

template<util::allocator_type Base, util::mutex_like Mutex = dummy_mutex>
class thread_safe_allocator : private Base {
public:
  constexpr explicit thread_safe_allocator() noexcept           = default;

  thread_safe_allocator(thread_safe_allocator const &other)     = delete;
  thread_safe_allocator(thread_safe_allocator &&other) noexcept = default;

  auto operator=(thread_safe_allocator const &other)
    -> thread_safe_allocator & = delete;
  auto operator=(thread_safe_allocator &&other) noexcept
    -> thread_safe_allocator & = default;

  [[nodiscard]]
  constexpr auto allocate(
    memory_block_size const    size,
    std::source_location const loc = std::source_location::current()
  ) noexcept -> memory_block {
    std::lock_guard lock{ memory_guard };
    return Base::allocate(size, loc);
  }

  [[nodiscard]]
  constexpr auto expand(
    memory_block              &block,
    usize const                additional_bytes,
    std::source_location const loc = std::source_location::current()
  ) noexcept -> bool {
    std::lock_guard lock{ memory_guard };
    return Base::expand(block, additional_bytes, loc);
  }

  constexpr void deallocate(
    memory_block              &block,
    std::source_location const loc = std::source_location::current()
  ) {
    std::lock_guard lock{ memory_guard };
    Base::deallocate(block, loc);
  }

  [[nodiscard]]
  constexpr auto owns(memory_block const block) noexcept -> bool {
    return Base::owns(block);
  }

private:
  Mutex memory_guard{};
};

} // namespace gzn::fnd
