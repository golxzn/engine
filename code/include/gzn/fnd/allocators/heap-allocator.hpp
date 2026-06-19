#pragma once

#if defined(GZN_DEBUG)
#  include <thread>
#endif // defined(GZN_DEBUG)


#include "gzn/fnd/allocators/allocators-common.hpp"

namespace gzn::fnd {

inline constexpr usize MAX_HEAPS_PER_THREAD{ 32 };
using heap_idx = usize;

class heap_allocator {
public:
  explicit heap_allocator() noexcept;

  heap_allocator(heap_allocator const &other)                     = default;
  heap_allocator(heap_allocator &&other) noexcept                 = default;

  auto operator=(heap_allocator const &other) -> heap_allocator & = default;
  auto operator=(heap_allocator &&other) noexcept
    -> heap_allocator & = default;


  [[nodiscard]]
  auto allocate(
    memory_block_size const    size,
    std::source_location const loc = std::source_location::current()
  ) noexcept -> memory_block;

  [[nodiscard]]
  auto expand(
    memory_block              &block,
    usize const                additional_bytes,
    std::source_location const loc = std::source_location::current()
  ) noexcept -> bool;

  void deallocate(
    memory_block              &block,
    std::source_location const loc = std::source_location::current()
  );

  [[nodiscard]]
  auto owns(memory_block const block) noexcept -> bool;

  [[nodiscard]]
  constexpr auto is_valid() const noexcept -> bool {
    return idx < MAX_HEAPS_PER_THREAD;
  }

private:
  heap_idx idx;
#if defined(GZN_DEBUG)
  std::thread::id thread_id;

  void ensure_same_thread() const {
    if (std::this_thread::get_id() != thread_id) [[unlikely]] {
      gzn_do_assertion("Attempt to allocate on heap from another thread!");
      std::terminate();
    }
  }
#endif // defined(GZN_DEBUG)
};

} // namespace gzn::fnd
