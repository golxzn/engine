#pragma once

#include <utility>

#include "gzn/fnd/types.hpp"

namespace gzn::gfx {

enum class topology : u8 {
  points,
  lines,
  line_strip,
  triangles,
  triangle_strip,
};

enum class cull_mode : u8 {
  none,
  front,
  back,
};

enum class front_face : u8 {
  clockwise,
  counter_clockwise,
};

enum class blend_factor : u8 {
  zero,
  one,
  src_color,
  one_minus_src_color,
  dst_color,
  one_minus_dst_color,
  src_alpha,
  one_minus_src_alpha,
  dst_alpha,
  one_minus_dst_alpha,
};

enum class blend_op : u8 {
  add,
  subtract,
  reverse_subtract,
  min,
  max,
};

enum class attachment_load_op : u8 {
  load,
  clear,
  dont_care,
};

enum class attachment_store_op : u8 {
  store,
  dont_care,
};

// clang-format off
enum class data_type : u8 {
  as_in_init_data,
  f16, f32, f64,
  u8, u16, u32, u64,
  s8, s16, s32, s64,
};
// clang-format on

enum class buffer_memory_type : u8 {
  gpu,
  cpu,
  cpu_to_gpu,
  gpu_to_cpu,
};

enum class usage_rate : u8 {
  per_vertex,
  per_instance,
};

enum class descriptor_type : u8 {
  sampler,
  uniform_texel_buffer,
  storage_texel_buffer,
  uniform_buffer,
  storage_buffer,
  uniform_buffer_dynamic,
  storage_buffer_dynamic,
};

struct alignas(u32) shader_type {
  static constexpr u32 vertex{ 1 << 0 };
  static constexpr u32 fragment{ 1 << 1 };
  static constexpr u32 geometry{ 1 << 2 };
  static constexpr u32 tessellation_control{ 1 << 3 };
  static constexpr u32 tessellation_evaluation{ 1 << 4 };
  static constexpr u32 compute{ 1 << 7 };

  u32 value;
};

struct alignas(u32) buffer_usage {
  static constexpr u32 vertex{ 1 << 0 };
  static constexpr u32 index{ 1 << 1 };
  static constexpr u32 uniform{ 1 << 2 }; // constant buffer
  static constexpr u32 storage{ 1 << 3 }; // UAV / SSBO
  static constexpr u32 indirect{ 1 << 4 };
  static constexpr u32 texel{ 1 << 5 };
  static constexpr u32 transfer_src{ 1 << 6 };
  static constexpr u32 transfer_dst{ 1 << 7 };

  u32 value;
};

#pragma region TEXTURE

enum class texture_type : u8 {
  texture_1d,
  texture_2d,
  texture_3d,
  texture_cube_map,
  texture_2d_array,
  // maybe 3d_array and so on.
};

struct alignas(u32) texture_usage {
  static constexpr u32 color_attachment{ 1 << 0 };
  static constexpr u32 depth_attachment{ 1 << 1 };
  static constexpr u32 sampled{ 1 << 2 };
  static constexpr u32 storage{ buffer_usage::storage };
  static constexpr u32 transfer_src{ buffer_usage::transfer_src };
  static constexpr u32 transfer_dst{ buffer_usage::transfer_dst };

  u32 value;
};

#pragma endregion TEXTURE

#pragma region SAMPLER

enum class compare_op : u8 {
  never,
  less,
  equal,
  less_equal,
  greater,
  not_equal,
  greater_equal,
  always,
};

enum class filter_mode : u8 {
  none,
  nearest,
  linear,
};
using mip_filter = filter_mode;

enum class address_mode : u8 {
  repeat,
  mirror_repeat,
  clamp_edge,
  clamp_border,
};

#pragma endregion SAMPLER

/// @todo fill this shit
enum class format_type : u16 {
  undefined,

  rgb_u8,
  rgb_s8,
  rgba_u8,
  rgba_s8,

  rgb_f32,
  rgba_f32,
};

} // namespace gzn::gfx
