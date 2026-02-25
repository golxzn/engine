#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/vec3.hpp>
#include <gzn/application>
#include <gzn/foundation>
#include <gzn/graphics>

std::string_view inline constexpr vertex_shader{ R"glsl(#version 300 es

precision highp float;

layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec2 a_uv;

out vec2 v_uv;

void main() {
  v_uv = a_uv;
  gl_Position = vec4(a_pos, 1.0);
}
)glsl" };

std::string_view inline constexpr fragment_shader{ R"glsl(#version 300 es

precision highp float;

in vec2 v_uv;

out vec4 out_color;

// uniform sampler2D u_texture;

void main() {
  // out_color = texture(u_texture, v_uv);
  out_color = vec4(v_uv.rgr, 1.0);
}
)glsl" };

struct gl_framebuffer {
  enum { COLOR, DEPTH, STENCIL, COUNT };

  GLuint                    handle{};
  std::array<GLuint, COUNT> textures{};
  glm::u32vec2              size{};

  void bind() const { glBindFramebuffer(GL_FRAMEBUFFER, handle); }

  void bind_read() const { glBindFramebuffer(GL_READ_FRAMEBUFFER, handle); }

  void bind_draw() const { glBindFramebuffer(GL_DRAW_FRAMEBUFFER, handle); }

  void unbind() const { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

  void unbind_read() const { glBindFramebuffer(GL_READ_FRAMEBUFFER, 0); }

  void unbind_draw() const { glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); }

  void blit(glm::u32vec2 const view_size) const {
    // clang-format off
    glBlitFramebuffer(
      0, 0, size.x, size.y,
      0, 0, view_size.x, view_size.y,
      GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT,
      GL_NEAREST
    );
    // clang-format on
  }

  static auto remake(gl_framebuffer &fb, glm::u32vec2 size) {
    destroy(fb);
    fb = make(size);
  }

