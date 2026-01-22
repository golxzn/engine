#include <array>

#include <gzn/core/view.hpp>
#include <gzn/fnd/owner.hpp>
#include <gzn/gfx.hpp>

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

union float3 {
  struct {
    float x, y, z;
  };

  std::array<float, 3> elements{};
};

union float4 {
  struct {
    float x, y, z, w;
  };

  std::array<float, 4> elements{};
};

union mat4 {
  struct {
    float a00, a01, a02, a03;
    float a10, a11, a12, a13;
    float a20, a21, a22, a23;
    float a30, a31, a32, a33;
  };

  std::array<float, 16> elements{};
};

struct vertex {
  float3 pos{};
  float4 col{};
};

int main() {
  using namespace gzn;

  // clang-format off
  expected view{ app::view::make({
      .title{ "Triangle test" },
      .size{ 1940u, 1080u }
  }) };
  // clang-format on
  if (!view.has_value()) {
    // logging error
  }

  fnd::stack_arena_allocator<256> view_storage{};
  fnd::stack_owner result{ view_storage, std::move(view).value() };

  // fnd::chunk_allocator<512> gfx_storage{};
  fnd::base_allocator gfx_storage{};
  fnd::stack_owner    ctx{ gfx::context::make(
    gfx_storage,
    {
         .backend = gfx::backend::any,
         .view    = fnd::ref{ view },
    }
  ) };

  auto const triangle{
    gfx::buffer::make(
      ctx,
      { .name{ "triangle"_name },
        .usage  = gfx::shader::type::vertex,
        .memory = gfx::buffer::memory::gpu,
        .initial_data{
          vertex{ .pos{ 0.0f, -0.5f, 0.0f },
                  .color{ 1.0f, 0.0f, 0.0f, 1.0f } },
          vertex{ .pos{ 0.5f, 0.5f, 0.0f }, .color{ 0.0f, 1.0f, 0.0f, 1.0f } },
          vertex{ .pos{ -0.5f, 0.5f, 0.0f },
                  .color{ 0.0f, 0.0f, 1.0f, 1.0f } },
        } }
    )
  };
  auto mvp_buffer{ gfx::buffer::make(
    {
      .name{ "mvp"_name },
      .size   = sizeof(mat4),
      .usage  = gfx::shader::type::vertex,
      .memory = gfx::buffer::memory::gpu_cpu,
    }
  ) };
  auto mvp{ gfx::buffer_view<mat4>::make({ .buffer = mvp_buffer }) };
  mvp = mat4{
    .elements{
              1.0f, 0.0f,
              0.0f, 0.0f,
              0.0f, 1.0f,
              0.0f, 0.0f,
              0.0f, 0.0f,
              1.0f, 0.0f,
              0.0f, 0.0f,
              0.0f, 1.0f,
              }
  };

  auto const pipeline{
    gzn::pipeline::make(
      { .name{ "primitive"_name },
                        .shaders{ { .type  = gfx::shader::type::vertex,
                    .data  = vertex_shader,
                    .flags = gfx::shader::flags::compile },
                  { .type  = gfx::shader::type::fragment,
                    .data  = fragment_shader,
                    .flags = gfx::shader::flags::compile } },
                        .bindings{
          { .byte_stride = sizeof(vertex),
            .atributes{
              { .byte_offset = 0, .format = gfx::format::rgb32_float },
              { .byte_offset = 12, .format = gfx::format::rgba32_float },
            } } },
                        .constants{},
                        .uniform_binding_groups{
          gfx::binding::group::make_from_buffer(mvp_buffer) } }
    )
  };

  while (true) {
    {
      auto scoped{ ctx.begin_scoped() }; // initiate context::begin()
      gfx::cmd::use(pipeline);
      gfx::cmd::push(mvp); // or mvp_buffer
      gfx::cmd::bind(triangle);
      gfx::cmd::draw({ .vertex_count = 3 });
    }

    ctx.swap_buffers();
  }
}
