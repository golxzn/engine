#include "gzn/gfx/commands.hpp"

#include "./backends/cmd/metal.inl"
#include "./backends/cmd/opengl.inl"
#include "./backends/cmd/vulkan.inl"
#include "gzn/gfx/context.hpp"

namespace gzn::gfx {

namespace {

struct cache {
  void (*clear)(fnd::util::unsafe_any_ref, cmd_clear const &){ nullptr };
  void (*submit)(fnd::util::unsafe_any_ref){ nullptr };
};

template<class backend>
constexpr auto make_cache_for() noexcept {
  return cache{
    .clear  = &backend::clear,
    .submit = &backend::submit,
  };
}

#if defined(GZN_GFX_BACKEND_METAL)
cache inline constexpr metal{ make_cache_for<backends::metal>() };
#endif // defined(GZN_GFX_BACKEND_METAL)

#if defined(GZN_GFX_BACKEND_VULKAN)
cache inline constexpr vulkan{ make_cache_for<backends::vulkan>() };
#endif // defined(GZN_GFX_BACKEND_VULKAN)

#if defined(GZN_GFX_BACKEND_OPENGL)
cache inline constexpr opengl{ make_cache_for<backends::opengl>() };
#endif // defined(GZN_GFX_BACKEND_OPENGL)

cache const *_current_backend{
#if !defined(GZN_GFX_BACKEND_ANY)
  &GZN_GFX_BACKEND
#endif // !defined(GZN_GFX_BACKEND_ANY)
};

} // namespace

void cmd::setup_for(context &ctx) {
#if defined(GZN_GFX_BACKEND_ANY)
  switch (ctx.m.backend) {
    case backend_type::metal     : _current_backend = &metal; break;
    case backend_type::vulkan    : _current_backend = &vulkan; break;
    case backend_type::opengl    : _current_backend = &opengl; break;

    default                      : std::unreachable();
  }
#endif // defined(GZN_GFX_BACKEND_ANY)
}

void cmd::start(context &ctx) {}

void cmd::clear(context &ctx, cmd_clear const &clr) {
  _current_backend->clear(ctx.data(), clr);
}

void cmd::submit(context &ctx) { _current_backend->submit(ctx.data()); }

void cmd::present(context &ctx) { ctx.present(); }

} // namespace gzn::gfx
