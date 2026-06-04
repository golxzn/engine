#pragma once

#include <expected>

namespace gzn::fnd {

template<class T, class E>
using expected = std::expected<T, E>;

template<class E>
using unexpected = std::unexpected<E>;

} // namespace gzn::fnd
