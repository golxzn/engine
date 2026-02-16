#include <array>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

// clang-format off
#include <gzn/foundation>
#include <gzn/graphics>
#include <gzn/application>
// clang-format on

constexpr auto vertex_shader{ R"glsl(#version 400

layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec4 a_color;

layout(binding = 1) uniform mat4 u_mvp;

out vec4 frag_color;

void main() {
	frag_color = a_color;
	gl_Position = vec4(a_pos, 1.0);
}
)glsl" };

constexpr auto fragment_shader{ R"glsl(#version 400
in vec4 frag_color;
out vec4 color;

void main() {
	color = frag_color;
}
)glsl" };

struct vertex {
  glm::vec3 pos{};
  glm::vec4 col{};
};

int main() {
  using namespace gzn;
  using namespace gzn::fnd::name_literals;

  // clang-format off
  fnd::stack_arena_allocator<256> view_storage{};
  auto view{ app::view::make<fnd::stack_owner>(view_storage, {
    .title{ "Triangle test" },
    .size{ 1940u, 1080u }
  }) };
  // clang-format on

  if (!view.is_alive()) {
    // logging error
    return EXIT_FAILURE;
  }

  fnd::base_allocator gfx_storage{};
  // clang-format off
  auto constexpr gfx_backend{ gfx::backend_type::vulkan };
  auto ctx{ gfx::context::make<fnd::stack_owner>(gfx_storage, {
      .backend = gfx_backend,
      .surface_builder{
        view->get_surface_proxy_builder(gfx_storage, gfx_backend)
      }
  }) };
  gfx::cmd::setup_for(*ctx);

  auto const triangle{ gfx::buffer::make(ctx, {
    .name{ "triangle"_name },
    .usage  = gfx::shader::type::vertex,
    .memory = gfx::buffer::memory::gpu,
    .bindings{
    // .index = auto enumerate (-1)
      { .stride = sizeof(vertex), .rate = gfx::rate::per_vertex }
    },
    .description{
    // .location = auto enumerate (-1)
      { .binding = 0, .format = gfx::format::rgb_f32, .offset = offsetof(vertex, pos) }
      { .binding = 0, .format = gfx::format::rgb_f32, .offset = offsetof(vertex, pos) }
    },
    .initial_data{
      vertex{ .pos{  0.0f, -0.5f, 0.0f }, .col{ 1.0f, 0.0f, 0.0f, 1.0f } },
      vertex{ .pos{  0.5f,  0.5f, 0.0f }, .col{ 0.0f, 1.0f, 0.0f, 1.0f } },
      vertex{ .pos{ -0.5f,  0.5f, 0.0f }, .col{ 0.0f, 0.0f, 1.0f, 1.0f } },
    }
  }) };

  auto mvp_buffer{ gfx::buffer::make(ctx, {
    .name{ "mvp"_name },
    .size   = sizeof(glm::mat4),
    .usage  = gfx::shader::type::vertex,
    .memory = gfx::buffer::memory::gpu_cpu,
  }) };

  auto mvp{ gfx::buffer_view<glm::mat4>::make({
    .buffer = mvp_buffer
  }) };
  mvp = glm::mat4{
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f,
  };

  auto const pipeline{ gzn::pipeline::make(ctx, {
    .name{ "primitive"_name },
    .shaders{
      { .type = gfx::shader::type::vertex,   .data = vertex_shader,   .flags = gfx::shader::flags::compile },
      { .type = gfx::shader::type::fragment, .data = fragment_shader, .flags = gfx::shader::flags::compile }
    },
    .bindings{
      {
        .byte_stride = sizeof(vertex),
        .atributes{
          { .byte_offset = 0, .format = gfx::format::rgb32_float },
          { .byte_offset = 12, .format = gfx::format::rgba32_float },
        }
      }
    },
    .constants{},
    .uniform_binding_groups{
      gfx::binding::group::make_from_buffer(mvp_buffer)
    }
  }) };
  // clang-format on

  while (true) {
    gfx::cmd::clear(ctx, {});

    gfx::cmd::start(ctx);
      gfx::cmd::use(ctx, pipeline);
      gfx::cmd::push(ctx, mvp); // or mvp_buffer
      gfx::cmd::bind(ctx, triangle);
      gfx::cmd::draw(ctx, { .vertex_count = 3 });
    gfx::cmd::submit(ctx);

    gfx::cmd::present(cmd);
  }
}
