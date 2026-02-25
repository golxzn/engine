#pragma once

#include "gzn/fnd/clamped.hpp"
#include "gzn/gfx/defaults.hpp"

namespace gzn::gfx {

struct render_capacities {
  fnd::clamped<usize, PIPELINES_MIN_COUNT> pipelines_count{ PIPELINES_COUNT };
  fnd::clamped<usize, BUFFERS_MIN_COUNT>   buffers_count{ BUFFERS_COUNT };
  fnd::clamped<usize, SAMPLERS_MIN_COUNT>  samples_count{ SAMPLERS_COUNT };

  template<class Sizes>
  gzn_inline constexpr auto total_size() const noexcept {
    return pipelines_count * Sizes::pipeline_bytes_count +
           buffers_count * Sizes::buffer_bytes_count;
  }
};

} // namespace gzn::gfx
