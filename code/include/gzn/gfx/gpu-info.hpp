#pragma once

#include <string_view>

#include "gzn/fnd/types.hpp"

namespace gzn::gfx {

enum class gpu_type {
  unknown,
  emulated_gpu,
  integrated_gpu,
  virtual_gpu,
  discrete_gpu,
};

struct gpu_info {
  std::string_view name{};
  std::string_view vendor{};
  std::string_view driver{};

  usize    vram_bytes{};
  gpu_type type{ gpu_type::unknown };
};

}; // namespace gzn::gfx
