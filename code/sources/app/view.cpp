#include "gzn/app/view.hpp"

#if defined(GZN_VIEW_BACKEND_X11)
#  include "./backends/view/x11.inl"
#elif defined(GZN_VIEW_BACKEND_WAYLAND)
#  include "./backends/view/wayland.inl"
#else
#  error "No suitable backend for gzn::app::view was choosen"
#endif

namespace gzn::app {

view::view(view_info const &info)
  : id{ backends::view::construct(info) } {}

view::~view() {
  backends::view::destroy(std::exchange(id, invalid_backend_id));
}

view::view(view &&other) noexcept
  : id{ std::exchange(other.id, invalid_backend_id) } {}

auto view::operator=(view &&other) noexcept -> view & {
  if (this != &other) { id = std::exchange(other.id, invalid_backend_id); }
  return *this;
}

auto view::take_next_event(event &ev) -> bool {
  return backends::view::take_event(id, ev);
}

auto view::get_size() const noexcept -> glm::u32vec2 {
  return backends::view::get_size(id);
}

auto view::get_native_handle() const noexcept -> native_view_handle {
  return backends::view::get_native_handle(id);
}

auto view::get_required_extensions() -> std::span<cstr const> {
  return backends::view::get_required_extensions();
}

auto view::make_surface_proxy(backend_id id, gfx::backend_type type)
  -> gfx::surface_proxy {
  return backends::view::make_surface_proxy(id, type);
}

} // namespace gzn::app
