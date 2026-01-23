#pragma once

#include <glm/vec2.hpp>

#include "gzn/fnd/owner.hpp"

namespace gzn::gfx {

class context;

struct text_draw_info {
  std::string_view text{};
  glm::vec2        pos{};
  u32              color{ 0xFF'FF'FF'FF };
};

struct filled_rect_draw_info {
  glm::vec2 pos{};
  glm::vec2 size{};
  u32       color{ 0xFF'FF'FF'FF };
};

struct cmd final {
  cmd()  = delete;
  ~cmd() = delete;

  static void start(context &ctx, u32 clear_color);

  static void draw(context &ctx, text_draw_info info);
  static void draw(context &ctx, filled_rect_draw_info info);

  static void submit(context &ctx);
  static void present(context &ctx);
};

struct scoped_cmd {
  fnd::ref<context> ctx{};

  scoped_cmd(fnd::ref<context> ctx, u32 clear_color = 0xFF'FF'FF'FF)
    : ctx{ std::move(ctx) } {
    cmd::start(*ctx, clear_color);
  }

  ~scoped_cmd() { cmd::submit(*ctx); }

  void draw(text_draw_info info) { cmd::draw(*ctx, std::move(info)); }

  void draw(filled_rect_draw_info info) { cmd::draw(*ctx, std::move(info)); }
};

} // namespace gzn::gfx
