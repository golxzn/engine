#pragma once

#include <cstddef>
#include <cstdint>

namespace gzn {

using s8    = int8_t;
using u8    = uint8_t;

using s16   = int16_t;
using u16   = uint16_t;

using s32   = int32_t;
using u32   = uint32_t;

using s64   = int64_t;
using u64   = uint64_t;

using f32   = float;
using f64   = double;

using cstr  = char const *;
using byte  = std::byte;

using usize = u64;
using ssize = s64;

template<class T, usize Length>
using c_array = T[Length];

} // namespace gzn
