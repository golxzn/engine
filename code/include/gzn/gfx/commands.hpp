#pragma once

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include "gzn/fnd/owner.hpp"

namespace gzn::gfx {

class context;

struct cmd_clear {
  glm::vec4 color;
};

struct cmd final {
  cmd()  = delete;
  ~cmd() = delete;

  static void setup_for(context &ctx);
  static void start(context &ctx);

  static void clear(context &ctx, cmd_clear const &clr);

  static void submit(context &ctx);
  static void present(context &ctx);
};

struct scoped_cmd {
  fnd::ref<context> ctx{};

  scoped_cmd(fnd::ref<context> ctx)
    : ctx{ std::move(ctx) } {
    cmd::start(*ctx);
  }

  ~scoped_cmd() { cmd::submit(*ctx); }

  constexpr void clear(cmd_clear const &clr) { cmd::clear(*ctx, clr); }
};

} // namespace gzn::gfx
