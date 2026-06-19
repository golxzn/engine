#pragma once

#include "gzn/fnd/allocators/allocators-common.hpp"

namespace gzn::fnd {

template<util::allocator_type Primary, util::allocator_type Fallback>
class fallback_allocator
  : private Primary
  , private Fallback {
public:
  constexpr explicit fallback_allocator() noexcept                  = default;

  constexpr fallback_allocator(fallback_allocator const &other)     = default;
  constexpr fallback_allocator(fallback_allocator &&other) noexcept = default;

  constexpr auto operator=(fallback_allocator const &other)
    -> fallback_allocator & = default;
  constexpr auto operator=(fallback_allocator &&other) noexcept
    -> fallback_allocator & = default;

  [[nodiscard]]
  constexpr auto allocate(
    memory_block_size const    size,
    std::source_location const loc = std::source_location::current()
  ) -> memory_block {
    if (auto block{ Primary::allocate(size, loc) }; block) { return block; }
    return Fallback::allocate(size, loc);
  }

  constexpr auto expand(
    memory_block              &block,
    usize                      additional_bytes,
    std::source_location const loc = std::source_location::current()
  ) -> bool {
    return Primary::owns(block)
           ? Primary::expand(block, additional_bytes, loc)
           : Fallback::expand(block, additional_bytes, loc);
  }

  constexpr void deallocate(
    memory_block              &block,
    std::source_location const loc = std::source_location::current()
  ) {
    return Primary::owns(block) ? Primary::deallocate(block, loc)
                                : Fallback::deallocate(block, loc);
  }

  [[nodiscard]]
  constexpr auto owns(memory_block const block) noexcept -> bool {
    return Primary::owns(block) || Fallback::owns(block);
  }
};

} // namespace gzn::fnd
