#pragma once

#include "gzn/fnd/allocators/allocators-common.hpp"

namespace gzn::fnd {

template<util::allocator_type Base, u32 BlockSize = 64, u32 BlocksCount = 64>
class pool_allocator : private Base {
public:
  static constexpr usize pool_bytes_count{ BlockSize * BlocksCount };

  constexpr explicit pool_allocator(
    std::source_location const loc = std::source_location::current()
  ) noexcept
    : block{ Base::allocate({ pool_bytes_count }, loc) } {
    gzn_assertion(
      block.address == nullptr, "Failed to allocate required block!"
    );
  }

  ~pool_allocator() { Base::deallocate(block); }

  pool_allocator(pool_allocator const &other)                     = delete;
  pool_allocator(pool_allocator &&other) noexcept                 = default;

  auto operator=(pool_allocator const &other) -> pool_allocator & = delete;
  auto operator=(pool_allocator &&other) noexcept
    -> pool_allocator & = default;

  [[nodiscard]]
  constexpr auto allocate(
    memory_block_size const size,
    std::source_location const
  ) noexcept -> memory_block {
    if (util::good_size(size) > BlockSize) {
      gzn_do_assertion(
        "The requested allocation size is larger than BlockSize!"
      );
      return {};
    }

    if (taken_blocks_count <= BlocksCount) [[likely]] {
      auto place{ *idx2addr<u32>(taken_blocks_count) };
      *place = ++taken_blocks_count;
    }

    if (freed_blocks_count < 0) [[unlikely]] { return {}; }

    u32 const next_index{ *reinterpret_cast<u32 *>(next) };
    void     *data{ next };

    --freed_blocks_count;
    next = freed_blocks_count != 0 ? idx2addr<byte>(next_index) : nullptr;

    return memory_block{
      .address = data,
      .size{ size },
    };
  }

  [[nodiscard]]
  constexpr auto expand(
    memory_block &block,
    usize const   additional_bytes,
    std::source_location const
  ) noexcept -> bool {
    usize const new_bytes_count{ block.size.bytes_count + additional_bytes };
    block.size.bytes_count = std::min<usize>(new_bytes_count, BlockSize);
    return block.size.bytes_count == new_bytes_count;
  }

  constexpr void deallocate(memory_block &block, std::source_location const) {
    if (freed_blocks_count == 0) [[unlikely]] { return; }

    *static_cast<u32 *>(block.address) = next != nullptr ? addr2idx(next)
                                                         : BlocksCount;
    next                               = static_cast<byte *>(block.address);
    ++freed_blocks_count;
    block = {};
  }

  [[nodiscard]]
  constexpr auto owns(memory_block const block) noexcept -> bool {
    return Base::owns(block);
  }

private:
  memory_block block;
  byte        *start{ block.as_bytes() };
  byte        *next{ start };
  usize        taken_blocks_count{};
  usize        freed_blocks_count{ BlocksCount };

  template<class T>
  auto idx2addr(u32 const index) noexcept -> T * {
    return reinterpret_cast<T *>(start + (index * BlockSize));
  }

  auto addr2idx(byte const *p) const -> u32 {
    return static_cast<u32>((static_cast<usize>(p - start)) / BlockSize);
  }
};

} // namespace gzn::fnd