  static auto make(glm::u32vec2 size) -> gl_framebuffer {

    std::array<GLuint, COUNT> textures;
    glGenTextures(std::size(textures), std::data(textures));
    glBindTexture(GL_TEXTURE_2D, textures[COLOR]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(
      GL_TEXTURE_2D,
      0,
      GL_RGBA,
      size.x,
      size.y,
      0,
      GL_RGBA,
      GL_UNSIGNED_BYTE,
      nullptr
    );

    glBindTexture(GL_TEXTURE_2D, textures[DEPTH]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(
      GL_TEXTURE_2D,
      0,
      GL_DEPTH_COMPONENT,
      size.x,
      size.y,
      0,
      GL_DEPTH_COMPONENT,
      GL_UNSIGNED_BYTE,
      nullptr
    );

    glBindTexture(GL_TEXTURE_2D, textures[STENCIL]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(
      GL_TEXTURE_2D,
      0,
      GL_DEPTH24_STENCIL8,
      size.x,
      size.y,
      0,
      GL_DEPTH_STENCIL,
      GL_UNSIGNED_INT_24_8,
      nullptr
    );


    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(
      GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[COLOR], 0
    );
    glFramebufferTexture2D(
      GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, textures[DEPTH], 0
    );
    glFramebufferTexture2D(
      GL_FRAMEBUFFER,
      GL_DEPTH_STENCIL_ATTACHMENT,
      GL_TEXTURE_2D,
      textures[STENCIL],
      0
    );
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      std::fprintf(stderr, "[gl_framebuffer] Framebuffer is not complete");
      return {};
    }

    return gl_framebuffer{ .handle   = framebuffer,
                           .textures = textures,
                           .size     = size };
  }

  static void destroy(gl_framebuffer &fb) {
    if (fb.handle != 0) {
      glDeleteTextures(std::size(fb.textures), std::data(fb.textures));
      glDeleteFramebuffers(1, &fb.handle);
      fb = { .size = fb.size };
    }
  }
};

struct egl_config_info {
  EGLint buffer_size;
  EGLint red, green, blue, alpha;
  EGLint depth, stencil;

  static auto load(EGLDisplay display, EGLConfig config) {
    egl_config_info out;
    eglGetConfigAttrib(display, config, EGL_BUFFER_SIZE, &out.buffer_size);
    eglGetConfigAttrib(display, config, EGL_RED_SIZE, &out.red);
    eglGetConfigAttrib(display, config, EGL_GREEN_SIZE, &out.green);
    eglGetConfigAttrib(display, config, EGL_BLUE_SIZE, &out.blue);
    eglGetConfigAttrib(display, config, EGL_ALPHA_SIZE, &out.alpha);
    eglGetConfigAttrib(display, config, EGL_DEPTH_SIZE, &out.depth);
    eglGetConfigAttrib(display, config, EGL_STENCIL_SIZE, &out.stencil);
    return out;
  }

  auto score(egl_config_info const &templ) const noexcept -> gzn::s32 {
    static constexpr gzn::s32 mismatch_penalty{ -1000 };
    static constexpr gzn::s32 max_score{ 32 };

    auto constexpr match{ [](auto comp, auto target, auto scale) -> gzn::s32 {
      if (comp == target) { return max_score; }

      auto const requested_but_not_present{ comp == 0 && target > 0 };
      auto const excluded_but_present{ comp > 0 && target == 0 };
      auto const out_of_color_range{ scale > 1 && comp > 8 && target <= 8 };
      if (requested_but_not_present || excluded_but_present ||
          out_of_color_range) {
        return mismatch_penalty;
      }

      return scale * (comp - target);
    } };
    // clang-format off
    return match(buffer_size, templ.buffer_size, 1)
         + match(depth,       templ.depth,       1)
         + match(stencil,     templ.stencil,     1)
         + match(red,         templ.red,         4)
         + match(green,       templ.green,       4)
         + match(green,       templ.green,       4)
         + match(blue,        templ.blue,        4)
         + match(alpha,       templ.alpha,       4);
    // clang-format on
  }
};

struct egl_context {
  EGLDisplay display{ EGL_NO_DISPLAY };
  EGLContext context{ EGL_NO_CONTEXT };

  void make_current() const {
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, context);
  }

  static auto make(gzn::app::native_view_handle handle, glm::u32vec2 size)
    -> egl_context {
    // auto display{ eglGetDisplay(EGL_DEFAULT_DISPLAY) };
    auto display{
      eglGetPlatformDisplay(EGL_PLATFORM_X11_EXT, handle, nullptr)
    };
    // auto display{ eglGetCurrentDisplay() };
    if (display == EGL_NO_DISPLAY) {
      std::fprintf(stderr, "[EGL] Failed to get display");
      return {};
    }

    EGLint major, minor;
    if (!eglInitialize(display, &major, &minor)) {
      std::fprintf(stderr, "[EGL] Failed to initialize EGL");
      return {};
    }
    std::printf("[EGL] Version: %d.%d\n", major, minor);
    std::fflush(stdout);

    if (!eglBindAPI(EGL_OPENGL_ES_API)) { return {}; }

    auto constexpr MAX_CONFIGS{ 32 };
    egl_config_info const wanted_config{
      .buffer_size = 32,
      .red         = 8,
      .green       = 8,
      .blue        = 8,
      .alpha       = 8,
      .depth       = 24,
      .stencil     = 8,
    };
    // clang-format off
    EGLint const config_attrs[]{
      EGL_SURFACE_TYPE, EGL_WINDOW_BIT,         // Surface type: Window
      EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,  // Support OpenGL ES 3.0
      EGL_BLUE_SIZE, wanted_config.blue,        // 8 bits for blue channel
      EGL_GREEN_SIZE, wanted_config.green,      // 8 bits for green
      EGL_RED_SIZE, wanted_config.red,          // 8 bits for red
      EGL_ALPHA_SIZE, wanted_config.alpha,      // 8 bits for alpha
      EGL_DEPTH_SIZE, wanted_config.depth,      // 24-bit depth buffer
      EGL_NONE
    };
    // clang-format on
    EGLConfig configs[MAX_CONFIGS];
    EGLint    configs_count;
    if (!eglChooseConfig(
          display, config_attrs, configs, MAX_CONFIGS, &configs_count
        )) {
      return {};
    }

    auto config{ select_best_config(
      display, wanted_config, { configs, static_cast<size_t>(configs_count) }
    ) };

    // clang-format off
    EGLint const context_attrs[]{
      EGL_CONTEXT_CLIENT_VERSION, 3, // Request OpenGL ES 3.0
      EGL_NONE
    };
    // clang-format on
    auto context{ eglCreateContext(display, config, nullptr, context_attrs) };

    eglSwapInterval(display, 1); // enabled vsync

    return egl_context{
      .display = display,
      .context = context,
    };
  }

  static void destroy(egl_context &egl) {
    if (egl.context == EGL_NO_CONTEXT) { return; }

    eglMakeCurrent(
      egl.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT
    );
    eglDestroyContext(egl.display, egl.context);
    eglTerminate(egl.display);
    egl = {};
  }

  static auto select_best_config(
    [[maybe_unused]] EGLDisplay display,
    egl_config_info const      &most_wanted,
    std::span<EGLConfig const>  configs
  ) -> EGLConfig {

    auto info{ egl_config_info::load(display, configs[0]) };
    std::pair<gzn::usize, gzn::s32> selected{
      0, std::numeric_limits<gzn::s32>::min()
    };

    for (gzn::usize i{ 1 }; i < std::size(configs); ++i) {
      info = egl_config_info::load(display, configs[i]);
      auto const score{ info.score(most_wanted) };
      if (score > selected.second) { selected = std::make_pair(i, score); }
    }
    return configs[selected.first];
  }
};

int main() {
  using namespace gzn;
  fnd::base_allocator alloc{};

  auto view{
    app::view::make<fnd::stack_owner>(alloc, { .size{ 1920, 1080 } }
     )
  };
  if (!view.is_alive()) { return EXIT_FAILURE; }

  auto constexpr backend{ gfx::backend_type::opengl };
  auto render{ gfx::context::make<fnd::stack_owner>(
    alloc,
    gfx::context_info{
      .backend = backend,
      .surface_builder{ view->get_surface_builder(alloc, backend) } }
  ) };
  if (!render.is_alive()) { return EXIT_FAILURE; }

#pragma region EGL INIT

  auto egl{ egl_context::make(view->get_native_handle(), view->get_size()) };
  egl.make_current();

  auto fb{ gl_framebuffer::make(view->get_size()) };

#pragma endregion EGL INIT


  auto constexpr build_shader{ [](auto type, auto src) {
    auto shader{ glCreateShader(type) };

    GLchar const *const data{ std::data(src) };
    auto const          size{ static_cast<GLint>(std::size(src)) };
    glShaderSource(shader, 1, &data, &size);
    glCompileShader(shader);

    GLint is_compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &is_compiled);
    if (is_compiled != GL_TRUE) {
      GLsizei log_length;
      GLchar  message[1024];
      glGetShaderInfoLog(shader, 1024, &log_length, message);
      std::fprintf(
        stderr,
        "FAILED TO CREATE SHADER: %.*s\n",
        static_cast<int>(log_length),
        message
      );
      std::fflush(stderr);
      std::exit(EXIT_FAILURE);
    }
    return shader;
  } };

