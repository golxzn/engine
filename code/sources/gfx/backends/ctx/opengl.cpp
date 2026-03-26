#include "gzn/gfx/backends/ctx/opengl.hpp"

#include <glad/egl.h>
#include <glad/gl.h>

namespace gzn::gfx::backends::ctx {

namespace {

constexpr cstr THIS_MODULE{ "gfx::ctx::opengl" };

int glad_egl_version{};
int glad_gl_version{};

} // namespace

void opengl::load() {
  glad_egl_version = gladLoaderLoadEGL(EGL_NO_DISPLAY);
  glad_gl_version  = gladLoaderLoadGL();
}

void opengl::unload() {
  gladLoaderUnloadGL();
  gladLoaderUnloadEGL();
  glad_egl_version = 0;
  glad_gl_version  = 0;
}

bool opengl::is_available() noexcept { return false; }

auto opengl::calc_required_space_for(render_capacities const &caps) noexcept
  -> usize {
  return 0;
}

auto opengl::make_context_on(
  context_info const       &info,
  fnd::util::unsafe_any_ref extra
) -> opengl * {
  return nullptr;
}

auto opengl::setup(
  std::span<byte>     storage,
  context_info const &info,
  surface_proxy      &surface
) -> bool {
  return false;
}

void opengl::destroy() {}


} // namespace gzn::gfx::backends::ctx
