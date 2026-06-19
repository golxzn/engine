#pragma once

// IWYU pragma: begin_keep
// clang-format off

#include "gzn/fnd/allocators/dummy-allocator.hpp"
#include "gzn/fnd/allocators/heap-allocator.hpp"
#include "gzn/fnd/allocators/mmap-allocator.hpp" /// @todo
#include "gzn/fnd/allocators/in-stack-allocator.hpp" // in-stack with stack logic is weird!

#include "gzn/fnd/allocators/fallback-allocator.hpp"
// #include "gzn/fnd/allocators/cascade-allocator.hpp" /// @todo
// #include "gzn/fnd/allocators/scoped-allocator.hpp" /// @todo

#include "gzn/fnd/allocators/affix-allocator.hpp" /// @todo finish expand method
#include "gzn/fnd/allocators/thread-safe-allocator.hpp"
#include "gzn/fnd/allocators/segregate-allocator.hpp"

#include "gzn/fnd/allocators/freelist-allocator.hpp"
#include "gzn/fnd/allocators/freetree-allocator.hpp" /// @todo implement
#include "gzn/fnd/allocators/pool-allocator.hpp"
#include "gzn/fnd/allocators/bucket-allocator.hpp"

// clang-format on
// IWYU pragma: end_keep
