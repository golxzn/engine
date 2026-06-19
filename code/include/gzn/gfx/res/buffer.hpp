#pragma once

#include "gzn/fnd/containers/span.hpp"
#include "gzn/gfx/enums.hpp"

namespace gzn::gfx {

inline constexpr auto LOCATION_AUTO{ (std::numeric_limits<usize>::max)() };

struct binding_info {
  usize      stride;
  usage_rate rate{ usage_rate::per_vertex };
};

struct attribute_description {
  usize       location{ LOCATION_AUTO };
  usize       binding;
  usize       offset;
  format_type format{ format_type::undefined };
};

struct attribute_layout {
  fnd::span<binding_info const>          bindings{};
  fnd::span<attribute_description const> description{};
};

class buffer {
public:
  [[nodiscard]] constexpr auto layout() const noexcept -> attribute_layout {
    return {};
  }

  [[nodiscard]] constexpr auto size() const noexcept -> usize { return 0; }

  [[nodiscard]] constexpr auto data() noexcept -> byte * { return nullptr; }

  [[nodiscard]] constexpr auto data() const noexcept -> byte const * {
    return nullptr;
  }

  [[nodiscard]] constexpr auto span() noexcept -> fnd::span<byte> {
    return {};
  }

  [[nodiscard]] constexpr auto span() const noexcept -> fnd::span<byte const> {
    return {};
  }
};

} // namespace gzn::gfx
