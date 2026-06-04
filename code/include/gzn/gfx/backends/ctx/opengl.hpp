#if defined(GZN_GFX_BACKEND_OPENGL)

#  include <span>

#  include "gzn/fnd/containers/pool.hpp"
#  include "gzn/fnd/util/unsafe-any-ref.hpp"
#  include "gzn/gfx/render-capacities.hpp"
#  include "gzn/gfx/surface.hpp"

namespace gzn::gfx {
struct context_info;
} // namespace gzn::gfx

namespace gzn::gfx::backends::ctx {

struct opengl {
  static void load();

  static void unload();

  static bool is_available() noexcept;

  static auto calc_required_space_for(render_capacities const &caps) noexcept
    -> usize;

  static auto make_context_on(
    context_info const       &info,
    fnd::util::unsafe_any_ref extra
  ) -> opengl *;

  static auto setup(
    std::span<byte>     storage,
    context_info const &info,
    surface_proxy      &surface
  ) -> bool;

  static void destroy();
};

} // namespace gzn::gfx::backends::ctx

#endif // defined(GZN_GFX_BACKEND_OPENGL)
