#pragma once

#include <variant>

namespace gzn::app {

// clang-format off
enum class key {
  none,
  a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z,
  n_0, n_1, n_2, n_3, n_4, n_5, n_6, n_7, n_8, n_9,
  numpad_0, numpad_1, numpad_2, numpad_3, numpad_4,
  numpad_5, numpad_6, numpad_7, numpad_8, numpad_9,
  f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
  left_control , left_shift , left_alt , left_system , left_bracket,
  right_control, right_shift, right_alt, right_system, right_bracket,
  escape, menu, semicolon, comma, period, quote, slash, backslash, tilde,
  equal, dash, space, enter, backspace, tab, pageup, pagedown, end, home,
  insert, del, add, subtract, multiply, divide, left, right, up, down,
  pause,
};
// clang-format on

struct event_key {
  key code{ key::none };
  // fnd::time timestamp{};
};

struct event_focus_gained {};

struct event_focus_lost {};

struct event_key_pressed : event_key {};

struct event_key_released : event_key {};

using event = std::variant<
  std::monostate,
  event_focus_gained,
  event_focus_lost,
  event_key_pressed,
  event_key_released>;

} // namespace gzn::app
