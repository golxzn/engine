#pragma once

#include <variant>

#include <glm/vec2.hpp>

#include "gzn/fnd/types.hpp"

namespace gzn::app {

// clang-format off
enum class keys {
  none,
  a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z,
  n_0, n_1, n_2, n_3, n_4, n_5, n_6, n_7, n_8, n_9,
  left_control , left_shift , left_alt , left_system , left_bracket,
  right_control, right_shift, right_alt, right_system, right_bracket,
  left, right, up, down,
  escape, menu, semicolon, comma, period, apostrophe, quote, slash, backslash,
  tilde, equal, minus, space, enter, backspace, tab, pageup, pagedown, end,
  home, insert, del, pause, print_screen, scroll_lock, caps_lock, num_lock,
  numpad_0, numpad_1, numpad_2, numpad_3, numpad_4,
  numpad_5, numpad_6, numpad_7, numpad_8, numpad_9,
  add, subtract, multiply, divide, decimal,
  f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15,
};
// clang-format on

enum class mouse_buttons {
  none,
  left,
  middle,
  right,
  wheel_up,
  wheel_down,
  wheel_left,
  wheel_right,
};

struct event_focus_gained {};

struct event_focus_lost {};

struct event_key_pressed {
  u32  time_ms{};
  keys key{ keys::none };
};

struct event_key_released {
  u32  time_ms{};
  keys key{ keys::none };
};

struct event_mouse_button_pressed {
  u32           time_ms{};
  glm::u32vec2  pos{};
  mouse_buttons button{ mouse_buttons::none };
};

struct event_mouse_button_released {
  u32           time_ms{};
  glm::u32vec2  pos{};
  mouse_buttons button{ mouse_buttons::none };
};

struct event_mouse_motion {
  u32          time_ms{};
  glm::u32vec2 pos{};
};

struct event_resize {
  glm::u32vec2 size{};
};

using event = std::variant<
  std::monostate,
  event_focus_gained,
  event_focus_lost,
  event_key_pressed,
  event_key_released,
  event_mouse_button_pressed,
  event_mouse_button_released,
  event_mouse_motion,
  event_resize
  //
  >;

} // namespace gzn::app
