#pragma once

#include "gzn/fnd/types.hpp"

namespace gzn::gfx {

/// @todo Determine real counts
inline constexpr usize PIPELINES_COUNT{ 128 };
inline constexpr usize PIPELINES_MIN_COUNT{ 32 };

inline constexpr usize BUFFERS_COUNT{ 512 };
inline constexpr usize BUFFERS_MIN_COUNT{ 64 };

inline constexpr usize SAMPLERS_COUNT{ 128 };
inline constexpr usize SAMPLERS_MIN_COUNT{ 32 };

} // namespace gzn::gfx
