#pragma once

#include <bit>

#include "gzn/fnd/allocators.hpp"
#include "gzn/fnd/assert.hpp"
#include "gzn/fnd/bits.hpp"
#include "gzn/fnd/definitions.hpp"

namespace gzn::fnd {

template<class T>
class pool;

#pragma pack(push, 1)

struct alignas(u8) gen_counter {
  static constexpr u32 alive_mask{ 1 };

  u8 value{};

  gzn_inline constexpr void next_gen() noexcept { value += 2; }

  [[nodiscard]]
  gzn_inline constexpr auto is_alive() const noexcept {
    return static_cast<bool>(value & alive_mask);
  }

  gzn_inline constexpr void set_alive() noexcept { value |= alive_mask; }

  gzn_inline constexpr void set_dead() noexcept { value &= ~alive_mask; }

  gzn_inline constexpr void set_dead_and_next_gen() noexcept {
    value &= ~alive_mask;
    value += 2;
  }
};

#pragma pack(pop)

template<class T>
struct handle {
  using value_type = T;

  pool<T>    *pool{ nullptr };
  usize       location{ 0 };
  gen_counter generation{ 0 };

  [[nodiscard]]
  constexpr auto is_actual() const noexcept -> bool;

  [[nodiscard]]
  constexpr auto value(this auto &&self) noexcept -> value_type *;
};

template<class T>
class non_owning_pool {
  static constexpr bool T_nothrow_move{
    std::is_nothrow_move_constructible_v<T>
  };

public:
  using value_type            = T;
  using handle_type           = handle<T>;
  using destructor            = void (*)(void *, void *, usize);

  constexpr non_owning_pool() = default;

  explicit non_owning_pool(byte *storage, usize const elements_count) noexcept
    : count{ elements_count }
    , storage{ storage }
    , generations{ reinterpret_cast<gen_counter *>(storage) }
    , data{ reinterpret_cast<T *>(storage) + count * sizeof(gen_counter) } {
    gzn_assertion(count != 0, "pool should not be empty");
    std::fill_n(generations, count * sizeof(gen_counter), gen_counter{});
  }

  non_owning_pool(non_owning_pool const &)                         = default;
  non_owning_pool(non_owning_pool &&) noexcept                     = default;
  auto operator=(non_owning_pool const &) -> non_owning_pool &     = default;
  auto operator=(non_owning_pool &&) noexcept -> non_owning_pool & = default;

  [[nodiscard]]
  constexpr auto is_valid() const noexcept {
    return storage != nullptr;
  }

  [[nodiscard]]
  constexpr auto elements_count() const noexcept {
    return count;
  }

  [[nodiscard]]
  constexpr auto bytes_count() const noexcept {
    return get_size_for(count);
  }

  [[nodiscard]]
  constexpr auto at(this auto &&self, usize const index) noexcept {
    gzn_assertion(index > self.count, "Index is out of range");
    return handle_type{ .pool       = &self,
                        .location   = index,
                        .generation = self.generations[index] };
  }

  [[nodiscard]]
  constexpr auto value_at(this auto &&self, usize const index) noexcept {
    gzn_assertion(index > self.count, "Index is out of range");
    return self.generations[index];
  }

  [[nodiscard]]
  constexpr auto generation_at(this auto &&self, usize const index) noexcept {
    gzn_assertion(index > self.count, "Index is out of range");
    return self.generations[index];
  }

  constexpr auto try_insert_unsafe(
    value_type  value,
    usize const index
  ) noexcept(T_nothrow_move) -> bool {
    gzn_assertion(index > count, "Index is out of range");
    if (auto gen{ generations + index }; gen->is_alive()) {
      std::construct_at(data + index, std::move(value));
      gen->set_alive();
      return true;
    }
    return false;
  }

  gzn_inline constexpr auto try_append(value_type value) noexcept(
    T_nothrow_move
  ) -> bool {
    auto const status{ try_insert_unsafe(std::move(value), top) };
    top += static_cast<usize>(status);
    return status;
  }

  constexpr auto remove_at(usize const index) -> bool {
    gzn_assertion(index > count, "Index is out of range");
    if (auto gen{ generations + index }; gen->is_alive()) {
      if constexpr (!std::is_trivially_destructible_v<value_type>) {
        std::destroy_at(data + index);
      }
      gen->set_dead_and_next_gen();
      return true;
    }
    return false;
  }

