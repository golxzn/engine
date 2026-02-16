#pragma once

#include <span>
#include <typeindex>

#include <glm/vec2.hpp>

#include "gzn/fnd/definitions.hpp"
#include "gzn/fnd/owner.hpp"
#include "gzn/gfx/backend-type.hpp"
#include "gzn/gfx/render-capacities.hpp"
#include "gzn/gfx/surface.hpp"

namespace gzn::gfx {

struct context_info {
  backend_type         backend{ backend_type::any };
  surface_builder_func surface_builder{};
  render_capacities    capacities{};
};

using storage_key   = u32;
using native_handle = void *;

class context_data_view {
public:
  constexpr context_data_view() = default;

  constexpr explicit context_data_view(auto &data) noexcept
    : ptr{ static_cast<void *>(&data) } {
    gzn_if_debug(type = typeid(std::remove_pointer_t<decltype(data)>));
  }

  constexpr explicit context_data_view(auto *data) noexcept
    : ptr{ static_cast<void *>(data) } {
    gzn_assertion(data != nullptr, "Invalid data!");
    gzn_if_debug(type = typeid(std::remove_pointer_t<decltype(data)>));
  }

  constexpr context_data_view(context_data_view const &other)     = default;
  constexpr context_data_view(context_data_view &&other) noexcept = default;
  constexpr auto operator=(context_data_view const &other)
    -> context_data_view & = default;
  constexpr auto operator=(context_data_view &&other) noexcept
    -> context_data_view & = default;

  template<class T>
  [[nodiscard]]
  gzn_inline constexpr auto as(this auto &&self) noexcept {
    gzn_assertion(self.ptr != nullptr, "Invalid data!");
    gzn_if_debug(gzn_assertion(self.type == typeid(T), "Invalid data type!"));
    return static_cast<T *>(self.ptr);
  }

  [[nodiscard]]
  constexpr auto operator==(std::nullptr_t) const noexcept -> bool {
    return ptr == nullptr;
  }

  [[nodiscard]]
  constexpr auto operator!=(std::nullptr_t) const noexcept -> bool {
    return ptr != nullptr;
  }

private:
  void *ptr{ nullptr };
  gzn_if_debug(std::type_index type{ typeid(void) });
};

class context {
  friend struct cmd;
  friend fnd::stack_owner<context>;
  friend fnd::heap_owner<context>;

  struct members {
    surface_proxy     surface{};
    backend_type      backend{};
    context_data_view data_view{};
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
  static auto make(fnd::util::allocator_type auto &alloc, context_info info)
    -> context {
    auto const required_space{
      calculate_required_space_for(info.backend, info.capacities)
    };
    if (auto place{ alloc.allocate(required_space, 0u) }; place) {
      std::span<byte> storage{ static_cast<byte *>(place), required_space };
      return context{ construct(storage, std::move(info)) };
    }
    return context{ {} };
  }

  [[nodiscard]] auto is_valid() const noexcept -> bool {
    return m.data_view != nullptr;
  }

  [[nodiscard]] auto data() const noexcept -> context_data_view {
    return m.data_view;
  }

  [[nodiscard]] auto get_surface_proxy(this auto &&self) {
    return &self.m.surface;
  }

private:
  void present();

  static auto construct(std::span<byte> storage, context_info info) -> members;
};

} // namespace gzn::gfx
