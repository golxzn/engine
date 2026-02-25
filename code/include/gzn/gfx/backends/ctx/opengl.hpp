#if defined(GZN_GFX_BACKEND_OPENGL)

#  include <span>

#  include "gzn/fnd/containers/pool.hpp"
#  include "gzn/fnd/util/unsafe_any_ref.hpp"
#  include "gzn/gfx/render-capacities.hpp"
#  include "gzn/gfx/surface.hpp"

namespace gzn::gfx {
struct context_info;
} // namespace gzn::gfx

namespace gzn::gfx::backends::ctx {

struct opengl {
  static bool is_available() noexcept { return false; }

  static auto calc_required_space_for(render_capacities const &caps) noexcept
    -> usize {
    return 0;
  }

  static auto make_context_on(
    context_info const       &info,
    fnd::util::unsafe_any_ref extra
  ) -> opengl * {
    return nullptr;
  }

  static auto setup(
    std::span<byte>     storage,
    context_info const &info,
    surface_proxy      &surface
  ) -> bool {
    return false;
  }

  static void destroy() {}
};

} // namespace gzn::gfx::backends::ctx

#endif // defined(GZN_GFX_BACKEND_OPENGL)
