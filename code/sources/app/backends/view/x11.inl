#include <array>
#include <limits>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GL/glx.h>
#include <X11/X.h>
#include <X11/XKBlib.h>
#include <X11/Xlib.h>

#include "gzn/app/view.hpp"
#include "gzn/fnd/definitions.hpp"
#include "gzn/gfx/context.hpp"

namespace gzn::app::backends {

inline constexpr u64 max_backends{ 4 };

#if defined(GZN_GFX_BACKEND_METAL)
struct mtl_context {};
#endif // defined(GZN_GFX_BACKEND_METAL)

#if defined(GZN_GFX_BACKEND_VULKAN)
struct vk_context {};
#endif // defined(GZN_GFX_BACKEND_VULKAN)


#if defined(GZN_GFX_BACKEND_OPENGL)

struct ogl_context {
  GLXContext context{};
};

struct egl_context {
  EGLDisplay display{ EGL_NO_DISPLAY };
  EGLSurface surface{ EGL_NO_SURFACE };
  EGLContext context{ EGL_NO_CONTEXT };
};

#endif // defined(GZN_GFX_BACKEND_OPENGL)

using graphics_data = std::variant<
#if defined(GZN_GFX_BACKEND_METAL)
  mtl_context,
#endif // defined(GZN_GFX_BACKEND_METAL)

#if defined(GZN_GFX_BACKEND_VULKAN)
  vk_context,
#endif // defined(GZN_GFX_BACKEND_VULKAN)

#if defined(GZN_GFX_BACKEND_OPENGL)
  ogl_context,
  egl_context,
#endif // defined(GZN_GFX_BACKEND_OPENGL)
  std::monostate>;

struct backend_context {
  Display      *display{};
  XVisualInfo  *visual_info{};
  Window        root{};
  Window        window{};
  int           screen_id{};
  glm::u32vec2  size{};
  graphics_data gfx{};
};

struct view {
  inline static fnd::stack_arena_allocator<512>           alloc{};
  inline static std::array<backend_context, max_backends> contexts{};
  inline static std::array<bool, max_backends>            ctx_in_use{};

  static auto construct(view_info const &info) -> backend_id {
    for (backend_id i{}; i < max_backends; ++i) {
      if (ctx_in_use[i]) { continue; }

      if (auto ctx{ make_backend(info) }; ctx.display) {
        contexts[i]   = ctx;
        ctx_in_use[i] = true;
        return i;
      }
      break;
    }
    return invalid_backend_id;
  }

  static auto is_in_use(backend_id const id) noexcept -> bool {
    return id < max_backends && ctx_in_use[id];
  }

  static void destroy(backend_id const id) {
    if (!is_in_use(id)) { return; }

    auto &ctx{ contexts[id] };

    XCloseDisplay(ctx.display);

    ctx_in_use[id] = false;
    ctx            = {};
  }

  static auto make_surface_proxy(backend_id id, gfx::backend_type backend)
    -> gfx::surface_proxy {
    switch (backend) {
#if defined(GZN_GFX_BACKEND_METAL)
      // case gfx::backend_type::metal: return make_metal_surface(id);
#endif // defined(GZN_GFX_BACKEND_METAL)

#if defined(GZN_GFX_BACKEND_VULKAN)
      case gfx::backend_type::vulkan: return {}; // make_vulkan_surface(id);
#endif // defined(GZN_GFX_BACKEND_VULKAN)

#if defined(GZN_GFX_BACKEND_OPENGL)
      // case gfx::backend_type::egl   : return make_egl_surface(id);
      case gfx::backend_type::opengl: return make_opengl_surface(id);
#endif // defined(GZN_GFX_BACKEND_OPENGL)

      default: break;
    }
    return {};
  }

