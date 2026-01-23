#pragma once

namespace gzn::gfx {

enum class backend_type {
  any,
#if defined(GZN_PLATFORM_IOS) || defined(GZN_PLATFORM_MACOS)
  metal,
#endif // defined(GZN_PLATFORM_IOS) || defined(GZN_PLATFORM_MACOS)
  vulkan,
  opengl,
  opengl_es,
};

} // namespace gzn::gfx
