#pragma once

#include "gzn/fnd/types.hpp"

namespace gzn::fnd {

union alignas(u32) version {
  u32 value{};

  struct {
    u32 major : 8;
    u32 minor : 12;
    u32 patch : 12;
  } split;

  [[nodiscard]]
  static constexpr auto make(u32 major, u32 minor, u32 patch) noexcept {
    return version{
      .split{ .major = major, .minor = minor, .patch = patch }
    };
  }
};

} // namespace gzn::fnd
