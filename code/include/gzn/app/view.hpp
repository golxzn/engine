#pragma once

#include <limits>
#include <span>
#include <string_view>

#include <glm/vec2.hpp>

#include "gzn/app/event.hpp"
#include "gzn/fnd/allocators.hpp"
#include "gzn/gfx/backend-type.hpp"
#include "gzn/gfx/surface.hpp"

namespace gzn::app {

/*
 * Pass view to the render is shit. The rendering layer doesn't care and know
 * about the application layer. BUT,
 * 1. Vulkan requires VkSurface which could be made only here;
 * 2. OpenGL requires context which is made by window again.
 *
 * I see only one option: view should take or construct context;
 * So, it should be possible to create render context without view when user
 * want to use glfw or something. But then we cannot provide "don't care 'bout
 * backstage" and should ask user to create the surface/glcontext and pass it
 * to out render.
 */

/*
 * The main idea of view is not to be a fully featured window class, but a thin
 * layer which provides just minimal enough functionality for rendering
 * purposes:
 * 1. Get width/height
 * 2. Be notified on screen change
 * 3. Access the surface to draw things
 */

struct view_flags {
  bool resizable   : 1 {};
  bool full_screen : 1 {};
};

struct view_info {
  std::string_view title{};
  std::string_view display_name{};
  glm::u32vec2     size{};
  view_flags       flags{};
};

using native_view_handle = void *;
using backend_id         = u32;
inline constexpr auto invalid_backend_id{
  std::numeric_limits<backend_id>::max()
};

class view {
  explicit view(view_info const &info);

public:
  ~view();

  view(view const &) = delete;
  view(view &&other) noexcept;

  auto operator=(view const &) -> view = delete;
  auto operator=(view &&other) noexcept -> view &;

  template<template<class> class stack_type>
  [[nodiscard]]
  static auto make(
    fnd::util::allocator_type auto &alloc,
    view_info const                &info
  ) -> stack_type<view> {
    return stack_type<view>{ alloc, view{ info } };
  }

  [[nodiscard]]
  auto take_next_event(event &ev) -> bool;

  [[nodiscard]]
  auto make_surface_proxy(gfx::backend_type const backend) const
    -> gfx::surface_proxy {
    return view::make_surface_proxy(id, backend);
  }

  [[nodiscard]]
  auto get_surface_builder(
    fnd::util::allocator_type auto &alloc,
    gfx::backend_type               backend
  ) const {
    return gfx::surface_builder_func{
      alloc,
      [i{ id }, backend] -> gfx::surface_proxy {
        return view::make_surface_proxy(i, backend);
      }
    };
  }

  [[nodiscard]]
  auto get_size() const noexcept -> glm::u32vec2;

  [[nodiscard]]
  auto get_native_handle() const noexcept -> native_view_handle;

  [[nodiscard]]
  static auto get_required_extensions() -> std::span<cstr const>;

private:
  backend_id id{ invalid_backend_id };

  static auto make_surface_proxy(backend_id id, gfx::backend_type type)
    -> gfx::surface_proxy;
};

} // namespace gzn::app
