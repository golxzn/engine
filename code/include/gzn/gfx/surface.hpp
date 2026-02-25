#pragma once

#include <glm/vec2.hpp>

#include "gzn/fnd/func.hpp"
#include "gzn/fnd/util/unsafe_any_ref.hpp"

namespace gzn::gfx {


using surface_handle = void *;

struct surface_proxy {
  template<class T>
  using func = fnd::move_only_func<T>;

  func<auto(fnd::util::unsafe_any_ref)->bool> setup{};
  func<void(fnd::util::unsafe_any_ref)>       destroy{};
  func<void(fnd::util::unsafe_any_ref)>       present{};
  func<auto()->glm::u32vec2>                  get_size{};
  func<auto()->surface_handle>                get_handle{};

  [[nodiscard]]
  constexpr auto valid() const noexcept {
    return setup && destroy && present && get_size;
  }
};

using surface_builder_func = fnd::move_only_func<auto()->gfx::surface_proxy>;

} // namespace gzn::gfx
