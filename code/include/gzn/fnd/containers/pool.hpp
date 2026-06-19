#pragma once

#include "gzn/fnd/allocators.hpp"
#include "gzn/fnd/assert.hpp"
#include "gzn/fnd/containers/span.hpp"
#include "gzn/fnd/definitions.hpp"
#include "gzn/fnd/owner.hpp"
#include "gzn/fnd/utility.hpp"

namespace gzn::fnd {

using handle_index_type = u16;
inline constexpr auto MAX_POOL_SIZE{
  (std::numeric_limits<handle_index_type>::max)()
};

struct alignas(sizeof(handle_index_type) * 2) store_key {
  handle_index_type location{};
  handle_index_type generation{};
};

[[nodiscard]]
constexpr auto operator==(store_key const lhv, store_key const rhv) noexcept {
  return lhv.location == rhv.location && lhv.generation == rhv.generation;
}

inline constexpr store_key null_key{
  .location   = 0u,
  .generation = 0u,
};

template<class T>
class pool;

template<class T>
struct handle {
  using pool_type  = pool<T>;
  using value_type = T;

  ref<pool_type> pool{};
  store_key      key{ null_key };

  [[nodiscard]]
  constexpr auto has_value() const noexcept -> bool;

  [[nodiscard]]
  constexpr auto value(this auto &&self) noexcept -> value_type *;
};

template<class T>
class pool {
  static constexpr bool T_nothrow_move{
    std::is_nothrow_move_constructible_v<T>
  };

public:
  using size_type       = handle_index_type;
  using value_type      = T;
  using handle_type     = handle<T>;

  constexpr pool() = default;

  template<class Type>
  static constexpr auto bytes(size_type count) noexcept -> size_type {
    return sizeof(Type) * count;
  }

  explicit pool(fnd::span<byte> storage, size_type count) noexcept
    : storage{ storage }
    , values{ util::data(
        storage.subrange_morph<value_type>(0, bytes<value_type>(count))
      ) }
    , generations{ util::data(storage.subrange_morph<handle_index_type>(
        bytes<value_type>(count),
        bytes<handle_index_type>(count)
      )) }
    , occupations{ util::data(storage.subrange_morph<bool>(
        bytes<value_type>(count) + bytes<handle_index_type>(count),
        bytes<bool>(count)
      )) } {
    std::memset(occupations, false, bytes<bool>(count));
  }

  pool(pool const &)                         = default;
  pool(pool &&) noexcept                     = default;
  auto operator=(pool const &) -> pool &     = default;
  auto operator=(pool &&) noexcept -> pool & = default;

  [[nodiscard]]
  constexpr auto is_valid() const noexcept {
    return std::empty(storage);
  }

  [[nodiscard]]
  constexpr auto elements_count() const noexcept {
    return util::size(storage);
  }

  [[nodiscard]]
  constexpr auto get(this auto &&self, store_key const key) noexcept {
    gzn_assertion(
      key.location >= self.elements_count(), "Key location is out of range!"
    );
    if (key.generation == self.generations[key.location]) {
      return self.values[key.location];
    }
    return nullptr;
  }

  constexpr void remove(store_key const key) noexcept {
    if (key == null_key) { return; }

    gzn_assertion(
      key.location >= elements_count(), "Key location is out of range!"
    );
    if (key.generation != generations[key.location]) { return; }

    if constexpr (!std::is_trivially_destructible_v<value_type>) {
      std::destroy_at(values + key.location);
    }
    ++generations[key.location];
    occupations[key.location] = false;
  }

  [[nodiscard]]
  constexpr auto add(value_type &&value) noexcept(T_nothrow_move)
    -> store_key {
    auto const count{ elements_count() };
    for (size_type idx{ 1 }; idx < count; ++idx) {
      if (occupations[idx]) { continue; }

      values[idx] = std::move(value);
      ++generations[idx];
      occupations[idx] = true;
      return store_key{ .location = idx, .generation = 1u };
    }
    return null_key;
  }

  [[nodiscard]]
  constexpr static auto get_size_for(usize const count) noexcept {
    return count * sizeof(value_type) + count * sizeof(handle_index_type) +
           count * sizeof(bool);
  }

protected:
  fnd::span<byte>    storage{};
  value_type        *values{};
  handle_index_type *generations{};
  bool              *occupations{};
};


template<class T>
class owning_pool : public pool<T> {
  static constexpr bool T_nothrow_move{
    std::is_nothrow_move_constructible_v<T>
  };

public:
  using super             = pool<T>;
  using size_type         = typename super::size_type;
  using value_type        = typename super::value_type;
  using handle_type       = typename super::handle_type;
  using destructor        = void (*)(void *, void *, usize);

  constexpr owning_pool() = default;

  template<fnd::util::allocator_type Alloc>
  explicit owning_pool(Alloc &alloc, usize const count)
    : super{ alloc.allocate(super::get_size_for(count)), count }
    , alloc{ &alloc }
    , destruct_storage{ make_destructor<Alloc, byte>() } {}

  ~owning_pool() {
    if constexpr (!std::is_trivially_destructible_v<value_type>) {
      auto const count{ super::elements_count() };
      for (size_type idx{ 1 }; idx < count; ++idx) {
        if (super::occupations[idx]) { std::destroy_at(super::values + idx); }
      }
    }
    destruct_storage(
      alloc, util::data(super::storage), util::size(super::storage)
    );
  }

  owning_pool(owning_pool const &)                         = delete;
  owning_pool(owning_pool &&) noexcept                     = delete;
  auto operator=(owning_pool const &) -> owning_pool &     = delete;
  auto operator=(owning_pool &&) noexcept -> owning_pool & = delete;

private:
  template<fnd::util::allocator_type Alloc, class U>
  gzn_inline static auto make_destructor() noexcept -> destructor {
    static auto fn{ [](void *a, void *d, usize sz) {
      fnd::util::dealloc(&static_cast<Alloc *>(a), static_cast<U *>(d), sz);
    } };
    return fn;
  }

  void      *alloc{ nullptr };
  destructor destruct_storage{ [](auto, auto, auto) {} };
};

template<class T>
constexpr auto handle<T>::has_value() const noexcept -> bool {

  return pool.is_alive() && pool->contains(key);
}

template<class T>
constexpr auto handle<T>::value(this auto &&self) noexcept -> value_type * {
  return self.has_value() ? self.pool->get(self.key) : nullptr;
}

} // namespace gzn::fnd
