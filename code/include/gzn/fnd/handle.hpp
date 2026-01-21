#pragma once

#include <array> // for std::in_place_t
#include <concepts>
#include <limits>

#include "gzn/fnd/assert.hpp"

namespace gzn::fnd {

struct store_key {
  u32 index{};
  u32 generation{};
};

constexpr auto operator==(store_key const lhv, store_key const rhv) noexcept
  -> bool {
  return lhv.index == rhv.index && lhv.generation == rhv.generation;
}

constexpr store_key null_key{
  .index      = (std::numeric_limits<u32>::max)(),
  .generation = (std::numeric_limits<u32>::max)(),
};

template<class Storage, class T>
concept storage_class = requires {
  { T::destroy(store_key{}) };
  { T::get(store_key{}) } -> std::convertible_to<std::add_pointer_t<T>>;
};

struct base_storage {
  template<class T, class... Args>
  static auto construct(std::in_place_t, Args &&...args) -> store_key {
    return null_key;
  }

  static void destory(store_key const key) {}

  static auto get(store_key const key) -> void * { return nullptr; }
};

template<class T>
struct handle_traits {
  using storage_class = base_storage;
};

template<
  class T,
  storage_class<T> Storage = typename handle_traits<T>::storage_class>
class handle {
public:
  using pointer       = std::add_pointer_t<T>;
  using const_pointer = std::add_pointer_t<std::add_const_t<T>>;

  handle()            = default;

  handle(T &&value) noexcept
    : m_key{ Storage::construct(std::move(value)) } {}

  template<class... Args>
    requires std::constructible_from<T, Args &&...>
  handle(Args &&...args) noexcept
    : m_key{ Storage::template construct<T>(
        std::in_place,
        std::forward<Args>(args)...
      ) } {}

  [[nodiscard]]
  constexpr auto load() const -> const_pointer {
    return static_cast<const_pointer>(Storage::get(m_key));
  }

  [[nodiscard]]
  constexpr auto load() -> pointer {
    return static_cast<pointer>(Storage::get(m_key));
  }

  [[nodiscard]]
  constexpr auto has_value() const noexcept -> bool {
    return m_key != null_key && load() != nullptr;
  }

  [[nodiscard]]
  constexpr auto key() const noexcept -> store_key {
    return m_key;
  }

private:
  store_key m_key{ null_key };
};

template<class T, storage_class<T> S>
constexpr auto operator==(
  handle<T, S> const &lhv,
  handle<T, S> const &rhv
) noexcept -> bool {
  return lhv.key() == rhv.key();
}

template<class T, storage_class<T> S>
constexpr auto operator==(
  handle<T, S> const &lhv,
  store_key const     rhv
) noexcept -> bool {
  return lhv.key() == rhv;
}

template<class T, storage_class<T> S>
constexpr auto operator==(
  store_key const     lhv,
  handle<T, S> const &rhv
) noexcept -> bool {
  return lhv == rhv.key();
}


} // namespace gzn::fnd
