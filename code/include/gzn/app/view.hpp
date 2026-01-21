#pragma once

#include <string_view>

#include "gzn/app/dimensions.hpp"
#include "gzn/fnd/expected.hpp"

namespace gzn::app {

/*
 * The main idea of view is not to be a fully featured window class, but a thin
 * layer which provides just minimal enough functionality for rendering
 * purposes:
 * 1. Get width/height
 * 2. Be notified on screen change
 * 3. Access the surface to draw things
 */

struct view_flags {
  bool resizable   : 1 {};
  bool full_screen : 1 {};
};

struct view_info {
  std::string_view title{};
  u32dim           size{};
  view_flags       flags{};
};

class view {
public:
  enum class make_error {
    invalid_memory_resource,
  };

  [[nodiscard]]
  static auto make(view_info const &info) -> expected<view, make_error>;

private:
};

} // namespace gzn::app
