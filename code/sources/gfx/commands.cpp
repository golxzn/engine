#include "gzn/gfx/commands.hpp"

#include "./backends/cmd/metal.inl"
#include "./backends/cmd/opengl.inl"
#include "./backends/cmd/vulkan.inl"
#include "gzn/fnd/util/string-literal.hpp"
#include "gzn/gfx/context.hpp"

namespace gzn::gfx {

namespace {

inline constexpr cstr CMD_MODULE{ "gfx::gfx::cmd" };

template<fnd::string_literal Label, class... Args>
void mock_cmd(context &ctx, Args const &...) {
  ctx.get_logger().fatal(
    CMD_MODULE,
    R"(Command "%.*s" was called before "setup_for")",
    static_cast<int>(std::size(Label)),
    std::data(Label)
  );
}

struct cache {
  void (*begin_frame)(context &){ &mock_cmd<"begin_frame"> };
  void (*end_frame)(context &){ &mock_cmd<"end_frame"> };

  void (*clear_color)(context &ctx, cmd_clear_color const &info){
    &mock_cmd<"clear_color", cmd_clear_color>
  };
  void (*clear_depth_stencil)(context &ctx, cmd_depth_stencil const &info){
    &mock_cmd<"clear_depth_stencil", cmd_depth_stencil>
  };
  void (*clear_values)(context &ctx, std::span<clear_value> const &info){
    &mock_cmd<"clear_values", std::span<clear_value>>
  };
};

template<class backend>
constexpr auto make_cache_for() noexcept {
  return cache{
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

void cmd::init(context &ctx, cmd_init const &) {
#if defined(GZN_GFX_BACKEND_ANY)
  switch (ctx.m.backend) {
    case backend_type::metal : _current_backend = &metal; break;
    case backend_type::vulkan: _current_backend = &vulkan; break;
    case backend_type::opengl: _current_backend = &opengl; break;

    default                  : std::unreachable();
  }
#endif // defined(GZN_GFX_BACKEND_ANY)
}

void cmd::begin_frame(context &ctx) { _current_backend->begin_frame(ctx); }

void cmd::end_frame(context &ctx) { _current_backend->end_frame(ctx); }


} // namespace gzn::gfx
