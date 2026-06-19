#pragma once

#include <variant>

#include <glm/vec2.hpp>

#include "gzn/fnd/func.hpp"
#include "gzn/fnd/pointers.hpp"

namespace gzn::gfx {

namespace api {

using none = std::monostate;

struct xcb {
  fnd::not_null<void> connection;
  u32                 window;
};

struct winapi {
  fnd::not_null<void> hinstance;
  fnd::not_null<void> hwnd;
};

struct wayland {
  fnd::not_null<void> wl_display;
  fnd::not_null<void> wl_surface;
};

struct android {
  fnd::not_null<void> handle; // ANativeWindow*
};

struct metal_layer {
  fnd::not_null<void> handle; // CAMetalLayer* (iOS/macOS)
};

struct web_canvas {
  fnd::not_null<void> handle; // JS HTMLCanvasElement* (for WebGPU)
};

} // namespace api

// clang-format off
using surface_handle = std::variant<
  api::none,
  api::xcb,
  api::winapi,
  api::wayland,
  api::android,
  api::metal_layer,
  api::web_canvas
>;
// clang-format on

struct surface_proxy {
  template<class T>
  using func = fnd::move_only_func<T>;

  func<auto()->surface_handle> get_handle{};
  func<auto()->glm::u32vec2>   get_size{};

  [[nodiscard]]
  constexpr auto valid() const noexcept -> bool {
    return get_handle && get_size;
  }
};

using surface_builder_func = fnd::move_only_func<auto()->gfx::surface_proxy>;

} // namespace gzn::gfx
