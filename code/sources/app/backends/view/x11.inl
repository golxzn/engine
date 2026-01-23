#include <array>
#include <limits>

#include <GL/glx.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

#include "gzn/app/view.hpp"
#include "gzn/gfx/context.hpp"

namespace gzn::app::backends {

inline constexpr u64 max_backends{ 4 };

struct backend_context {
  Display     *display{};
  XVisualInfo *visual_info{};
  GLXContext   gl_context{};
  Window       root{};
  Window       window{};
  int          screen_id{};
  glm::u32vec2 size{};
};

struct view {


  inline static fnd::stack_arena_allocator<512>           alloc{};
  inline static std::array<backend_context, max_backends> internals{};
  inline static std::array<bool, max_backends>            in_use{};

  static auto construct(view_info const &info) -> backend_id {
    for (backend_id i{}; i < max_backends; ++i) {
      if (in_use[i]) { continue; }

      if (auto ctx{ make_backend(info) }; ctx.display) {
        internals[i] = ctx;
        in_use[i]    = true;
        return i;
      }
      break;
    }
    return invalid_backend_id;
  }

  static auto is_in_use(backend_id const id) noexcept -> bool {
    return id < max_backends && in_use[id];
  }

  static void destroy(backend_id const id) {
    if (!is_in_use(id)) { return; }

    auto &ctx{ internals[id] };

    XCloseDisplay(ctx.display);

    in_use[id] = false;
    ctx        = {};
  }

  static auto take_event(backend_id id, event &ev) -> bool {
    if (!is_in_use(id)) { return false; }

    auto &ctx{ internals[id] };
    if (XPending(ctx.display) == 0) { return false; }

    thread_local static XEvent event;
    XNextEvent(ctx.display, &event);

    switch (event.type) {
      case KeyPress: {
        ev = event_key_pressed{};
      } break;

      case KeyRelease: {
      } break;

      default: return false;
    }
    return true;
  }

  static auto make_surface_proxy(backend_id id, gfx::backend_type backend)
    -> gfx::surface_proxy {
    switch (backend) {
      case gfx::backend_type::opengl: return make_opengl_surface(id);

      default                       : break;
    }
    return {};
  }

private:
  static auto make_backend(view_info const &info) -> backend_context {
    auto display{ XOpenDisplay(nullptr) };
    if (!display) {
      // std::fprintf(stderr, "\n\tcannot connect to X server\n\n");
      return {};
    }

    auto screen{ DefaultScreen(display) };
    return backend_context{ .display     = display,
                            .visual_info = nullptr,
                            .gl_context  = nullptr,
                            .root        = RootWindow(display, screen),
                            .window      = None,
                            .screen_id   = screen,
                            .size        = info.size };
  }

  static auto make_scancode_key_lookup(Display *display) {
    // std::array<key, 256> keys{};
    // auto const key_q{ XKeysymToKeycode(display, XStringToKeysym("q")) };
  }

#pragma region OPENGL

  static auto make_opengl_surface(backend_id id) -> gfx::surface_proxy {
    // clang-format off
    return gfx::surface_proxy{
      .setup{alloc, [id](gfx::context &) -> bool {
        return setup_opengl(internals[id]);
      } },
      .destroy{ alloc, [id](gfx::context &) {
        destroy_opengl(internals[id]);
      } },
      .present{ alloc, [id](gfx::context &) {
        auto &ctx{ internals[id] };
        glXSwapBuffers(ctx.display, ctx.window);
       } },
      .get_size{ alloc, [id] -> glm::u32vec2 { return internals[id].size; } }
    };
    // clang-format on
  }

  static auto setup_opengl(backend_context &ctx) -> bool {
    /// @todo maybe we should take this parameters from display or somethign
    int attributes[]{ GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };

    ctx.visual_info = glXChooseVisual(ctx.display, ctx.screen_id, attributes);
    if (ctx.visual_info == nullptr) {
      // std::fprintf(stderr, "\n\tno appropriate visual found\n\n");
      return false;
    }
    // std::printf("\n\tvisual %p selected\n", (void *)visual_info->visualid);


    XSetWindowAttributes window_attributes{
      .event_mask = ExposureMask | KeyPressMask,
      .colormap   = XCreateColormap(
        ctx.display, ctx.root, ctx.visual_info->visual, AllocNone
      )
    };
    ctx.window = XCreateWindow(
      /* display      */ ctx.display,
      /* parent       */ ctx.root,
      /* x            */ 0,
      /* y            */ 0,
      /* width        */ ctx.size.x,
      /* height       */ ctx.size.y,
      /* border_width */ 0u,
      /* depth        */ ctx.visual_info->depth,
      /* class        */ InputOutput,
      /* visual       */ ctx.visual_info->visual,
      /* valuemask    */ CWColormap | CWEventMask,
      /* attributes   */ &window_attributes
    );

    XMapWindow(ctx.display, ctx.window);
    XStoreName(ctx.display, ctx.window, "PLACE TITLE HERE DUDE");

    ctx.gl_context = glXCreateContext(
      ctx.display, ctx.visual_info, NULL, GL_TRUE
    );
    if (ctx.gl_context == nullptr) { return false; }

    glXMakeCurrent(ctx.display, ctx.window, ctx.gl_context);
    glViewport(0, 0, ctx.size.x, ctx.size.y);

    return true;
  }

  static void destroy_opengl(backend_context &ctx) {
    glXDestroyContext(ctx.display, ctx.gl_context);
    glXMakeCurrent(ctx.display, None, nullptr);
    XDestroyWindow(ctx.display, ctx.window);
    ctx.gl_context = nullptr;
  }

#pragma endregion OPENGL
};

} // namespace gzn::app::backends
