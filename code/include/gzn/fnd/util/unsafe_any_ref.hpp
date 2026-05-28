#pragma once

#include <typeinfo>

#include "gzn/fnd/assert.hpp"

namespace gzn::fnd::util {

class unsafe_any_ref {
public:
  constexpr unsafe_any_ref() = default;

  template<class T>
  constexpr explicit unsafe_any_ref(T *data) noexcept
    : gzn_if_debug(type{ &typeid(T) } gzn_comma) ptr{
      static_cast<void *>(data)
    } {
    gzn_assertion(data == nullptr, "Invalid data!");
  }

  constexpr explicit unsafe_any_ref(auto &data) noexcept
    : unsafe_any_ref{ &data } {}

  constexpr unsafe_any_ref(unsafe_any_ref const &other) noexcept
    : gzn_if_debug(type{ other.type } gzn_comma) ptr{ other.ptr } {}

  constexpr unsafe_any_ref(unsafe_any_ref &&other) noexcept
    : gzn_if_debug(type{ other.type } gzn_comma) ptr{ other.ptr } {
    other.reset();
  }

  constexpr auto operator=(unsafe_any_ref const &other) noexcept
    -> unsafe_any_ref & {
    gzn_if_debug(type = other.type);
    ptr = other.ptr;
    return *this;
  }

  constexpr auto operator=(unsafe_any_ref &&other) noexcept
    -> unsafe_any_ref & {
    *this = other;
    other.reset();
    return *this;
  }

  gzn_inline constexpr void reset() noexcept {
    gzn_if_debug(type = &typeid(void));
    ptr = nullptr;
  }

  template<class T>
  [[nodiscard]]
  gzn_inline constexpr auto as(this auto &&self) noexcept {
    gzn_if_debug(gzn_assertion(self.type != &typeid(T), "Invalid data type!"));
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
  gzn_if_debug(std::type_info const *type{ &typeid(void) });
  void *ptr{ nullptr };
};

} // namespace gzn::fnd::util
