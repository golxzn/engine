#include "gzn/app/view.hpp"

namespace gzn::app {

auto view::make([[maybe_unused]] view_info const &info)
  -> expected<view, make_error> {
  return unexpected{ make_error::invalid_memory_resource };
}

} // namespace gzn::app
