#pragma once

#include <algorithm>
#include <array>

#include "gzn/fnd/assert.hpp"
#include "gzn/gfx/res/buffer.hpp"

namespace gzn::gfx {

template<class T>
class buffer_view {
public:
  using value_type      = T;
  using pointer         = T *;
  using const_pointer   = T const *;
  using reference       = T &;
  using const_reference = T const &;

  ~buffer_view()        = default;

  explicit constexpr buffer_view(fnd::handle<buffer> buf) noexcept
    : buf{ buf } {
    gzn_assertion(!buf.has_value(), "You must provide handle with value!");
    gzn_assertion(
      std::size(*buf.load()) < sizeof(value_type),
      "Buffer is smaller than size of value!"
    );
  }

  buffer_view(buffer_view const &) noexcept                     = default;
  buffer_view(buffer_view &&) noexcept                          = default;
  auto operator=(buffer_view const &) noexcept -> buffer_view & = default;
  auto operator=(buffer_view &&) noexcept -> buffer_view &      = default;

  auto operator=(value_type &&value) -> buffer_view & {
    if (auto buffer_data{ buf.load() }; buffer_data) {
      std::copy_n(std::data(*buffer_data), std::size(*buffer_data), &value);
    }
    return *this;
  }

private:
  fnd::handle<buffer> buf;
};


} // namespace gzn::gfx
