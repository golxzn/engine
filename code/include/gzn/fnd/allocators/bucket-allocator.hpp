#pragma once

#include <algorithm>
#include <array>

#include "gzn/fnd/allocators/allocators-common.hpp"
#include "gzn/fnd/util/algorithms.hpp"

namespace gzn::fnd {

/**
 * @example
 * using assets_allocator = bucket_allocator<page_allocator<mmap_allocator>,
 *     64_B,
 *    256_B,
 *    1_MiB,
 *    4_MiB,
 *    8_MiB,
 *   16_MiB
 * >;
 */
template<util::allocator_type Allocator, usize... BucketThesholds>
class bucket_allocator {
public:
  static constexpr usize buckets_count{ sizeof...(BucketThesholds) };
  static constexpr fnd::span<usize const> bucket_sizes{
    { BucketThesholds... }
  };

  gzn_static_assert(
    !util::is_sorted(bucket_sizes),
    "Bucket thesholds must be sorted!"
  );

  constexpr explicit bucket_allocator() noexcept                = default;

  constexpr bucket_allocator(bucket_allocator const &other)     = default;
  constexpr bucket_allocator(bucket_allocator &&other) noexcept = default;

  constexpr auto operator=(bucket_allocator const &other)
    -> bucket_allocator & = default;
  constexpr auto operator=(bucket_allocator &&other) noexcept
    -> bucket_allocator & = default;

  [[nodiscard]]
  constexpr auto allocate(
    memory_block_size const    size,
    std::source_location const loc = std::source_location::current()
  ) -> memory_block {
    if (auto alloc{ get_allocator_for(util::good_size(size)) }; alloc) {
      return alloc->allocate(size, loc);
    }
    return {};
  }

  constexpr auto expand(
    memory_block              &block,
    usize const                additional_bytes,
    std::source_location const loc = std::source_location::current()
  ) -> bool {
    usize const old_bytes_count{ util::good_size(block.size) };

    auto current_allocator{ get_allocator_for(old_bytes_count) };
    if (!current_allocator) [[unlikely]] { return false; }


    usize const new_bytes_count{ old_bytes_count + additional_bytes };

    auto next_allocator{ get_allocator_for(new_bytes_count) };
    if (next_allocator == nullptr) [[unlikely]] { return false; }
    if (current_allocator == next_allocator) {
      return current_allocator.expand(block, additional_bytes, loc);
    }

    auto new_block{ next_allocator.allocate(
      {
        .bytes_count = new_bytes_count,
        .alignment   = block.size.alignment,
      },
      loc
    ) };
    if (new_block.data == nullptr) [[unlikely]] { return false; }

    std::memcpy(new_block.data, block.address, block.size.bytes_count);

    current_allocator.deallocate(block, loc);
    block = new_block;
    return true;
  }

  constexpr void deallocate(
    memory_block              &block,
    std::source_location const loc = std::source_location::current()
  ) {
    if (auto alloc{ get_allocator_for(util::good_size(block.size)) }; alloc) {
      alloc->deallocate(block, loc);
    }
  }

  [[nodiscard]]
  constexpr auto owns(memory_block const block) noexcept -> bool {
    if (auto alloc{ get_allocator_for(util::good_size(block.size)) }; alloc) {
      return alloc->owns(block);
    }
    return false;
  }

private:
  std::array<Allocator, buckets_count> buckets{};

  auto get_allocator_for(usize const bytes_count) noexcept -> Allocator * {
    auto const sizes_begin{ std::begin(bucket_sizes) };
    auto const sizes_end{ std::end(bucket_sizes) };
    auto const found{ std::lower_bound(sizes_begin, sizes_end, bytes_count) };
    return found != std::end(bucket_sizes)
           ? std::data(buckets) + std::distance(sizes_begin, found)
           : nullptr;
  }
};

} // namespace gzn::fnd