  auto vs{ build_shader(GL_VERTEX_SHADER, vertex_shader) };
  auto fs{ build_shader(GL_FRAGMENT_SHADER, fragment_shader) };

  auto program{ glCreateProgram() };
  glAttachShader(program, vs);
  glAttachShader(program, fs);
  glLinkProgram(program);

  glDeleteShader(vs);
  glDeleteShader(fs);

  GLint is_linked;
  glGetProgramiv(program, GL_LINK_STATUS, &is_linked);
  if (is_linked != GL_TRUE) {
    GLsizei log_length;
    GLchar  message[1024];
    glGetProgramInfoLog(program, 1024, &log_length, message);
    std::fprintf(
      stderr,
      "FAILED TO LINK PROGRAM: %.*s\n",
      static_cast<int>(log_length),
      message
    );
    std::fflush(stderr);
    return EXIT_FAILURE;
  }

  // clang-format off
  struct vertex {
    glm::vec3 pos{};
    glm::vec2 uv{};
  };
  fnd::raw_data static const vertices{ {
    vertex{ .pos{ -0.5f, -0.5f, 0.0f }, .uv{ 0.0f, 0.0f } },
    vertex{ .pos{  0.5f,  0.5f, 0.0f }, .uv{ 1.0f, 1.0f } },
    vertex{ .pos{ -0.5f,  0.5f, 0.0f }, .uv{ 0.0f, 1.0f } },
    vertex{ .pos{  0.5f, -0.5f, 0.0f }, .uv{ 1.0f, 0.0f } }
  } };

  fnd::raw_data static const indices{ c_array<u16, 6>{ 0, 1, 2, 0, 3, 1 } };
  // clang-format on

  GLuint VAO;
  glGenVertexArrays(1, &VAO);

  enum { VBO, EBO, BUFFERS_COUNT };

  GLuint buffers[BUFFERS_COUNT];
  glGenBuffers(BUFFERS_COUNT, buffers);

  glBindVertexArray(VAO);
  auto const vertices_size{ vertices.bytes_count() };
  glBindBuffer(GL_ARRAY_BUFFER, buffers[VBO]);
  glBufferData(GL_ARRAY_BUFFER, vertices_size, nullptr, GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, vertices_size, std::data(vertices));

  auto const indices_size{ indices.bytes_count() };
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[EBO]);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_size, nullptr, GL_STATIC_DRAW);
  glBufferSubData(
    GL_ELEMENT_ARRAY_BUFFER, 0, indices_size, std::data(indices)
  );

  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);

  auto const stride{ vertices.stride() };
  glVertexAttribPointer(
    0, 3, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(vertex, pos)
  );
  glVertexAttribPointer(
    1, 2, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(vertex, uv)
  );


  glClearColor(0.435f, 0.376f, 0.541f, 1.0f);

  bool       running{ true };
  app::event event{};
  while (running) {
    while (view->take_next_event(event)) {
      if (auto key{ std::get_if<app::event_key_pressed>(&event) }; key) {
        if (fnd::util::any_from(key->key, app::keys::q, app::keys::escape)) {
          running = false;
          break;
        }
      } else if (auto resize{ std::get_if<app::event_resize>(&event) };
                 resize) {
        if (resize->size != fb.size) {
          gl_framebuffer::remake(fb, resize->size);
          std::printf("RESIZED: %d, %d\n", resize->size.x, resize->size.y);
          std::fflush(stdout);
        }
      }
    }
    auto const view_size{ view->get_size() };

    if (view_size.x == 0 || view_size.y == 0) {
      usleep(10 * 1000);
      continue;
    }

    fb.bind_draw();
    glViewport(0, 0, fb.size.x, fb.size.y);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glUseProgram(program);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_SHORT, nullptr);

    fb.unbind_draw();

    fb.bind_read();
    fb.blit(view_size);
    fb.unbind_read();

    glViewport(0, 0, view_size.x, view_size.y);
    gfx::cmd::present(*render);
  }
}
