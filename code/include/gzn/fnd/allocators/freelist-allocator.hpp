#pragma once

#include "gzn/fnd/allocators/allocators-common.hpp"

namespace gzn::fnd {

template<util::allocator_type Base, usize MaxBytesCount = 2048>
class freelist_allocator : private Base {
public:
  constexpr explicit freelist_allocator() noexcept        = default;

  freelist_allocator(freelist_allocator const &other)     = default;
  freelist_allocator(freelist_allocator &&other) noexcept = default;

  auto operator=(freelist_allocator const &other)
    -> freelist_allocator & = default;
  auto operator=(freelist_allocator &&other) noexcept
    -> freelist_allocator & = default;

  [[nodiscard]]
  constexpr auto allocate(
    memory_block_size          size,
    std::source_location const loc = std::source_location::current()
  ) noexcept -> memory_block {
    usize const required_size{ util::good_size(size) };
    auto        found{ find_block_for(required_size) };
    if (!found) {
      size.bytes_count = std::min(size.bytes_count, sizeof(free_block));
      return Base::allocate(size, loc);
    }

    auto prev{ found->prev };
    auto next{ found->next };

    auto data{ reinterpret_cast<byte *>(found) };
    if (found->bytes_count - required_size >= sizeof(free_block)) {
      prev->next = new (data + required_size) free_block{
        .prev        = prev,
        .next        = next,
        .bytes_count = found->bytes_count - required_size,
      };
      next->prev = prev->next;
    } else {
      prev->next = next;
      next->prev = prev;
    }

    return memory_block{
      .address = data + (size.bytes_count - required_size),
      .size{ size },
    };
  }

  [[nodiscard]]
  constexpr auto expand(
    memory_block              &block,
    usize const                additonal_bytes,
    std::source_location const loc = std::source_location::current()
  ) noexcept -> bool {
    return Base::expand(block, additonal_bytes, loc);
  }

  constexpr void deallocate(
    memory_block              &block,
    std::source_location const loc
  ) {
    usize const bytes_count{ block.size.bytes_count };
    if (total_bytes_occupied + bytes_count > MaxBytesCount) [[unlikely]] {
      return Base::deallocate(block, loc);
    }

    total_bytes_occupied += bytes_count;
    if (head == nullptr) [[unlikely]] {
      head = new (block.address) free_block{
        .bytes_count = bytes_count,
      };
      return;
    }

    head->prev = new (block.address) free_block{
      .next        = head,
      .prev        = nullptr,
      .bytes_count = bytes_count,
    };
    head  = head->prev;

    block = {};
  }

  [[nodiscard]]
  constexpr auto owns(memory_block const block) noexcept -> bool {
    return Base::owns(block);
  }

private:
  struct free_block {
    free_block *prev{ nullptr };
    free_block *next{ nullptr };
    usize       bytes_count{};
  };

  free_block *head{ nullptr };
  usize       total_bytes_occupied{};

  auto find_block_for(usize const &bytes_count) -> free_block * {
    free_block *curr{ head };
    while (curr != nullptr) {
      if (curr->bytes_count >= bytes_count) { break; }
      curr = curr->next;
    }
    return curr;
  }
};

} // namespace gzn::fnd
