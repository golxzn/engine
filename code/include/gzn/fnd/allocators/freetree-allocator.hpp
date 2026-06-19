#pragma once

#include "gzn/fnd/allocators/allocators-common.hpp"

namespace gzn::fnd {

template<util::allocator_type Base, usize MaxBytesCount = 2048>
class freetree_allocator : private Base {
  /** @todo */
};

} // namespace gzn::fnd
