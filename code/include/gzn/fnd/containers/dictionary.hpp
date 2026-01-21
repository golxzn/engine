#pragma once


#include "gzn/fnd/allocators.hpp"
#include "gzn/fnd/hash.hpp"

namespace gzn::fnd {

template<class Key, class Value>
struct record {
  Key   key{};
  Value value{};
};

template<
  class Key,
  class Value,
  util::allocator_type Allocator = base_allocator>
class hash_map {
public:
  using record_type    = record<Key, Value>;
  using allocator_type = Allocator;

  using size_type      = u32;

  explicit hash_map(allocator_type &allocator, size_type capacity);

  template<u64 Length>
  explicit hash_map(c_array<record_type, Length> &&arr);
};

} // namespace gzn::fnd
