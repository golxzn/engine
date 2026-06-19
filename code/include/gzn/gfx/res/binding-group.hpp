#pragma once

#include "gzn/fnd/containers/span.hpp"
#include "gzn/fnd/name.hpp"
#include "gzn/gfx/enums.hpp"

namespace gzn::gfx {

struct layout_binding_info {
  usize           binding;
  usize           count;
  descriptor_type type;
  shader_type     stages;
};

class binding_group {
  fnd::s8name_view               name;
  fnd::span<layout_binding_info> bindings;
};

} // namespace gzn::gfx
