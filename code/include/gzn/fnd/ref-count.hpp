#pragma once

#include <atomic>
#include <concepts>

#include "gzn/fnd/allocators.hpp"
#include "gzn/fnd/assert.hpp"
#include "gzn/fnd/func.hpp"

namespace gzn::fnd {

namespace util {

template<class T>
concept counter_type = requires(T value) {
  { value == 0 } -> std::same_as<bool>;
  { ++value };
  { --value };
};

template<class T>
concept ref_counter_type = requires(T value) {
  { value.ref_count() } -> std::same_as<typename T::number_type>;
  { value.acquire() } -> std::same_as<typename T::number_type>;
  { value.release() } -> std::same_as<typename T::number_type>;
};

} // namespace util

template<util::counter_type Number>
struct ref_counter {
  using number_type = Number;

  number_type counter{ 1 };

  [[nodiscard]]
  constexpr auto ref_count() const noexcept -> number_type {
    return counter;
  }

  constexpr auto acquire() noexcept -> number_type { return ++counter; }

  constexpr auto release() -> number_type {
    gzn_assertion(counter == 0, "Release of zero counter");
    return --counter;
  }
};

template<util::counter_type Number>
  requires std::atomic<Number>::is_always_lock_free
struct ref_counter_atomic {
  using number_type = Number;

  std::atomic<number_type> value{ 1 };

  [[nodiscard]]
  constexpr auto ref_count() const noexcept -> number_type {
    return value.load(std::memory_order_relaxed);
  }

  constexpr auto acquire() noexcept -> number_type {
    return value.fetch_add(1, std::memory_order_acq_rel) + 1;
  }

  constexpr auto release() -> number_type {
    gzn_assertion(value != 0, "Release of zero counter");
    return value.fetch_sub(1, std::memory_order_acq_rel) - 1;
  }
};

template<class T>
struct ref_counted_storage {
  using destructor_type = copyable_func<void(ref_counted_storage *)>;
  using value_type      = T;

  value_type              value;
  ref_counter_atomic<u32> counter{};
  destructor_type         destructor;

  template<class... Args>
    requires std::constructible_from<value_type, Args &&...>
  explicit ref_counted_storage(
    util::allocator_type auto &alloc,
    Args &&...args
  ) noexcept(std::is_nothrow_constructible_v<value_type>)
    : value{ std::forward<Args>(args)... }
    , destructor{ alloc, [&alloc](ref_counted_storage *me) {
                   util::destroy(alloc, me);
                 } } {}

  ~ref_counted_storage() = default;

  void acquire() noexcept { counter.acquire(); }

  void release() {
    if (counter.release() == 0) { destructor(this); }
  }
};

template<class T, class... Args>
  requires std::constructible_from<T, Args &&...>
[[nodiscard]]
constexpr auto make_ref_counted_storage(
  util::allocator_type auto *alloc,
  Args &&...args
) noexcept(std::is_nothrow_constructible_v<T>)
  -> std::add_pointer_t<ref_counted_storage<T>> {
  if (alloc == nullptr) { return nullptr; }

  using type = ref_counted_storage<T>;
  if (auto memory{ util::alloc<type>(*alloc) }; memory) {
    return new (memory) type{ *alloc, std::forward<Args>(args)... };
  }
  return nullptr;

  // return util::construct<ref_counted_storage<T>>(
  //   alloc, alloc, std::forward<Args>(args)...
  // );
}


} // namespace gzn::fnd
