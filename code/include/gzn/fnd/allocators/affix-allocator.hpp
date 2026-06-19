#pragma once

#include "gzn/fnd/allocators/allocators-common.hpp"

namespace gzn::fnd {

template<util::allocator_type Base, class Prefix, class Postfix = void>
class affix_allocator : private Base {
  // gzn_static_assert(
  //   !std::is_void_v<Prefix> && !std::is_bitwise_movable<Prefix>,
  //   "Prefix must be trivially relocatable"
  // );
  // gzn_static_assert(
  //   !std::is_void_v<Postfix> && !std::is_bitwise_movable<Postfix>,
  //   "Postfix must be trivially relocatable"
  // );

public:
  static constexpr memory_block_size prefix{ util::size_of<Prefix>() };
  static constexpr memory_block_size postfix{ util::size_of<Postfix>() };
  static constexpr usize             prefix_size{ util::good_size(prefix) };
  static constexpr usize             postfix_size{ util::good_size(postfix) };

  [[nodiscard]]
  static constexpr auto get_real_block_of(memory_block const block) {
    if (!block) { return block; }

    auto const [required_size, offset]{
      util::good_size_and_offset(block.size)
    };
    auto const alignment{
      (std::max)({ prefix.alignment, block.size.alignment, postfix.alignment })
    };
    return memory_block{
      .address = block.as_bytes() - prefix_size - offset,
      .size{
            .bytes_count = prefix_size + required_size + postfix_size,
            .alignment   = alignment,
            },
    };
  }

  constexpr explicit affix_allocator() noexcept               = default;

  constexpr affix_allocator(affix_allocator const &other)     = default;
  constexpr affix_allocator(affix_allocator &&other) noexcept = default;

  constexpr auto operator=(affix_allocator const &other)
    -> affix_allocator & = default;
  constexpr auto operator=(affix_allocator &&other) noexcept
    -> affix_allocator & = default;

  [[nodiscard]]
  constexpr auto allocate(
    memory_block_size const    size,
    std::source_location const loc = std::source_location::current()
  ) -> memory_block {
    auto const [required_size, offset]{ util::good_size_and_offset(size) };


    memory_block_size const total_size{
      .bytes_count = prefix_size + required_size + postfix_size,
      .alignment   = (std::max)({ prefix.alignment,
                                  size.alignment,
                                  postfix.alignment })
    };
    memory_block block{ Base::allocate(total_size, loc) };

    if constexpr (!std::is_void_v<Prefix>) {
      new (block.address) Prefix{ size, loc };
    }

    if constexpr (!std::is_void_v<Postfix>) {
      auto address{ block.as_bytes() + prefix_size + util::good_size(size) };
      new (address) Postfix{ size, loc };
    }

    return memory_block{
      .address = block.as_bytes() + prefix_size + offset,
      .size{ size },
    };
  }

  constexpr auto expand(
    memory_block              &block,
    usize const                additional_bytes,
    std::source_location const loc = std::source_location::current()
  ) -> bool {
    if constexpr (std::is_void_v<Postfix>) {
      return Base::expand(block, additional_bytes, loc);
    }

    Postfix postfix_copy;
    std::swap(
      postfix_copy,
      *reinterpret_cast<Postfix *>(
        block.as_bytes() + util::good_size(block.size)
      )
    );

    gzn_do_assertion("FILL ME WITH YOUR IMPLEMENTATION SEMPAI!");
    /// @todo

    return true;
  }

  constexpr void deallocate(
    memory_block              &block,
    std::source_location const loc = std::source_location::current()
  ) {
    memory_block whole_block{ get_real_block_of(block) };
    if constexpr (!std::is_void_v<Prefix>) {
      whole_block.as<Prefix>()->~Prefix();
    }
    if constexpr (!std::is_void_v<Postfix>) {
      auto address{ whole_block.as_bytes() + prefix_size +
                    util::good_size(block.size) };
      reinterpret_cast<Postfix *>(address)->~Postfix();
    }
    Base::deallocate(whole_block, loc);
    block = {};
  }

  [[nodiscard]]
  constexpr auto owns(memory_block const block) noexcept -> bool {
    return Base::owns(get_real_block_of(block));
  }
};

} // namespace gzn::fnd