  static auto take_event(backend_id id, event &ev) noexcept -> bool {
    if (!is_in_use(id)) { return false; }

    auto &ctx{ contexts[id] };
    if (XPending(ctx.display) == 0) { return false; }

    thread_local static XEvent pending;
    XNextEvent(ctx.display, &pending);

    switch (pending.type) {
      case KeyPress:
        ev = event_key_pressed{
          .time_ms = static_cast<u32>(pending.xkey.time),
          .key     = translate_to_key(pending.xkey),
        };
        break;

      case KeyRelease:
        ev = event_key_released{
          .time_ms = static_cast<u32>(pending.xkey.time),
          .key     = translate_to_key(pending.xkey),
        };
        break;

      case ButtonPress:
        ev = event_mouse_button_pressed{
          .time_ms = static_cast<u32>(pending.xbutton.time),
          .pos{ pending.xbutton.x, pending.xbutton.y },
          .button = translate_mouse_button(pending.xbutton.button)
        };
        break;
      case ButtonRelease:
        ev = event_mouse_button_released{
          .time_ms = static_cast<u32>(pending.xbutton.time),
          .pos{ pending.xbutton.x, pending.xbutton.y },
          .button = translate_mouse_button(pending.xbutton.button)
        };
        break;

      case MotionNotify:
        ev = event_mouse_motion{
          .time_ms = static_cast<u32>(pending.xmotion.time),
          .pos{ pending.xmotion.x, pending.xmotion.y },
        };
        break;

      // case EnterNotify     : break;
      // case LeaveNotify     : break;
      // case FocusOut        : ev = event_focus_lost{}; break;
      // case FocusIn         : ev = event_focus_gained{}; break;
      // case KeymapNotify    : break;
      // case Expose          : break;
      // case GraphicsExpose  : break;
      // case NoExpose        : break;
      // case VisibilityNotify: break;
      // case CreateNotify    : break;
      // case DestroyNotify   : break;
      // case UnmapNotify     : break;
      // case MapNotify       : break;
      // case MapRequest      : break;
      // case ReparentNotify  : break;
      // case ConfigureNotify : break;
      // case ConfigureRequest: break;
      // case GravityNotify   : break;
      case ResizeRequest:
        ctx.size.x = pending.xresizerequest.width;
        ctx.size.y = pending.xresizerequest.height;
        ev         = event_resize{ .size = ctx.size };
        break;

        // case CirculateNotify : break;
        // case CirculateRequest: break;
        // case PropertyNotify  : break;
        // case SelectionClear  : break;
        // case SelectionRequest: break;
        // case SelectionNotify : break;
        // case ColormapNotify  : break;
        // case ClientMessage   : break;
        // case MappingNotify   : break;


      default: return false;
    }
    return true;
  }

  static auto get_size(backend_id id) noexcept -> glm::u32vec2 {
    return is_in_use(id) ? contexts[id].size : glm::u32vec2{};
  }

  static auto get_native_handle(backend_id id) noexcept -> native_view_handle {
    return is_in_use(id)
           ? static_cast<native_view_handle>(contexts[id].display)
           : nullptr;
  }

private:
  static auto make_backend(view_info const &info) -> backend_context {
    auto display{ XOpenDisplay(std::data(info.display_name)) };
    if (!display) {
      // std::fprintf(stderr, "\n\tcannot connect to X server\n\n");
      return {};
    }

    auto screen{ DefaultScreen(display) };
    return backend_context{ .display     = display,
                            .visual_info = nullptr,
                            .root        = RootWindow(display, screen),
                            .window      = None,
                            .screen_id   = screen,
                            .size        = info.size };
  }

  static auto translate_mouse_button(u32 const button) noexcept
    -> mouse_buttons {
    std::printf("Mouse pressed %u\n", button);
    switch (button) {
      case Button1: return mouse_buttons::left;
      case Button2: return mouse_buttons::middle;
      case Button3: return mouse_buttons::right;
      case Button4: return mouse_buttons::wheel_up;
      case Button5: return mouse_buttons::wheel_down;
      case 6      : return mouse_buttons::wheel_left;
      case 7      : return mouse_buttons::wheel_right;
      default     : break;
    }
    return mouse_buttons::none;
  }

