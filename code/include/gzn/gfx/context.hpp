#pragma once

#include <string_view>

#include <glm/vec2.hpp>

#include "gzn/gfx/backend_type.hpp"
#include "gzn/gfx/surface.hpp"

namespace gzn::gfx {

struct context_info {
  backend_type          backend{ backend_type::any };
  surface_creation_func make_surface{};
};

class context {
  friend struct cmd;

public:
  explicit context(context_info info);
  ~context();

  context(context const &other)                         = delete;
  context(context &&other) noexcept;

  auto operator=(context const &other) -> context &     = delete;
  auto operator=(context &&other) noexcept -> context &;

  template<template<class> class stack_type>
  [[nodiscard]]
  static auto make(fnd::util::allocator_type auto &alloc, context_info info)
    -> stack_type<context> {
    return stack_type<context>{ alloc, std::move(info) };
  }

private:
  struct members {
    surface_proxy surface{};
    backend_type  backend{};
  } m;
};

} // namespace gzn::gfx
