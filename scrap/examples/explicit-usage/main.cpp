#include <cstdio>
#include <cstdlib>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <glm/vec2.hpp>
#include <gzn/fnd/owner.hpp>

void DrawAQuad() {
  glClearColor(1.0, 1.0, 1.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-1., 1., -1., 1., 1., 20.);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(0., 0., 10., 0., 0., 0., 0., 1., 0.);

  glBegin(GL_QUADS);
  glColor3f(1., 0., 0.);
  glVertex3f(-.75, -.75, 0.);
  glColor3f(0., 1., 0.);
  glVertex3f(.75, -.75, 0.);
  glColor3f(0., 0., 1.);
  glVertex3f(.75, .75, 0.);
  glColor3f(1., 1., 0.);
  glVertex3f(-.75, .75, 0.);
  glEnd();
}

namespace test {

struct view_info {
  std::string_view title{};
  glm::i32vec2     pos{};
  glm::u32vec2     size{};
};

struct view {
  view_info    info{};
  Display     *display{};
  XVisualInfo *visual_info{};
  GLXContext   gl_context{};
  Window       root{};
  Window       window{};
  int          screen_id{};

  ~view() {
    if (display) {
      glXDestroyContext(display, gl_context);
      glXMakeCurrent(display, None, NULL);
      XDestroyWindow(display, window);
      XCloseDisplay(display);
    }
  }

  static auto make(gzn::fnd::util::allocator_type auto &alloc, view_info info)
    -> gzn::fnd::stack_owner<view> {
    Display *display{ XOpenDisplay(nullptr) };
    if (!display) {
      std::fprintf(stderr, "\n\tcannot connect to X server\n\n");
      return {};
    }

    int screen{ DefaultScreen(display) };
    int attributes[]{ GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };

    XVisualInfo *visual_info{ glXChooseVisual(display, screen, attributes) };
    if (visual_info == nullptr) {
      std::fprintf(stderr, "\n\tno appropriate visual found\n\n");
      std::exit(EXIT_FAILURE);
    }

    Window root{ RootWindow(display, screen) };
    std::printf("\n\tvisual %p selected\n", (void *)visual_info->visualid);

    XSetWindowAttributes window_attributes{
      .event_mask = ExposureMask | KeyPressMask,
      .colormap   = XCreateColormap(
        display, root, visual_info->visual, AllocNone
      )
    };
    Window window{ XCreateWindow(
      /* display      */ display,
      /* parent       */ root,
      /* x            */ info.pos.x,
      /* y            */ info.pos.x,
      /* width        */ info.size.x,
      /* height       */ info.size.y,
      /* border_width */ 0u,
      /* depth        */ visual_info->depth,
      /* class        */ InputOutput,
      /* visual       */ visual_info->visual,
      /* valuemask    */ CWColormap | CWEventMask,
      /* attributes   */ &window_attributes
    ) };

    XMapWindow(display, window);
    XStoreName(display, window, std::data(info.title));

    GLXContext glc{ glXCreateContext(display, visual_info, NULL, GL_TRUE) };

    glXMakeCurrent(display, window, glc);
    glViewport(0, 0, info.size.x, info.size.y);

    return gzn::fnd::stack_owner<view>{ alloc, info, display, visual_info,
                                        glc,   root, window,  screen };
  }
};

} // namespace test

int main(int argc, char *argv[]) {
  using namespace gzn;
  fnd::stack_arena_allocator<128> view_storage{};

  auto view{ test::view::make(
    view_storage, test::view_info{ .title{ "XLib Test application" } }
  ) };
  if (!view.is_alive()) { return EXIT_FAILURE; }

  glEnable(GL_DEPTH_TEST);

  auto const key_q{ XKeysymToKeycode(view->display, XStringToKeysym("q")) };

  XEvent event;
  bool   running{ true };
  while (running) {
    XNextEvent(view->display, &event);

    switch (event.type) {
      case KeyPress: {
        if (event.xkey.keycode == key_q) { running = false; }
      } break;

      case Expose: {
        glm::u32vec2 const size{ static_cast<u32>(event.xexpose.width),
                                 static_cast<u32>(event.xexpose.height) };
        std::printf("Exposed with [%u, %u]\n", size.x, size.y);
        if (size != view->info.size) {
          view->info.size = size;
          glViewport(0, 0, size.x, size.y);
        }
        DrawAQuad();
        glXSwapBuffers(view->display, view->window);
      } break;
    }
  }
}
