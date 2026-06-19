#pragma once

#include "gzn/fnd/allocators/allocators-common.hpp"

namespace gzn::fnd {

template<usize BytesCount>
class in_stack_allocator {
public:
  constexpr explicit in_stack_allocator() noexcept                  = default;

  constexpr in_stack_allocator(in_stack_allocator const &other)     = default;
  constexpr in_stack_allocator(in_stack_allocator &&other) noexcept = default;

  constexpr auto operator=(in_stack_allocator const &other)
    -> in_stack_allocator & = default;
  constexpr auto operator=(in_stack_allocator &&other) noexcept
    -> in_stack_allocator & = default;

  [[nodiscard]]
  constexpr auto allocate(
    memory_block_size const                     size,
    [[maybe_unused]] std::source_location const loc =
      std::source_location::current()
  ) -> memory_block {
    auto const [total_memory_required, offset]{
      util::good_size_and_offset(size)
    };
    if (total_memory_required > available_bytes_count()) {
      // This asserting is not necessary since not-enough-space isn't an error.
      // gzn_do_assertion("Not enough stack storage for this allocation");
      return {};
    }

    auto address{ &data[top + offset] };
    top += total_memory_required;
    return memory_block{
      .address = address,
      .size{ size },
    };
  }

  constexpr auto expand(
    memory_block                               &block,
    usize                                       additional_bytes,
    [[maybe_unused]] std::source_location const loc =
      std::source_location::current()
  ) -> bool {
    if (block.address == nullptr) [[unlikely]] {
      gzn_do_assertion("Attempt to expand empty block!");
      return false;
    }
    auto const block_top{ block.as_bytes() + block.size.bytes_count };
    if (block_top != &data[top]) [[unlikely]] {
      gzn_do_assertion(
        "Attempt to expand block which is not the latest allocated one!"
      );
      return false;
    }

    auto const new_bytes_count{ block.size.bytes_count + additional_bytes };
    if (new_bytes_count > available_bytes_count()) [[unlikely]] {
      // This asserting is not necessary since not-enough-space isn't an error.
      // gzn_do_assertion("Not enough space for block extention");
      return false;
    }

    top                    += additional_bytes;
    block.size.bytes_count += additional_bytes;
    return true;
  }

  constexpr void deallocate(
    memory_block                               &block,
    [[maybe_unused]] std::source_location const loc =
      std::source_location::current()
  ) {
    gzn_assertion(
      block.address == nullptr || block.size.bytes_count == 0u,
      "Attempt to expand empty block!"
    );

    auto const block_top{ block.as_bytes() + block.size.bytes_count };
    if (block_top != &data[top]) [[unlikely]] {
      gzn_do_assertion(
        "Attempt to deallocate block which is not the latest allocated one!"
      );
      return;
    }

    top   -= util::good_size(block.size);
    block  = {};
  }

  [[nodiscard]]
  constexpr auto owns(memory_block const block) noexcept -> bool {
    return block.address >= begin() && block.address < end();
  }

  constexpr void reset() { top = 0; }

  [[nodiscard]]
  static constexpr auto total_size() noexcept -> usize {
    return BytesCount;
  }

  [[nodiscard]]
  constexpr auto available_bytes_count() const noexcept -> usize {
    return BytesCount - top;
  }

  [[nodiscard]]
  constexpr auto allocated_bytes_count() const noexcept -> usize {
    return top;
  }

  [[nodiscard]]
  constexpr auto begin() const noexcept {
    return std::data(data);
  }

  [[nodiscard]]
  constexpr auto end() const noexcept {
    return std::data(data) + BytesCount;
  }

private:
  std::array<byte, BytesCount> data{};
  usize                        top{};
};


} // namespace gzn::fnd