  [[nodiscard]]
  constexpr static auto get_size_for(usize const count) noexcept {
    return count * sizeof(T) + count * sizeof(gen_counter);
  }


private:
  usize        count{};
  usize        top{};
  byte        *storage{ nullptr };
  gen_counter *generations{ nullptr };
  value_type  *data{ nullptr };
};

template<class T>
class pool {
  static constexpr bool T_nothrow_move{
    std::is_nothrow_move_constructible_v<T>
  };

public:
  using value_type  = T;
  using handle_type = handle<T>;
  using destructor  = void (*)(void *, void *, usize);

  constexpr pool()  = default;

  explicit pool(void *storage, usize const elements_count) noexcept
    : count{ elements_count }
    , storage{ storage }
    , generations{ reinterpret_cast<gen_counter *>(storage) }
    , data{ reinterpret_cast<T *>(storage) + count * sizeof(gen_counter) } {
    gzn_assertion(count == 0, "pool should not be empty");
    gzn_assertion(
      !std::has_single_bit(elements_count),
      "Pool size must be power of 2. In Release mode will be rounded to "
      "closest lower power of 2"
    );
    std::fill_n(generations, count * sizeof(gen_counter), gen_counter{});
  }

  template<fnd::util::allocator_type Alloc>
  explicit pool(Alloc &alloc, usize const count)
    : pool{ alloc.allocate(get_size_for(count)), count }
    , alloc{ &alloc }
    , destruct_storage{ make_destructor<Alloc, gen_counter>() } {}

  ~pool() { destruct_storage(alloc, storage, get_size_for(count)); }

  pool(pool const &)                         = delete;
  pool(pool &&) noexcept                     = delete;
  auto operator=(pool const &) -> pool &     = delete;
  auto operator=(pool &&) noexcept -> pool & = delete;

  [[nodiscard]]
  constexpr auto is_valid() const noexcept {
    return storage != nullptr;
  }

  [[nodiscard]]
  constexpr auto elements_count() const noexcept {
    return count;
  }

  [[nodiscard]]
  constexpr auto bytes_count() const noexcept {
    return get_size_for(count);
  }

  [[nodiscard]]
  constexpr auto at(this auto &&self, usize const index) noexcept {
    gzn_assertion(index > self.count, "Index is out of range");
    return handle_type{ .pool       = &self,
                        .location   = index,
                        .generation = self.generations[index] };
  }

  [[nodiscard]]
  constexpr auto value_at(this auto &&self, usize const index) noexcept {
    gzn_assertion(index > self.count, "Index is out of range");
    return self.generations[index];
  }

  [[nodiscard]]
  constexpr auto generation_at(this auto &&self, usize const index) noexcept {
    gzn_assertion(index > self.count, "Index is out of range");
    return self.generations[index];
  }

  constexpr auto try_insert_unsafe(
    value_type  value,
    usize const index
  ) noexcept(T_nothrow_move) -> bool {
    gzn_assertion(index > count, "Index is out of range");
    if (auto gen{ generations + index }; gen->is_alive()) {
      std::construct_at(data + index, std::move(value));
      gen->set_alive();
      return true;
    }
    return false;
  }

  gzn_inline constexpr auto try_append(value_type value) noexcept(
    T_nothrow_move
  ) -> bool {
    auto const status{ try_insert_unsafe(std::move(value), top) };
    top += static_cast<usize>(status);
    return status;
  }

  constexpr auto remove_at(usize const index) -> bool {
    gzn_assertion(index > count, "Index is out of range");
    if (auto gen{ generations + index }; gen->is_alive()) {
      if constexpr (!std::is_trivially_destructible_v<value_type>) {
        std::destroy_at(data + index);
      }
      gen->set_dead_and_next_gen();
      return true;
    }
    return false;
  }

  [[nodiscard]]
  constexpr static auto get_size_for(usize const count) noexcept {
    return count * sizeof(T) + count * sizeof(gen_counter);
  }

private:
  template<fnd::util::allocator_type Alloc, class U>
  gzn_inline static auto make_destructor() noexcept -> destructor {
    static auto fn{ [](void *a, void *d, usize sz) {
      fnd::util::dealloc(&static_cast<Alloc *>(a), static_cast<U *>(d), sz);
    } };
    return fn;
  }

  usize        count{};
  usize        top{};
  void        *storage{ nullptr };
  gen_counter *generations{ nullptr };
  value_type  *data{ nullptr };
  void        *alloc{ nullptr };
  destructor   destruct_storage{ [](auto, auto, auto) {} };
};

template<class T>
constexpr auto handle<T>::is_actual() const noexcept -> bool {
  return pool != nullptr && generation == pool->generation_at(location);
}

template<class T>
constexpr auto handle<T>::value(this auto &&self) noexcept -> value_type * {
  return self.is_actual() ? self.pool->value_at(self.location) : nullptr;
}

} // namespace gzn::fnd
