#include "gzn/fnd/allocators.hpp"

#include <cstddef>

#include <mimalloc.h>

#include "gzn/fnd/assert.hpp"

namespace gzn::fnd {

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-= base_allocator =-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// //
//
auto base_allocator::allocate(
  u32 const bytes_count,
  [[maybe_unused]] u32 const
) -> void * {
  gzn_assertion(bytes_count != 0, "Meaningless allocate(0, ?) call");
  return mi_malloc(bytes_count);
}

auto base_allocator::allocate(
  u32 const bytes_count,
  u32 const alignment,
  u32 const offset,
  [[maybe_unused]] u32 const
) -> void * {
  gzn_assertion(bytes_count != 0, "Meaningless allocate(0, ?) call");
  return mi_malloc_aligned_at(bytes_count, alignment, offset);
}

void base_allocator::deallocate(void *memory, [[maybe_unused]] u32 const) {
  gzn_assertion(memory != nullptr, "Attempt to deallocate nullptr!");
  mi_free(memory);
}

void base_allocator::deallocate(void *memory, u32 count, u32 alignment) {
  gzn_assertion(memory != nullptr, "Attempt to deallocate nullptr!");
  mi_free_size_aligned(memory, count, alignment);
}

} // namespace gzn::fnd
