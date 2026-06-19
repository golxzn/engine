#pragma once

#include "gzn/fnd/allocators/allocators-common.hpp"

namespace gzn::fnd {

template<
  usize                Threshold,
  util::allocator_type Small,
  util::allocator_type Large>
class segregate_allocator
  : private Small
  , private Large {
public:
  constexpr explicit segregate_allocator() noexcept               = default;

  constexpr segregate_allocator(segregate_allocator const &other) = default;
  constexpr segregate_allocator(segregate_allocator &&other) noexcept =
    default;

  constexpr auto operator=(segregate_allocator const &other)
    -> segregate_allocator & = default;
  constexpr auto operator=(segregate_allocator &&other) noexcept
    -> segregate_allocator & = default;

  [[nodiscard]]
  constexpr auto allocate(
    memory_block_size const    size,
    std::source_location const loc = std::source_location::current()
  ) -> memory_block {
    return util::good_size(size) <= Threshold ? Small::allocate(size, loc)
                                              : Large::allocate(size, loc);
  }

  constexpr auto expand(
    memory_block              &block,
    usize                      additional_bytes,
    std::source_location const loc = std::source_location::current()
  ) -> bool {
    return Small::owns(block) ? Small::expand(block, additional_bytes, loc)
                              : Large::expand(block, additional_bytes, loc);
  }

  constexpr void deallocate(
    memory_block              &block,
    std::source_location const loc = std::source_location::current()
  ) {
    return Small::owns(block) ? Small::deallocate(block, loc)
                              : Large::deallocate(block, loc);
  }

  [[nodiscard]]
  constexpr auto owns(memory_block const block) noexcept -> bool {
    return Small::owns(block) || Large::owns(block);
  }

  [[nodiscard]]
  static constexpr auto threshold() noexcept -> usize {
    return Threshold;
  }
};

} // namespace gzn::fnd
