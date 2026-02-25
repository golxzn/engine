#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/vec3.hpp>
#include <gzn/application>
#include <gzn/foundation>
#include <gzn/graphics>


std::string_view inline constexpr vertex_shader{ R"glsl(#version 330 core

layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec2 a_uv;

out vec2 v_uv;

void main() {
  v_uv = a_uv;
  gl_Position = vec4(a_pos, 1.0);
}
)glsl" };

std::string_view inline constexpr fragment_shader{ R"glsl(#version 330 core

in vec2 v_uv;

out vec4 out_color;

// uniform sampler2D u_texture;

void main() {
  // out_color = texture(u_texture, v_uv);
  out_color = vec4(v_uv.rgr, 1.0);
}
)glsl" };

int main() {
  using namespace gzn;
  fnd::base_allocator alloc{};

  auto view{
    app::view::make<fnd::stack_owner>(
      alloc,
      {
        .size{ 1920, 1080 },
        }
    )
  };
  if (!view.is_alive()) { return EXIT_FAILURE; }

  auto render{ gfx::context::make<fnd::stack_owner>(
    alloc,
    gfx::context_info{
      .backend = gfx::backend_type::opengl,
      .surface_builder{ view->get_surface_proxy_builder(alloc) } }
  ) };
  if (!render.is_alive()) { return EXIT_FAILURE; }

  auto const version{ gladLoadGL() };
  if (version == 0) { return EXIT_FAILURE; }

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
        glViewport(0, 0, resize->size.x, resize->size.y);
      }
    }

    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(program);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_SHORT, nullptr);

    gfx::cmd::present(*render);
  }
}
