#pragma once

#include <glm/vec2.hpp>

#include "gzn/fnd/owner.hpp"

namespace gzn::gfx {

class context;
enum class backend_type;

struct surface_proxy {
  template<class T>
  using func = fnd::move_only_func<T>;

  func<auto(context &)->bool> setup{};
  func<void(context &)>       destroy{};
  func<void(context &)>       present{};

  func<auto()->glm::u32vec2> get_size{};

  [[nodiscard]]
  constexpr auto valid() const noexcept {
    return setup && destroy && present && get_size;
  }
};

using surface_creation_func =
  fnd::move_only_func<auto(gfx::backend_type)->gfx::surface_proxy>;

} // namespace gzn::gfx
