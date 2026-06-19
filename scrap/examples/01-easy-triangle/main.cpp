#include <array>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

// clang-format off
#include <gzn/foundation>
#include <gzn/graphics>
#include <gzn/application>
// clang-format on

std::string_view constexpr vertex_shader{ R"glsl(#version 400

layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec4 a_color;

layout(binding = 1) uniform mat4 u_mvp;

out vec4 frag_color;

void main() {
	frag_color = a_color;
	gl_Position = vec4(a_pos, 1.0);
}
)glsl" };

std::string_view constexpr fragment_shader{ R"glsl(#version 400
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
  fnd::in_stack_allocator<256> view_storage{};
  auto view{ app::view::make<fnd::stack_owner>(view_storage, {
    .title{ "Triangle test" },
    .size{ 1940u, 1080u }
  }) };
  // clang-format on

  if (!view.is_alive()) {
    // logging error
    return EXIT_FAILURE;
  }

  fnd::heap_allocator gfx_storage{};
  // clang-format off
  auto constexpr gfx_backend{ gfx::backend_type::vulkan };
  auto ctx{ gfx::context::make(gfx_storage, {
    .backend = gfx_backend,
    .surface{ view->make_surface_proxy(gfx_backend) },
  }) };
  if (!ctx.is_valid()) {
    return EXIT_FAILURE;
  }
  gfx::cmd::init(ctx, {
    .buffers_count = 1,
  });

  auto main_pass{ gfx::res::make(ctx, gfx::res_render_pass{
    .name{ "Main Pass" },
    .color_attachments{ {
      gfx::attachment_info{ .format = gfx::format_type::rgba_u8 }
    } },
  }) };


  gfx::attribute_layout const static triangle_layout{
    .bindings{ {
      gfx::binding_info{ .stride = sizeof(vertex), .rate = gfx::usage_rate::per_vertex },
    } },
    .description{ c_array<gfx::attribute_description const, 2>{
      { .binding = 0, .offset = offsetof(vertex, pos), .format = gfx::format_type::rgb_f32 },
      { .binding = 0, .offset = offsetof(vertex, col), .format = gfx::format_type::rgba_f32 },
    } },
  };

  fnd::handle<gfx::buffer> const triangle{ gfx::res::make(ctx, gfx::res_vertex_buffer{
    .description{
      .name{ "triangle" },
      .usage{ gfx::shader_type::vertex },
      .memory = gfx::buffer_memory_type::gpu,
      .init_data{ {
        vertex{ .pos{  0.0f, -0.5f, 0.0f }, .col{ 1.0f, 0.0f, 0.0f, 1.0f } },
        vertex{ .pos{  0.5f,  0.5f, 0.0f }, .col{ 0.0f, 1.0f, 0.0f, 1.0f } },
        vertex{ .pos{ -0.5f,  0.5f, 0.0f }, .col{ 0.0f, 0.0f, 1.0f, 1.0f } },
      } },
    },
    .layout{ triangle_layout }
  }) };


  fnd::handle<gfx::buffer> mvp_buffer{ gfx::res::make(ctx, gfx::res_buffer{
    .name{ "mvp" },
    .usage{ gfx::shader_type::vertex },
    .memory = gfx::buffer_memory_type::cpu_to_gpu,
    .size   = sizeof(glm::mat4),
  }) };

  gfx::buffer_view<glm::mat4> mvp{ mvp_buffer } ;
  mvp = glm::mat4{
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f,
  };

  fnd::handle const pipeline{ gfx::res::make(ctx, gfx::res_pipeline{
    .name{ "primitive" },
    .pass{ main_pass },
    .shaders{ c_array<gfx::res_shader, 2>{
      { .type{ gfx::shader_type::vertex },   .source_code{ vertex_shader }   },
      { .type{ gfx::shader_type::fragment }, .source_code{ fragment_shader } }
    } },
    .layout{ triangle_layout },
    .binding_groups{ {
      gfx::res::make(ctx, gfx::res_binding_group{
        .name{ "Transform" },
        .bindings{ {
          gfx::layout_binding_info{
            .binding = 0,
            .count = 1,
            .type = gfx::descriptor_type::uniform_buffer,
            .stages{ gfx::shader_type::vertex },
          },
        } },
      })
    } },
  }) };


  // commands is a static structure which handles commands
  auto draw_triangle_commands{ gfx::cmd::get({
    .index = 0,
  }) };

  gfx::cmd::add(draw_triangle_commands, gfx::cmd_begin_debug_group{
    .label{ "Triangle Drawing" },
  });
  gfx::cmd::add(draw_triangle_commands, gfx::cmd_clear_color{
    .rgba{ 0.0f, 0.0f, 0.0f, 1.0f },
  });
  gfx::cmd::add(draw_triangle_commands, gfx::cmd_depth_stencil{
    .depth   = 1.0f,
    .stencil = 0u,
  });
  gfx::cmd::add(draw_triangle_commands, gfx::cmd_bind_render_pass{
    .pass = main_pass,
  });
  gfx::cmd::add(draw_triangle_commands, gfx::cmd_bind_pipeline{
    .pipeline = pipeline,
  });
  gfx::cmd::add(draw_triangle_commands, gfx::cmd_bind_buffer{
    .buffer = mvp_buffer,
  });
  gfx::cmd::add(draw_triangle_commands, gfx::cmd_bind_buffer{
    .buffer = triangle,
  });
  gfx::cmd::add(draw_triangle_commands, gfx::cmd_draw{
    .vertex_count = 3,
  });
  gfx::cmd::add(draw_triangle_commands, gfx::cmd_end_debug_group{});
  // clang-format on

  while (true) {
    gfx::cmd::begin_frame(ctx);

    gfx::cmd::submit(draw_triangle_commands);

    gfx::cmd::end_frame(ctx);
  }
}
