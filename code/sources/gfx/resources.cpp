#include "gzn/gfx/resources.hpp"

#include "gzn/gfx/res/framebuffer.hpp"
#include "gzn/gfx/res/pipeline.hpp"
#include "gzn/gfx/res/render-pass.hpp"
#include "gzn/gfx/res/texture.hpp"

namespace gzn::gfx {

namespace {

inline constexpr cstr RES_MODULE{ "gfx::ctx::res" };

} // namespace

auto res::make(context &ctx, res_buffer const &info) -> fnd::handle<buffer> {
  return {};
}

auto res::make(context &ctx, res_vertex_buffer const &info)
  -> fnd::handle<buffer> {
  return {};
}

void res::free(context &ctx, fnd::handle<buffer> &info) {}

auto res::make(context &ctx, res_shader const &info) -> fnd::handle<shader> {
  return {};
}

void res::free(context &ctx, fnd::handle<shader> &info) {}

auto res::make(context &ctx, res_pipeline const &info)
  -> fnd::handle<pipeline> {
  return {};
}

void res::free(context &ctx, fnd::handle<pipeline> &info) {}

auto res::make(context &ctx, res_sampler const &info) -> fnd::handle<sampler> {
  return {};
}

void res::free(context &ctx, fnd::handle<sampler> &info) {}

auto res::make(context &ctx, res_texture const &info) -> fnd::handle<texture> {
  return {};
}

void res::free(context &ctx, fnd::handle<texture> &info) {}

auto res::make(context &ctx, res_texture_view const &info)
  -> fnd::handle<texture_view> {
  return {};
}

void res::free(context &ctx, fnd::handle<texture_view> &info) {}

auto res::make(context &ctx, res_render_pass const &info)
  -> fnd::handle<render_pass> {
  return {};
}

void res::free(context &ctx, fnd::handle<render_pass> &info) {}

auto res::make(context &ctx, res_framebuffer const &info)
  -> fnd::handle<framebuffer> {
  return {};
}

void res::free(context &ctx, fnd::handle<framebuffer> &info) {}

auto res::make(context &ctx, res_binding_group const &info)
  -> fnd::handle<binding_group> {
  return {};
}

void res::free(context &ctx, fnd::handle<binding_group> &info) {}


} // namespace gzn::gfx
