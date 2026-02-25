#pragma once

#include <span>

#include <glm/vec2.hpp>

#include "gzn/fnd/owner.hpp"
#include "gzn/fnd/util/unsafe_any_ref.hpp"
#include "gzn/fnd/version.hpp"
#include "gzn/gfx/backend-type.hpp"
#include "gzn/gfx/render-capacities.hpp"
#include "gzn/gfx/surface.hpp"

namespace gzn::gfx {

struct gpu_info;

struct context_info {
  using select_gpu_func =
    fnd::move_only_func<auto(std::span<gpu_info const>)->usize>;

  backend_type backend{ backend_type::any };

  std::string_view app_name{};
  fnd::version     app_version{ fnd::version::make(0, 1, 0) };
  std::string_view engine_name{ app_name };
  fnd::version     engine_version{ fnd::version::make(0, 1, 0) };

  surface_builder_func    surface_builder{};
  std::span<cstr const>   extensions{};
  mutable select_gpu_func select_gpu{};
  render_capacities       capacities{};
};

class context {
  friend struct cmd;
  friend fnd::stack_owner<context>;
  friend fnd::heap_owner<context>;

  struct members {
    surface_proxy             surface{};
    backend_type              backend{};
    fnd::util::unsafe_any_ref data_ref{};
  } m;

  explicit context(members info) noexcept;

public:
  ~context();

  context(context const &other) = delete;
  context(context &&other) noexcept;

  auto operator=(context const &other) -> context & = delete;
  auto operator=(context &&other) noexcept -> context &;

  [[nodiscard]]
  static auto calculate_required_space_for(
    backend_type const       type,
    render_capacities const &caps
  ) noexcept -> usize;

  [[nodiscard]]
  static auto is_available(backend_type type) -> bool;

  [[nodiscard]]
  static auto make(
    fnd::util::allocator_type auto &alloc,
    context_info                    info,
    fnd::util::unsafe_any_ref       api_specific = {}
  ) -> context {
    auto const required_space{
      calculate_required_space_for(info.backend, info.capacities)
    };
    if (auto place{ alloc.allocate(required_space, 0u) }; place) {
      std::span<byte> storage{ static_cast<byte *>(place), required_space };
      return context{ construct(storage, std::move(info), api_specific) };
    }
    return context{ {} };
  }

  [[nodiscard]] auto is_valid() const noexcept -> bool {
    return m.data_ref != nullptr;
  }

  [[nodiscard]] auto data() const noexcept -> fnd::util::unsafe_any_ref {
    return m.data_ref;
  }

  [[nodiscard]] auto get_surface_proxy(this auto &&self) {
    return &self.m.surface;
  }

private:
  void present();

  static auto construct(
    std::span<byte>           storage,
    context_info              info,
    fnd::util::unsafe_any_ref api_specific
  ) -> members;
};

} // namespace gzn::gfx
