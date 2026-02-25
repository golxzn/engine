#if defined(GZN_GFX_BACKEND_METAL)

#  include "gzn/fnd/util/unsafe_any_ref.hpp"
#  include "gzn/gfx/commands.hpp"

namespace gzn::gfx::backends {

struct metal {
  static void clear(fnd::util::unsafe_any_ref ctx, cmd_clear const &data) {}

  static void submit(fnd::util::unsafe_any_ref ctx) {}
};

} // namespace gzn::gfx::backends

#endif // defined(GZN_GFX_BACKEND_METAL)
