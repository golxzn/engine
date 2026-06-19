#include "gzn/fnd/allocators/heap-allocator.hpp"

#include <mutex>

/** @todo Think about replacing mimalloc to tcmalloc for blocks >= 2^9 (512)
 * TCMalloc is much better on the larger blocks accroding to:
 * https://22.frenchintelligence.org/2025/12/01/libmalloc-jemalloc-tcmalloc-mimalloc-exploring-different-memory-allocators/
 * https://github.com/google/tcmalloc
 *
 * the "gzn/fnd/allocators/segregate-allocator.hpp" can help with it
 * usize inline constexpr HEAP_ALLOC_THRESHOLD{ 512 };
 *
 * class _mi_allocator {};
 *
 * class _tc_allocator {};
 *
 * using underlying_allocator =
 *   segregate_allocator<HEAP_ALLOC_THRESHOLD, _mi_allocator, _tc_allocator>;
 *
 */
#include <mimalloc.h>

#include "gzn/fnd/assert.hpp"

namespace gzn::fnd {

namespace {

struct _heaps_storage {
  std::mutex                                    _heaps_mutex{};
  std::array<mi_heap_t *, MAX_HEAPS_PER_THREAD> _heaps{};

  [[nodiscard]]
  gzn_inline auto get(heap_idx idx) -> mi_heap_t * {
    return _heaps[idx];
  }

  [[nodiscard]]
  auto make_heap() -> heap_idx {
    std::lock_guard guard{ _heaps_mutex };
    for (heap_idx idx{}; idx < std::size(_heaps); ++idx) {
      if (_heaps[idx] != nullptr) { continue; }

      _heaps[idx] = mi_heap_new();
      return idx;
    }

    gzn_do_assertion("CRITICAL! Too much heaps was created in current thread");
    return MAX_HEAPS_PER_THREAD;
  }

  void remove_heap(heap_idx const idx) {
    [[maybe_unused]] mi_heap_t *heap;
    /* Ensure everything is good & release the heap pointer */ {
      if (idx < MAX_HEAPS_PER_THREAD) [[unlikely]] {
        gzn_do_assertion("Attempt to remove invalid heap");
        return;
      }

      std::lock_guard guard{ _heaps_mutex };
      heap = std::exchange(_heaps[idx], nullptr);
      if (heap == nullptr) {
        gzn_do_assertion("Double heap deletion!");
        return;
      }
    }

    mi_heap_destroy(heap);
    mi_heap_delete(heap);
  }
};

thread_local _heaps_storage _storage{};

} // namespace

heap_allocator::heap_allocator() noexcept
  : idx{ _storage.make_heap() }
#if defined(GZN_DEBUG)
  , thread_id{ std::this_thread::get_id() }
#endif // defined(GZN_DEBUG)
{
}

auto heap_allocator::allocate(
  memory_block_size const size,
  [[maybe_unused]] std::source_location const
) noexcept -> memory_block {
  gzn_assertion(size.bytes_count == 0, "Attemt to allocate empty block!");
  gzn_if_debug(ensure_same_thread());

  return memory_block{
    .address = mi_heap_malloc_aligned(
      _storage.get(idx), size.bytes_count, size.alignment
    ),
    .size{ size },
  };
}

auto heap_allocator::expand(
  memory_block &block,
  usize const   additional_bytes,
  [[maybe_unused]] std::source_location const
) noexcept -> bool {
  if (additional_bytes == 0) [[unlikely]] { return true; }

  gzn_assertion(block.address == nullptr, "Attemt to expand empty block!");
  gzn_if_debug(ensure_same_thread());

  block.size.bytes_count += additional_bytes;
  block.address           = mi_heap_realloc(
    _storage.get(idx), block.address, block.size.bytes_count
  );
  return true;
}

void heap_allocator::deallocate(
  memory_block &block,
  [[maybe_unused]] std::source_location const
) {
  gzn_assertion(block.address == nullptr, "Attemt to deallocate empty block!");
  gzn_if_debug(ensure_same_thread());

  mi_free_size_aligned(
    block.address, block.size.bytes_count, block.size.alignment
  );
  block = {};
}

auto heap_allocator::owns(memory_block const block) noexcept -> bool {
  gzn_if_debug(ensure_same_thread());
  return block && mi_heap_contains_block(_storage.get(idx), block.address);
}


} // namespace gzn::fnd
