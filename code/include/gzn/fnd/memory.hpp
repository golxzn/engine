#pragma once

#include "gzn/fnd/bits.hpp"

namespace gzn::fnd::mem {

usize inline constexpr DEFAULT_PAGE_SIZE{ 4096u };

[[nodiscard]]
auto get_page_size() -> usize;

usize static const PAGE_SIZE{ get_page_size() };

} // namespace gzn::fnd::mem
