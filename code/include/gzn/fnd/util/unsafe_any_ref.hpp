#pragma once

#include <type_traits>
#include <typeindex>

#include "gzn/fnd/assert.hpp"

namespace gzn::fnd::util {

class unsafe_any_ref {
public:
  constexpr unsafe_any_ref() = default;

  constexpr explicit unsafe_any_ref(auto *data) noexcept
    : ptr{ static_cast<void *>(data) } {
    gzn_assertion(data != nullptr, "Invalid data!");
    gzn_if_debug(type = typeid(std::remove_pointer_t<decltype(data)>));
  }

  constexpr explicit unsafe_any_ref(auto &data) noexcept
    : unsafe_any_ref{ &data } {}

  constexpr unsafe_any_ref(unsafe_any_ref const &other)     = default;
  constexpr unsafe_any_ref(unsafe_any_ref &&other) noexcept = default;
  constexpr auto operator=(unsafe_any_ref const &other)
    -> unsafe_any_ref & = default;
  constexpr auto operator=(unsafe_any_ref &&other) noexcept
    -> unsafe_any_ref & = default;

  template<class T>
  [[nodiscard]]
  gzn_inline constexpr auto as(this auto &&self) noexcept {
    gzn_if_debug(gzn_assertion(self.type == typeid(T), "Invalid data type!"));
    return static_cast<T *>(self.ptr);
  }

  [[nodiscard]]
  constexpr auto operator==(std::nullptr_t) const noexcept -> bool {
    return ptr == nullptr;
  }

  [[nodiscard]]
  constexpr auto operator!=(std::nullptr_t) const noexcept -> bool {
    return ptr != nullptr;
  }

private:
  void *ptr{ nullptr };
  gzn_if_debug(std::type_index type{ typeid(void) });
};

} // namespace gzn::fnd::util
