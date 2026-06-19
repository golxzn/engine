#pragma once

#include "gzn/fnd/allocators/allocators-common.hpp"
#include "gzn/fnd/assert.hpp"

namespace gzn::fnd {

#define __DUMMY_OOPS(MSG)                                                    \
  gzn_do_assertion(                                                          \
    MSG                                                                      \
    " request for dummy allocator is prohibited! Use another allocator one!" \
  );

class dummy_allocator {
public:
  constexpr explicit dummy_allocator() noexcept               = default;

  constexpr dummy_allocator(dummy_allocator const &other)     = default;
  constexpr dummy_allocator(dummy_allocator &&other) noexcept = default;

  constexpr auto operator=(dummy_allocator const &other)
    -> dummy_allocator & = default;
  constexpr auto operator=(dummy_allocator &&other) noexcept
    -> dummy_allocator & = default;

  [[nodiscard]]
  constexpr auto allocate(
    memory_block_size const,
    std::source_location const loc = std::source_location::current()
  ) noexcept -> memory_block {
    __DUMMY_OOPS("Allocate");
    return {};
  }

  [[nodiscard]]
  constexpr auto expand(
    memory_block &,
    usize,
    std::source_location const loc = std::source_location::current()
  ) noexcept -> bool {
    __DUMMY_OOPS("Expand");
    return false;
  }

  constexpr void deallocate(
    memory_block &,
    std::source_location const loc = std::source_location::current()
  ) {
    __DUMMY_OOPS("Free");
  }

  [[nodiscard]]
  constexpr auto owns(memory_block const block) noexcept -> bool {
    return block.address == nullptr;
  }
};

static dummy_allocator g_dummy_alloc{};

#undef __DUMMY_OOPS

} // namespace gzn::fnd
