#include "gzn/gfx/commands.hpp"

#include "gzn/gfx/context.hpp"

namespace gzn::gfx {

void cmd::start(context &ctx, u32 clear_color) {
  /// @todo IMPLEMENTATION
}

void cmd::draw(context &ctx, text_draw_info info) {
  /// @todo IMPLEMENTATION
}

void cmd::draw(context &ctx, filled_rect_draw_info info) {
  /// @todo IMPLEMENTATION
}

void cmd::submit(context &ctx) {
  /// @todo IMPLEMENTATION
}

void cmd::present(context &ctx) {
  ctx.m.surface.present(ctx);
}

} // namespace gzn::gfx
