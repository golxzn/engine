#pragma once

#include <type_traits>

#include "gzn/fnd/allocators.hpp"
#include "gzn/fnd/ref-count.hpp"

namespace gzn::fnd {

/*

template<class T, util::counter_type Counter, std::invocable<T *> Deleter>
struct ref_count_separate_storage {
  using pointer = std::add_pointer_t<T>;

  pointer value{};
  Counter ref_count{};
  Counter weak_ref_count{};

  [[nodiscard]]
  auto use_count() const noexcept {
    return ref_count;
  }
};

template<class T, util::counter_type Counter, std::invocable<T *> Deleter>
struct ref_count_storage {
  using pointer = std::add_pointer_t<T>;
  using storage = std::array<std::byte, sizeof(T)>;

  alignas(T) storage value;
  Counter ref_count{ 1 };
  Counter weak_ref_count{ 1 };
};

template<
  class T,
  util::allocator_type Allocator = base_allocator,
  std::invocable<T *>  Deleter   = util::default_deleter<T>,
  util::counter_type   Counter   = ref_counter_atomic>
class shared {
public:
  using value_type       = T;
  using pointer          = std::add_pointer_t<T>;
  using const_pointer    = std::add_pointer_t<std::add_const_t<T>>;
  using reference        = std::add_lvalue_reference_t<T>;
  using const_reference  = std::add_lvalue_reference_t<std::add_const_t<T>>;

  using separate_storage = ref_count_separate_storage<T, Counter, Deleter>;

  // using separate_storage = ref_count_separate_storage<T, Counter, Deleter>;

  explicit shared(pointer obj = nullptr) noexcept
    : m_object{ obj } {
    acquire();
  }

  shared(shared const &other) noexcept
    : shared{ other.m_object } {}

  shared(shared &&other) noexcept
    : m_object{ std::exchange(other.m_object, nullptr) } {}

  template<class... Args>
    requires std::constructible_from<value_type, Args &&...>
  explicit shared(std::pmr::memory_resource *res, Args &&...args)
    : shared{ make_in_place(res, std::forward<Args>(args)...) } {}

  ~shared() { release(); }

  auto operator=(shared const &other) noexcept -> shared & {
    if (this != &other) {
      release();
      m_object = other.m_object;
      acquire();
    }
    return *this;
  }

  auto operator=(shared &&other) noexcept -> shared & {
    if (this != &other) {
      release();
      m_object = std::exchange(other.m_object, nullptr);
    }
    return *this;
  }

  auto operator*() const -> const_reference { return *m_object; }

  auto operator->() const -> const_pointer { return m_object; }

  auto operator*() -> reference { return *m_object; }

  auto operator->() -> pointer { return m_object; }

  operator bool() const noexcept { return m_object != nullptr; }

private:
  pointer m_object;

  inline void acquire() const {
    if (m_object) { m_object->acquire(); }
  }

  inline void release() const {
    if (m_object) { m_object->release(); }
  }

  template<class... Args>
  static auto make_in_place(std::pmr::memory_resource *res, Args &&...args)
    -> pointer {
    auto place{ res->allocate(sizeof(value_type), alignof(value_type)) };
    return new (place) value_type{ std::forward<Args>(args)... };
  }
};

template<util::ref_counter_type T, class... Args>
  requires std::constructible_from<T, Args &&...>
[[nodiscard]] constexpr auto make_ref(Args &&...args) -> shared<T> {
  return shared<T>{ std::pmr::get_default_resource(),
                    std::forward<Args>(args)... };
}

template<util::ref_counter_type T, class... Args>
  requires std::constructible_from<T, Args &&...>
[[nodiscard]] constexpr auto make_ref(
  std::pmr::memory_resource &res,
  Args &&...args
) -> shared<T> {
  return shared<T>{ &res, std::forward<Args>(args)... };
}

*/

} // namespace gzn::fnd
