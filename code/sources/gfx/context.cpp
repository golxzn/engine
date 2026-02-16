#include "gzn/gfx/context.hpp"

#include "gzn/gfx/backends/ctx/metal.hpp"
#include "gzn/gfx/backends/ctx/opengl.hpp"
#include "gzn/gfx/backends/ctx/opengl_es2.hpp"
#include "gzn/gfx/backends/ctx/vulkan.hpp"

namespace gzn::gfx {

namespace {

auto select_available(backend_type const preferred)
  -> std::optional<backend_type> {
#if defined(GZN_GFX_BACKEND_ANY)

  auto static constexpr is_available{ [](auto const type) {
    using namespace backends;
    switch (type) {
      case backend_type::metal     : return ctx::metal::is_available();
      case backend_type::vulkan    : return ctx::vulkan::is_available();
      case backend_type::opengl    : return ctx::opengl::is_available();
      case backend_type::opengl_es2: return ctx::opengl_es2::is_available();
      default                      : break;
    }
    return false;
  } };

  if (preferred != backend_type::any && is_available(preferred)) {
    return preferred;
  }

  for (auto const type : backend_types) {
    if (is_available(type)) { return type; }
  }
  return std::nullopt;

#else

  return ctx::GZN_GFX_BACKEND::is_available() ? preferred : std::nullopt;

#endif // defined(GZN_GFX_BACKEND_ANY)
}

auto build_context(std::span<byte> storage, context_info const &info)
  -> context_data_view {
#if defined(GZN_GFX_BACKEND_ANY)
  using namespace backends;
  switch (info.backend) {
    using enum backend_type;
    case metal:
      return context_data_view{
        ctx::metal::make_context_on(storage, info.capacities)
      };
    case vulkan:
      return context_data_view{
        ctx::vulkan::make_context_on(storage, info.capacities)
      };
    case opengl:
      return context_data_view{
        ctx::opengl::make_context_on(storage, info.capacities)
      };
    case opengl_es2:
      return context_data_view{
        ctx::opengl_es2::make_context_on(storage, info.capacities)
      };
    default: break;
  }
  return {};
#else

  return context_data_view{
    ctx::GZN_GFX_BACKEND::make_context_on(storage, info.capacities)
  };

#endif // defined(GZN_GFX_BACKEND_ANY)
}


} // namespace

context::context(members mem) noexcept
  : m{ std::move(mem) } {}

context::~context() {
  if (m.surface.valid()) { m.surface.destroy(data()); }
}

context::context(context &&other) noexcept
  : m{ std::move(other.m) } {}

auto context::operator=(context &&other) noexcept -> context & {
  if (this != &other) { m = std::move(other.m); }
  return *this;
}

auto context::calculate_required_space_for(
  [[maybe_unused]] backend_type const type,
  render_capacities const            &caps
) noexcept -> usize {
  using namespace backends;

#if defined(GZN_GFX_BACKEND_ANY)
  switch (type) {
    using enum backend_type;

    case metal     : return ctx::metal::calc_required_space_for(caps);
    case vulkan    : return ctx::vulkan::calc_required_space_for(caps);
    case opengl    : return ctx::opengl::calc_required_space_for(caps);
    case opengl_es2: return ctx::opengl_es2::calc_required_space_for(caps);

    default        : break;
  }
  return {};
#else
  return ctx::GZN_GFX_BACKEND::calc_required_space_for(caps);
#endif // defined(GZN_GFX_BACKEND_ANY)
}

void context::present() { m.surface.present(data()); }

auto context::construct(std::span<byte> storage, context_info info)
  -> members {
  auto constexpr none{ gfx::backend_type::any };
  info.backend = select_available(info.backend).value_or(none);
  if (info.backend == none) { return {}; }

  auto data_view{ build_context(storage, info) };
  if (data_view == nullptr) { return {}; }

  auto surface{ info.surface_builder() };

  if (!surface.valid() || !surface.setup(data_view)) {}

  return members{
    .surface{ std::move(surface) },
    .backend   = info.backend,
    .data_view = data_view,
  };
}

} // namespace gzn::gfx
