#include "gzn/gfx/context.hpp"

#include "gzn/gfx/backends/ctx/metal.hpp"
#include "gzn/gfx/backends/ctx/opengl.hpp"
#include "gzn/gfx/backends/ctx/vulkan.hpp"
#include "gzn/gfx/gpu-info.hpp"

namespace gzn::gfx {

namespace {

auto default_select_gpu(std::span<gpu_info const> const devices) -> usize {
  if (std::empty(devices)) { return 0; }

  struct selection {
    usize idx{};
    usize score{ std::numeric_limits<usize>::min() };
  };

  selection selected{};

  for (usize idx{}; idx < std::size(devices); ++idx) {
    auto const     &device{ devices[idx] };
    selection const current{
      .idx   = idx,
      .score = device.vram_bytes + static_cast<usize>(device.type) * 1000zu,
    };
    if (current.score > selected.score) { selected = current; }
  }
  return selected.idx;
}

auto select_available(backend_type const preferred)
  -> std::optional<backend_type> {
#if defined(GZN_GFX_BACKEND_ANY)

  if (preferred != backend_type::any && context::is_available(preferred)) {
    return preferred;
  }

  for (auto const type : backend_types) {
    if (context::is_available(type)) { return type; }
  }
  return std::nullopt;

#else

  return ctx::GZN_GFX_BACKEND::is_available() ? preferred : std::nullopt;

#endif // defined(GZN_GFX_BACKEND_ANY)
}

auto build_context(
  context_info const       &info,
  fnd::util::unsafe_any_ref api_specific
) -> fnd::util::unsafe_any_ref {
  using namespace backends;

#if defined(GZN_GFX_BACKEND_ANY)
  switch (info.backend) {
    using enum backend_type;
    case metal:
      return fnd::util::unsafe_any_ref{
        ctx::metal::make_context_on(info, api_specific)
      };
    case vulkan:
      return fnd::util::unsafe_any_ref{
        ctx::vulkan::make_context_on(info, api_specific)
      };
    case opengl:
      return fnd::util::unsafe_any_ref{
        ctx::opengl::make_context_on(info, api_specific)
      };

    default: break;
  }
  return {};
#else

  return fnd::util::unsafe_any_ref{
    ctx::GZN_GFX_BACKEND::make_context_on(info, api_specific)
  };

#endif // defined(GZN_GFX_BACKEND_ANY)
}

auto setup_context(
  std::span<std::byte> storage,
  context_info const  &info,
  surface_proxy       &surface
) -> bool {
  using namespace backends;

#if defined(GZN_GFX_BACKEND_ANY)
  switch (info.backend) {
    using enum backend_type;
    case metal : return ctx::metal::setup(storage, info, surface);
    case vulkan: return ctx::vulkan::setup(storage, info, surface);
    case opengl: return ctx::opengl::setup(storage, info, surface);

    default    : break;
  }
  return {};
#else

  return ctx::GZN_GFX_BACKEND::setup(storage, info, surface);

#endif // defined(GZN_GFX_BACKEND_ANY)
}

auto destroy_context(backend_type const type) {
  using namespace backends;

#if defined(GZN_GFX_BACKEND_ANY)
  switch (type) {
    using enum backend_type;
    case metal : ctx::metal::destroy(); break;
    case vulkan: ctx::vulkan::destroy(); break;
    case opengl: ctx::opengl::destroy(); break;

    default    : break;
  }
#else
  ctx::GZN_GFX_BACKEND::destroy();
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

    case metal : return ctx::metal::calc_required_space_for(caps);
    case vulkan: return ctx::vulkan::calc_required_space_for(caps);
    case opengl: return ctx::opengl::calc_required_space_for(caps);

    default    : break;
  }
  return {};
#else
  return ctx::GZN_GFX_BACKEND::calc_required_space_for(caps);
#endif // defined(GZN_GFX_BACKEND_ANY)
}

auto context::is_available(backend_type type) -> bool {
  using namespace backends;
  switch (type) {
#if defined(GZN_GFX_BACKEND_METAL)
    case backend_type::metal: return ctx::metal::is_available();
#endif // defined(GZN_GFX_BACKEND_METAL)
#if defined(GZN_GFX_BACKEND_VULKAN)
    case backend_type::vulkan: return ctx::vulkan::is_available();
#endif // defined(GZN_GFX_BACKEND_VULKAN)
#if defined(GZN_GFX_BACKEND_OPENGL)
    case backend_type::opengl: return ctx::opengl::is_available();
#endif // defined(GZN_GFX_BACKEND_OPENGL)
    default: break;
  }
  return false;
}

void context::present() { m.surface.present(data()); }

auto context::construct(
  std::span<byte>           storage,
  context_info              info,
  fnd::util::unsafe_any_ref api_specific
) -> members {
  auto constexpr none{ gfx::backend_type::any };
  info.backend = select_available(info.backend).value_or(none);
  if (info.backend == none) { return {}; }

  if (!info.select_gpu) {
    fnd::dummy_allocator dummy{};
    info.select_gpu = context_info::select_gpu_func{
      dummy,
      &default_select_gpu,
    };
  }

  auto data_view{ build_context(info, api_specific) };
  if (data_view == nullptr) { return {}; }

  auto surface{ info.surface_builder() };

  if (!surface.valid() || !surface.setup(data_view)) {
    destroy_context(info.backend);
    /// @todo maybe some log
    return {};
  }

  if (!setup_context(storage, info, surface)) {
    destroy_context(info.backend);
    /// @todo maybe some log
    return {};
  }

  return members{
    .surface{ std::move(surface) },
    .backend  = info.backend,
    .data_ref = data_view,
  };
}

} // namespace gzn::gfx
