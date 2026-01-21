#pragma once

#include <concepts>

#include "gzn/fnd/types.hpp"

namespace gzn::app {

template<class T>
  requires std::integral<T> || std::floating_point<T>
struct dimensions {
  union {
    T width{};
    T w;
  };

  union {
    T height{};
    T h;
  };

  union {
    T depth{};
    T d;
  };
};

using s32dim = dimensions<s32>;
using u32dim = dimensions<u32>;
using f32dim = dimensions<f32>;

} // namespace gzn::app
