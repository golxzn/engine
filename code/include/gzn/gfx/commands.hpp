#pragma once

#include <string_view>
#include <variant>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "gzn/fnd/containers/pool.hpp"
#include "gzn/fnd/containers/span.hpp"
#include "gzn/gfx/enums.hpp"
#include "gzn/gfx/res/buffer.hpp"
#include "gzn/gfx/res/framebuffer.hpp"
#include "gzn/gfx/res/pipeline.hpp"
#include "gzn/gfx/res/render-pass.hpp"
#include "gzn/gfx/res/texture.hpp"

namespace gzn::gfx {

class context;

struct cmd_init {
  usize buffers_count{ 1 };
};

struct cmd_get {
  usize index;
};

struct cmd_clear_color {
  glm::vec4 rgba{ 0.0f, 0.0f, 0.0f, 1.0f };
};

struct cmd_depth_stencil {
  f32 depth;
  u32 stencil;
};

using clear_value = std::variant<cmd_clear_color, cmd_depth_stencil>;

struct cmd_begin_render_pass {
  fnd::handle<render_pass>     pass;
  fnd::handle<framebuffer>     framebuffer;
  glm::vec4                    render_area;
  fnd::span<clear_value const> clears{};
};

struct cmd_end_render_pass {
  fnd::handle<render_pass> pass;
};

struct cmd_set_viewport {
  glm::vec2 pos{};
  glm::vec2 size{};
  glm::vec2 depth{}; // min/max
};

struct cmd_set_scissor {
  glm::vec2 pos{};
  glm::vec2 size{};
};

struct cmd_bind_render_pass {
  fnd::handle<render_pass> pass;
};

struct cmd_bind_pipeline {
  fnd::handle<pipeline> pipeline;
};

struct cmd_bind_buffer {
  fnd::handle<buffer> buffer; // will provide buffer_usage & data_type
  usize               offset{};
  usize               slot{};
  shader_type         stage{ shader_type::vertex | shader_type::fragment };
};

struct cmd_bind_texture {
  fnd::handle<texture> texture;
  fnd::handle<sampler> sampler;
  shader_type          stage;
  u32                  slot;
};

struct cmd_draw {
  u32 vertex_count{};
  u32 first_vertex{};
};

struct cmd_draw_indexed {
  u32 index_count{};
  u32 first_index{};
  u32 vertex_offset{};
};

struct cmd_draw_instanced : cmd_draw {
  u32 instance_count{};
  u32 first_instance{};
};

struct cmd_draw_indexed_instanced
  : cmd_draw_indexed
  , cmd_draw_instanced {};

struct cmd_dispatch {
  glm::u32vec3 group;
};

struct cmd_resource_barrier {
  // fnd::span<barrier_description> barriers;
};

struct cmd_copy_buffer {
  fnd::handle<buffer> src;
  fnd::handle<buffer> dst;
  usize               src_offset;
  usize               dst_offset;
  usize               size;
};

struct subresource_range {
  /// @todo
};

struct texture_region {
  /// @todo
};

struct cmd_copy_texture {
  fnd::handle<texture> src;
  fnd::handle<texture> dst;
  subresource_range    range;
};

struct cmd_copy_buffer_to_texture {
  fnd::handle<buffer>  src;
  fnd::handle<texture> dst;
  texture_region       region;
};

struct cmd_blit_texture {
  fnd::handle<texture> src;
  fnd::handle<texture> dst;
  filter_mode          filter;
};

struct cmd_gen_mipmaps {
  fnd::handle<texture> target;
};

struct cmd_set_blend {
  glm::vec4 value;
};

struct cmd_set_depth_bias {
  f32 constant_factor{};
  f32 slope_factor{};
  f32 clamp{};
};

struct cmd_set_line_width {
  f32 width{ 1.0f };
};

struct cmd_begin_debug_group {
  std::string_view label{};
};

struct cmd_end_debug_group {};

class command_list;

struct cmd final {
  cmd()  = delete;
  ~cmd() = delete;

  static void init(context &ctx, cmd_init const &info);

  [[nodiscard]]
  static auto get(cmd_get const &info) -> command_list;

  static void begin_frame(context &ctx);
  static void end_frame(context &ctx);

  static void submit(command_list &ref);

  static void add(command_list &list, cmd_clear_color const &info);
  static void add(command_list &list, cmd_depth_stencil const &info);
  static void add(
    command_list                       &list,
    fnd::span<clear_value const> const &info
  );

  static void add(command_list &list, cmd_set_viewport const &info);
  static void add(command_list &list, cmd_set_scissor const &info);
  static void add(command_list &list, cmd_set_blend const &info);
  static void add(command_list &list, cmd_set_depth_bias const &info);
  static void add(command_list &list, cmd_set_line_width const &info);

  static void add(command_list &list, cmd_bind_render_pass const &info);
  static void add(command_list &list, cmd_bind_pipeline const &info);
  static void add(command_list &list, cmd_bind_buffer const &info);
  static void add(command_list &list, cmd_bind_texture const &info);

  static void add(command_list &list, cmd_draw const &info);
  static void add(command_list &list, cmd_draw_indexed const &info);
  static void add(command_list &list, cmd_draw_instanced const &info);
  static void add(command_list &list, cmd_draw_indexed_instanced const &info);

  static void add(command_list &list, cmd_dispatch const &info);

  static void add(command_list &list, cmd_resource_barrier const &info);

  static void add(command_list &list, cmd_copy_buffer const &info);
  static void add(command_list &list, cmd_copy_texture const &info);
  static void add(command_list &list, cmd_copy_buffer_to_texture const &info);

  static void add(command_list &list, cmd_blit_texture const &info);

  static void add(command_list &list, cmd_gen_mipmaps const &info);

  static void add(command_list &list, cmd_begin_debug_group const &info);
  static void add(command_list &list, cmd_end_debug_group const &info);
};

class command_list {
  friend struct cmd;

public:
  gzn_inline void operator+=(auto const &info) { cmd::add(*this, info); }

private:
  fnd::store_key handle{ fnd::null_key };
};


} // namespace gzn::gfx
