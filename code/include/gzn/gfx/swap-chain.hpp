#pragma once

#include <glm/vec2.hpp>

#include "gzn/fnd/types.hpp"
#include "gzn/gfx/enums.hpp"

namespace gzn::gfx {

enum class present_mode : uint8_t {
  immediate,
  mailbox,
  fifo,
};

inline constexpr glm::u32vec2 default_resolution{ 1024, 1024 };

struct swapchain_info {
  glm::u32vec2 resolution{ default_resolution };
  present_mode present{ present_mode::mailbox };
  u8           image_count{ 3 };
  format_type  image_format{ format_type::rgb_u8 };
  bool         clipped{ true };
};

} // namespace gzn::gfx
