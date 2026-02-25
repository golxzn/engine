#pragma once

#include <array>

namespace gzn::gfx {

#if defined(GZN_GFX_BACKEND_ANY)

enum class backend_type {
  any,
  metal,
  vulkan,
  opengl,
};

inline constexpr std::array backend_types{
  backend_type::metal,
  backend_type::vulkan,
  backend_type::opengl,
};

#else

enum class backend_type { GZN_GFX_BACKEND };

#endif // defined(GZN_GFX_BACKEND_ANY)

} // namespace gzn::gfx
