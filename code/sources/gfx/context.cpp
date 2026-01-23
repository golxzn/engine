#include "gzn/gfx/context.hpp"

namespace gzn::gfx {

context::context(context_info info)
  : m{ .surface{ info.make_surface(info.backend) }, .backend = info.backend } {
  if (m.surface.valid()) { m.surface.setup(*this); }
}

context::~context() {
  if (m.surface.valid()) { m.surface.destroy(*this); }
}

context::context(context &&other) noexcept
  : m{ std::move(other.m) } {}

auto context::operator=(context &&other) noexcept -> context & {
  if (this != &other) { m = std::move(other.m); }
  return *this;
}

} // namespace gzn::gfx