  static void construct_window_into(backend_context &ctx) {
    XSetWindowAttributes window_attributes{
      .background_pixmap = None, // background or None or ParentRelative
      .background_pixel  = BlackPixel(ctx.display, ctx.screen_id),
      .border_pixmap     = None,      // border of the window
      .border_pixel      = 0x0,       // border pixel value
      .bit_gravity       = 0,         // one of bit gravity values
      .win_gravity       = 0,         // one of the window gravity values
      .backing_store     = NotUseful, // NotUseful, WhenMapped, Always
      .backing_planes    = 0,         // planes to be preserved if possible
      .backing_pixel     = 0,         // value to use in restoring planes
      .save_under        = 0,         // should bits under be saved? (popups)
      .event_mask = ResizeRedirectMask | StructureNotifyMask | ExposureMask |
                    PointerMotionMask | KeyPressMask | KeyReleaseMask |
                    ButtonPressMask | ButtonReleaseMask,
      .do_not_propagate_mask = 0, // set of events that should not propagate
      .override_redirect     = false, // boolean value for override-redirect
      .colormap              = XCreateColormap(
        ctx.display, ctx.root, ctx.visual_info->visual, AllocNone
      ),
      .cursor = None // cursor to be displayed (or None)
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
  }

#if defined(GZN_GFX_BACKEND_OPENGL)
#  pragma region EGL

  static auto select_visual_info(backend_context &ctx, VisualID visual_id)
    -> bool {
    XVisualInfo vinfo_template{ .visualid = static_cast<VisualID>(visual_id) };
    int         visual_count{};
    ctx.visual_info = XGetVisualInfo(
      ctx.display, VisualIDMask, &vinfo_template, &visual_count
    );
    return ctx.visual_info != nullptr;
  }

  static auto make_egl_surface(backend_id id) -> gfx::surface_proxy {
    // clang-format off
    return gfx::surface_proxy{
      .setup{alloc, [id](gfx::context &) -> bool {
        return setup_egl(contexts[id]);
      } },
      .destroy{ alloc, [id](gfx::context &) {
        destroy_egl(contexts[id]);
      } },
      .present{ alloc, [id](gfx::context &) {
        auto &egl{ std::get<egl_context>(contexts[id].gfx) };
        eglSwapBuffers(egl.display, egl.surface);
       } },
      .get_size{ alloc, [id] -> glm::u32vec2 { return contexts[id].size; } }
    };
    // clang-format on
  }

  static auto setup_egl(backend_context &ctx) -> bool {
    // auto display{
    //   eglGetPlatformDisplay(EGL_PLATFORM_X11_EXT, ctx.display, nullptr)
    // };
    // if (display == nullptr) { return false; }
    //
    // if (EGLint major, minor; !eglInitialize(display, &major, &minor)) {
    //   return false;
    // }
    //
    // if (!eglBindAPI(EGL_OPENGL_ES_API)) { // I'm not sure it's required
    //   return false;
    // }
    //
    // EGLConfig config;
    // EGLint    configs_count;
    // // clang-format off
    // EGLint const attrs[]{
    //   EGL_SURFACE_TYPE, EGL_WINDOW_BIT,         // Surface type: Window
    //   EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,  // Support OpenGL ES 3.0
    //   EGL_BLUE_SIZE, 8,                         // 8 bits for blue channel
    //   EGL_GREEN_SIZE, 8,                        // 8 bits for green
    //   EGL_RED_SIZE, 8,                          // 8 bits for red
    //   EGL_ALPHA_SIZE, 8,                        // 8 bits for alpha
    //   EGL_DEPTH_SIZE, 24,                       // 24-bit depth buffer
    //   EGL_NONE
    // };
    // // clang-format on
    // if (!eglChooseConfig(display, attrs, &config, 1, &configs_count)) {
    //   return false;
    // }
    //
    // EGLint visual_id;
    // eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &visual_id);
    // if (!select_visual_info(ctx, visual_id)) { return false; }
    // construct_window_into(ctx);
    //
    // auto native_window{ reinterpret_cast<EGLNativeWindowType>(ctx.window) };
    // auto egl_surface{
    //   eglCreateWindowSurface(display, config, native_window, nullptr)
    // };
    // if (egl_surface == EGL_NO_SURFACE) {
    //   auto const err{ eglGetError() };
    //   std::fprintf(stderr, "ERR: %d\n", err);
    //   return false;
    // }
    //
    // // clang-format off
    // EGLint const context_attrs[]{
    //   EGL_CONTEXT_CLIENT_VERSION, 3,  // Request OpenGL ES 3.0
    //   EGL_NONE
    // };
    // // clang-format on
    //
    // auto egl_ctx{ eglCreateContext(display, config, nullptr, context_attrs)
    // }; if (egl_ctx == EGL_NO_CONTEXT) {
    //   eglDestroySurface(display, egl_surface);
    //   return false;
    // }
    //
    // if (!eglMakeCurrent(display, egl_surface, egl_surface, egl_ctx)) {
    //   eglDestroyContext(display, egl_ctx);
    //   eglDestroySurface(display, egl_surface);
    //   return false;
    // }
    //
    // ctx.gfx = egl_context{
    //   .display = display,
    //   .surface = egl_surface,
    //   .context = egl_ctx,
    // };
    //
    return true;
  }

  static void destroy_egl(backend_context &ctx) {
    // if (auto egl{ std::get_if<egl_context>(&ctx.gfx) }; egl) {
    //   eglMakeCurrent(
    //     egl->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT
    //   );
    //   eglDestroySurface(egl->display, egl->surface);
    //   eglDestroyContext(egl->display, egl->context);
    //   eglTerminate(egl->display);
    // }
  }

#  pragma endregion EGL

#  pragma region OPENGL

  static auto make_opengl_surface(backend_id id) -> gfx::surface_proxy {
    // clang-format off
    return gfx::surface_proxy{
      .setup{ alloc, [id](gfx::context &) -> bool {
        return setup_opengl(contexts[id]);
      } },
      .destroy{ alloc, [id](gfx::context &) {
        destroy_opengl(contexts[id]);
      } },
      .present{ alloc, [id](gfx::context &) {
        auto &ctx{ contexts[id] };
        glXSwapBuffers(ctx.display, ctx.window);
       } },
      .get_size{ alloc, [id] -> glm::u32vec2 { return contexts[id].size; } },
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
    // std::printf("\n\tvisual %p selected\n", (void
    // *)visual_info->visualid);

    construct_window_into(ctx);

    auto gl_ctx{
      glXCreateContext(ctx.display, ctx.visual_info, NULL, GL_TRUE)
    };
    if (gl_ctx == nullptr) { return false; }

    ctx.gfx = ogl_context{ .context = gl_ctx };
    glXMakeCurrent(ctx.display, ctx.window, gl_ctx);
    glViewport(0, 0, ctx.size.x, ctx.size.y);

    return true;
  }

  static void destroy_opengl(backend_context &ctx) {
    if (auto ogl{ std::get_if<ogl_context>(&ctx.gfx) }; ogl) {
      glXDestroyContext(ctx.display, ogl->context);
      ctx.gfx = {};
    }
    glXMakeCurrent(ctx.display, None, nullptr);
    XDestroyWindow(ctx.display, ctx.window);
  }

#  pragma endregion OPENGL

#endif // defined(GZN_GFX_BACKEND_OPENGL)

  constexpr static auto translate_to_key(XKeyEvent &ev) noexcept -> keys {
    auto const group{ 0 };
    auto const level{ (ev.state & ShiftMask) ? 1 : 0 };
    auto keysym{ XkbKeycodeToKeysym(ev.display, ev.keycode, group, level) };
    if (keysym >= XK_a && keysym <= XK_z) { keysym -= XK_a - XK_A; }
    switch (keysym) {
      case XK_A: return keys::a;
      case XK_B: return keys::b;
      case XK_C: return keys::c;
      case XK_D: return keys::d;
      case XK_E: return keys::e;
      case XK_F: return keys::f;
      case XK_G: return keys::g;
      case XK_H: return keys::h;
      case XK_I: return keys::i;
      case XK_J: return keys::j;
      case XK_K: return keys::k;
      case XK_L: return keys::l;
      case XK_M: return keys::m;
      case XK_N: return keys::n;
      case XK_O: return keys::o;
      case XK_P: return keys::p;
      case XK_Q: return keys::q;
      case XK_R: return keys::r;
      case XK_S: return keys::s;
      case XK_T: return keys::t;
      case XK_U: return keys::u;
      case XK_V: return keys::v;
      case XK_W: return keys::w;
      case XK_X: return keys::x;
      case XK_Y: return keys::y;
      default  : break;
    }

    switch (keysym) {
      case XK_0: return keys::n_0;
      case XK_1: return keys::n_1;
      case XK_2: return keys::n_2;
      case XK_3: return keys::n_3;
      case XK_4: return keys::n_4;
      case XK_5: return keys::n_5;
      case XK_6: return keys::n_6;
      case XK_7: return keys::n_7;
      case XK_8: return keys::n_8;
      case XK_9: return keys::n_9;
      default  : break;
    }

    switch (keysym) {
      case XK_Control_L: return keys::left_control;
      case XK_Shift_L  : return keys::left_shift;
      case XK_Alt_L    : return keys::left_alt;
      case XK_Super_L  : [[fallthrough]];
      case XK_Meta_L   : return keys::left_system;

      case XK_Control_R: return keys::right_control;
      case XK_Shift_R  : return keys::right_shift;
      case XK_Alt_R    : return keys::right_alt;
      case XK_Super_R  : [[fallthrough]];
      case XK_Meta_R   : return keys::right_system;
      default          : break;
    }

    switch (keysym) {
      // arrows
      case XK_Left       : return keys::left;
      case XK_Right      : return keys::right;
      case XK_Up         : return keys::up;
      case XK_Down       : return keys::down;

      // navigation
      case XK_Escape     : return keys::escape;
      case XK_Menu       : return keys::menu;
      case XK_Page_Up    : return keys::pageup;
      case XK_Page_Down  : return keys::pagedown;
      case XK_End        : return keys::end;
      case XK_Home       : return keys::home;
      case XK_Insert     : return keys::insert;
      case XK_Delete     : return keys::del;

      // punctuation
      case XK_semicolon  : return keys::semicolon;
      case XK_comma      : return keys::comma;
      case XK_period     : return keys::period;
      case XK_apostrophe : return keys::apostrophe;
      case XK_quotedbl   : return keys::quote;
      case XK_slash      : return keys::slash;
      case XK_backslash  : return keys::backslash;
      case XK_grave      : return keys::tilde;
      case XK_equal      : return keys::equal;
      case XK_minus      : return keys::minus;
      case XK_space      : return keys::space;

      // enter / editing
      case XK_Return     : return keys::enter;
      case XK_BackSpace  : return keys::backspace;
      case XK_Tab        : return keys::tab;

      // misc
      case XK_Pause      : return keys::pause;
      case XK_Print      : return keys::print_screen;
      case XK_Scroll_Lock: return keys::scroll_lock;
      case XK_Caps_Lock  : return keys::caps_lock;
      case XK_Num_Lock   : return keys::num_lock;

      default            : break;
    }

    switch (keysym) {
      case XK_KP_0       : return keys::numpad_0;
      case XK_KP_1       : return keys::numpad_1;
      case XK_KP_2       : return keys::numpad_2;
      case XK_KP_3       : return keys::numpad_3;
      case XK_KP_4       : return keys::numpad_4;
      case XK_KP_5       : return keys::numpad_5;
      case XK_KP_6       : return keys::numpad_6;
      case XK_KP_7       : return keys::numpad_7;
      case XK_KP_8       : return keys::numpad_8;
      case XK_KP_9       : return keys::numpad_9;

      case XK_KP_Add     : return keys::add;
      case XK_KP_Subtract: return keys::subtract;
      case XK_KP_Multiply: return keys::multiply;
      case XK_KP_Divide  : return keys::divide;
      case XK_KP_Enter   : return keys::enter;
      case XK_KP_Decimal : return keys::decimal;

      default            : break;
    }

    switch (keysym) {
      case XK_F1 : return keys::f1;
      case XK_F2 : return keys::f2;
      case XK_F3 : return keys::f3;
      case XK_F4 : return keys::f4;
      case XK_F5 : return keys::f5;
      case XK_F6 : return keys::f6;
      case XK_F7 : return keys::f7;
      case XK_F8 : return keys::f8;
      case XK_F9 : return keys::f9;
      case XK_F10: return keys::f10;
      case XK_F11: return keys::f11;
      case XK_F12: return keys::f12;
      case XK_F13: return keys::f13;
      case XK_F14: return keys::f14;
      case XK_F15: return keys::f15;

      default    : break;
    }

    return keys::none;
  }
};

} // namespace gzn::app::backends
