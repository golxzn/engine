#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "gzn/fnd/containers/pool.hpp"
#include "gzn/fnd/containers/span.hpp"
#include "gzn/fnd/name.hpp"
#include "gzn/fnd/optional.hpp"
#include "gzn/fnd/raw-data.hpp"
#include "gzn/gfx/enums.hpp"
#include "gzn/gfx/res/binding-group.hpp"
#include "gzn/gfx/res/buffer.hpp"

namespace gzn::gfx {

/**
 * Resources will be managed by this static class, but the resources itself are
 * in the context. Although the common data should be accessible from the high
 * API agnostic side, the API specific things must be hidden in the context.
 **/

class context;
class buffer;
class shader;
class pipeline;
class sampler;
class texture;
class texture_view;
class render_pass;
class framebuffer;
class binding_group;

inline constexpr auto SIZE_AS_IN_INIT_DATA{
  (std::numeric_limits<usize>::max)()
};

struct res_buffer {
  fnd::s8name_view   name;
  buffer_usage       usage{ buffer_usage::vertex };
  buffer_memory_type memory{ buffer_memory_type::gpu };
  fnd::raw_data      init_data{};
  data_type          type{ data_type::as_in_init_data };
  usize              size{ SIZE_AS_IN_INIT_DATA };
};

struct res_vertex_buffer {
  res_buffer       description;
  attribute_layout layout;
};

struct res_shader {
  shader_type   type;
  fnd::raw_data source_code{};
  fnd::raw_data bytecode{};
};

struct res_pipeline {
  fnd::s8name_view                            name;
  fnd::handle<render_pass>                    pass;
  fnd::span<res_shader const>                 shaders{};
  attribute_layout                            layout;
  fnd::span<fnd::handle<binding_group> const> binding_groups{};
};

struct res_sampler {
  struct filters_type {
    filter_mode min{ filter_mode::linear };
    filter_mode mag{ filter_mode::linear };
    filter_mode mipmap{ filter_mode::linear };
  };

  struct address_type {
    address_mode u{ address_mode::repeat };
    address_mode v{ address_mode::repeat };
    address_mode w{ address_mode::repeat };
  };

  struct mip_lod_type {
    f32 bias{ 0.0f };
    f32 min{ 0.0f };
    f32 max{ 1000.0f };
  };

  fnd::s8name_view     name;
  filters_type         filters{};
  address_type         address{};
  mip_lod_type         mip_lod{};
  f32                  max_anisotropy{ 1.0f };
  fnd::opt<compare_op> compare{};
};

struct res_texture {
  fnd::s8name_view name;
  texture_type     type{ texture_type::texture_2d };
  format_type      format{ format_type::rgba_u8 };
  glm::u32vec3     size; // width, height, depth
  u32              mip_levels{ 1u };
  u32              array_layers{ 1u };
  texture_usage    usage;
  fnd::raw_data    init_data{};
};

struct res_texture_view {
  fnd::handle<texture> texture;
  u32                  base_mip{ 0u };
  u32                  mip_count{ 1u };
  u32                  base_layer{ 0u };
  u32                  layer_count{ 1u };
};

struct attachment_info {
  format_type format;

  attachment_load_op  load_op{ attachment_load_op::dont_care };
  attachment_store_op store_op{ attachment_store_op::dont_care };

  attachment_load_op  stencil_load_op{ attachment_load_op::dont_care };
  attachment_store_op stencil_store_op{ attachment_store_op::dont_care };
};

struct res_render_pass {
  fnd::s8name_view                 name;
  fnd::span<attachment_info const> color_attachments;
  fnd::opt<attachment_info>        depth_attachment{};
};

struct res_framebuffer {
  fnd::s8name_view                           name;
  glm::vec2                                  size;
  fnd::handle<render_pass>                   pass;
  fnd::span<fnd::handle<texture_view> const> color_attachments;
  fnd::handle<texture_view>                  depth_attachments;
};

struct res_binding_group {
  fnd::s8name_view                     name;
  fnd::span<layout_binding_info const> layout;
  fnd::span<fnd::handle<texture_view>> textures;
  fnd::span<fnd::handle<buffer>>       buffers;
};

struct res {
  res()  = delete;
  ~res() = delete;

  [[nodiscard]] static auto make(context &ctx, res_buffer const &info)
    -> fnd::handle<buffer>;

  [[nodiscard]] static auto make(context &ctx, res_vertex_buffer const &info)
    -> fnd::handle<buffer>; /// @todo should it return vertex_buffer?

  static void free(context &ctx, fnd::handle<buffer> &info);

  [[nodiscard]] static auto make(context &ctx, res_shader const &info)
    -> fnd::handle<shader>;

  static void free(context &ctx, fnd::handle<shader> &info);

  [[nodiscard]] static auto make(context &ctx, res_pipeline const &info)
    -> fnd::handle<pipeline>;

  static void free(context &ctx, fnd::handle<pipeline> &info);

  [[nodiscard]] static auto make(context &ctx, res_sampler const &info)
    -> fnd::handle<sampler>;

  static void free(context &ctx, fnd::handle<sampler> &info);

  [[nodiscard]] static auto make(context &ctx, res_texture const &info)
    -> fnd::handle<texture>;

  static void free(context &ctx, fnd::handle<texture> &info);

  [[nodiscard]] static auto make(context &ctx, res_texture_view const &info)
    -> fnd::handle<texture_view>;

  static void free(context &ctx, fnd::handle<texture_view> &info);

  [[nodiscard]] static auto make(context &ctx, res_render_pass const &info)
    -> fnd::handle<render_pass>;

  static void free(context &ctx, fnd::handle<render_pass> &info);

  [[nodiscard]] static auto make(context &ctx, res_framebuffer const &info)
    -> fnd::handle<framebuffer>;

  static void free(context &ctx, fnd::handle<framebuffer> &info);

  [[nodiscard]] static auto make(context &ctx, res_binding_group const &info)
    -> fnd::handle<binding_group>;

  static void free(context &ctx, fnd::handle<binding_group> &info);
};

}; // namespace gzn::gfx
