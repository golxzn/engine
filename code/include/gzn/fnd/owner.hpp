#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <utility>

#include "gzn/fnd/allocators.hpp"
#include "gzn/fnd/func.hpp"
#include "gzn/fnd/ref-count.hpp"

namespace gzn::fnd {

/* [IDEA]
 * Holding the shared flag which is located on ??? and the owner has rights to
 * set it, while references could only read this flag.
 */

using owner_alive_state = ref_counted_storage<std::atomic_bool>;

template<class T>
class owner_base;

template<class T>
class ref;

template<class T>
struct value_with_status {
  using value_type = T;

  owner_alive_state *state{ nullptr };
  value_type        *value{ nullptr };

  [[nodiscard]]
  constexpr auto is_alive() const noexcept -> bool {
    return state && state->value.load(std::memory_order_relaxed);
  }

  [[nodiscard]]
  constexpr auto acquire_copy() const noexcept -> value_with_status {
    if (state != nullptr) { state->acquire(); }
    return *this;
  }
};

template<class T>
class owner_base {
public:
  using value_type      = T;
  using reference       = std::add_lvalue_reference_t<T>;
  using const_reference = std::add_lvalue_reference_t<std::add_const_t<T>>;
  using pointer         = std::add_pointer_t<T>;
  using const_pointer   = std::add_pointer_t<std::add_const_t<T>>;

  constexpr owner_base(owner_base const &) = delete;

  constexpr owner_base(owner_base &&other) noexcept
    : m_data{
      .state = std::exchange(other.m_data.state, nullptr),
      .value = std::exchange(other.m_data.value, nullptr),
    } {}

  constexpr auto operator=(owner_base const &) -> owner_base & = delete;

  constexpr auto operator=(owner_base &&other) noexcept -> owner_base & {
    if (&m_data == &other.m_data) { return *this; }
    m_data.state = std::exchange(other.m_data.state);
    m_data.value = std::exchange(other.m_data.value);
    return *this;
  }

  [[nodiscard]]
  constexpr auto get_value_with_status() const noexcept
    -> value_with_status<value_type> {
    return m_data.acquire_copy();
  }

  [[nodiscard]]
  constexpr auto is_alive() const noexcept -> bool {
    return m_data.is_alive();
  }

  [[nodiscard]]
  constexpr auto reference_count() const noexcept -> usize {
    return m_data.state ? m_data.state->counter.ref_count() : usize{};
  }

  [[nodiscard]]
  constexpr auto raw(this auto &&self) noexcept -> auto && {
    return std::forward<std::decay_t<decltype(self)>>(self).m_data.value;
  }

  [[nodiscard]]
  constexpr auto is_linked(ref<value_type> const &ref) const noexcept -> bool;

  constexpr auto operator*() const -> const_reference { return *m_data.value; }

  constexpr auto operator->() const -> const_pointer { return m_data.value; }

  constexpr auto operator*() -> reference { return *m_data.value; }

  constexpr auto operator->() -> pointer { return m_data.value; }

protected:
  value_with_status<value_type> m_data{};

  constexpr explicit owner_base(
    util::allocator_type auto *alloc,
    value_type                *value
  ) noexcept(noexcept(make_ref_counted_storage<std::atomic_bool>(alloc, true)))
    : m_data{ .state = make_ref_counted_storage<std::atomic_bool>(alloc, true),
              .value = value } {}

  constexpr void assign_value(value_type *value) noexcept {
    m_data.value = value;
  }
};

template<class T>
class ref {
  friend class owner_base<T>;

public:
  using value_type      = T;

  using reference       = std::add_lvalue_reference_t<T>;
  using const_reference = std::add_lvalue_reference_t<std::add_const_t<T>>;
  using pointer         = std::add_pointer_t<T>;
  using const_pointer   = std::add_pointer_t<std::add_const_t<T>>;

  constexpr ref(std::nullptr_t null = nullptr) noexcept {}

  constexpr explicit ref(owner_base<value_type> &owner) noexcept
    : m_data{ owner.get_value_with_status() } {}

  constexpr ~ref() { unlink(); }

  constexpr ref(ref const &other)
    : m_data{ other.m_data.get_value_with_status() } {}

  constexpr ref(ref &&other) noexcept
    : m_data{ std::exchange(other.m_data, {}) } {}

  constexpr auto operator=(ref const &other) -> ref & {
    if (this != &other && m_data.value != other.m_data.value) {
      if (m_data.state) { m_data.state->release(); }
      m_data = other.m_data.acquire_copy();
    }
    return *this;
  }

  constexpr auto operator=(ref &&other) noexcept -> ref & {
    if (this != &other) {
      if (m_data.state) { m_data.state->release(); }
      m_data = std::exchange(other.m_data, {});
    }
    return *this;
  }

  constexpr auto operator=(owner_base<value_type> &owner) noexcept -> ref & {
    if (!owner.is_linked(*this)) {
      unlink();
      m_data = owner.get_value_with_status();
    }
    return *this;
  }

  constexpr void unlink() {
    if (m_data.state) { m_data.state->release(); }
    m_data = {};
  }

  [[nodiscard]]
  constexpr auto is_alive() const noexcept -> bool {
    return m_data.is_alive();
  }

  [[nodiscard]]
  constexpr auto reference_count() const noexcept -> usize {
    return m_data.state ? m_data.state->counter.ref_count() : usize{};
  }

  constexpr operator bool() const noexcept { return is_alive(); }

  constexpr auto operator*() const -> const_reference { return *m_data.value; }

  constexpr auto operator->() const -> const_pointer { return m_data.value; }

  constexpr auto operator*() -> reference { return *m_data.value; }

  constexpr auto operator->() -> pointer { return m_data.value; }

private:
  value_with_status<value_type> m_data{};
};

template<class T>
constexpr auto owner_base<T>::is_linked(
  ref<value_type> const &ref
) const noexcept -> bool {
  return &m_data == &ref.m_data;
}

template<class T>
[[nodiscard]] constexpr decltype(auto) make_deletion_function(
  util::allocator_type auto &allocator
) {
  return [&allocator](T *ptr) { util::destroy(allocator, ptr); };
}

template<class T>
class heap_owner final : public owner_base<T> {
public:
  using base_class    = owner_base<T>;
  using value_type    = T;
  using pointer       = std::add_pointer_t<T>;
  using const_pointer = std::add_pointer_t<std::add_const_t<T>>;

  constexpr heap_owner(std::nullptr_t = nullptr) noexcept
    : base_class{ static_cast<base_allocator *>(nullptr), nullptr } {}

  constexpr heap_owner(heap_owner &&other) noexcept
    : base_class{ std::move(other) }
    , m_deleter{ std::move(other.m_deleter) } {}

  constexpr explicit heap_owner(
    util::allocator_type auto &allocator,
    pointer                    ptr
  ) noexcept
    : base_class{ &allocator, ptr }
    , m_deleter{ allocator, make_deletion_function<T>(allocator) } {}

  template<class... Args>
    requires std::constructible_from<value_type, Args &&...>
  explicit heap_owner(util::allocator_type auto &allocator, Args &&...args)
    : base_class{ &allocator,
                  util::construct<value_type>(
                    allocator,
                    std::forward<Args>(args)...
                  ) }
    , m_deleter{ allocator, make_deletion_function<T>(allocator) } {}

  constexpr ~heap_owner() { reset(); }

  constexpr void reset() {
    if (!base_class::is_alive()) { return; }

    base_class::m_data.state->value = false;
    auto value{ std::exchange(base_class::m_data.value, nullptr) };
    m_deleter(value);
    m_deleter = {};
  }

private:
  move_only_func<void(pointer)> m_deleter;
};

template<class T>
class stack_owner final : public owner_base<T> {
public:
  using base_class    = owner_base<T>;
  using value_type    = T;
  using pointer       = std::add_pointer_t<T>;
  using const_pointer = std::add_pointer_t<std::add_const_t<T>>;
  using storage_type  = std::array<std::byte, sizeof(T)>;

  constexpr stack_owner(std::nullptr_t n = nullptr) noexcept
    : base_class{ static_cast<base_allocator *>(nullptr), nullptr } {}

  constexpr stack_owner(stack_owner const &) = delete;

  constexpr stack_owner(stack_owner &&other) noexcept
    : base_class{ std::move(other) }
    , m_value{ std::move(other.m_value) } {}

  constexpr explicit stack_owner(
    util::allocator_type auto &alloc,
    value_type               &&value
  ) noexcept
    : base_class{ &alloc, nullptr }
    , m_value{ std::move(value) } {
    base_class::assign_value(&m_value);
  }

  template<class... Args>
    requires std::constructible_from<T, Args &&...>
  constexpr explicit stack_owner(
    util::allocator_type auto &alloc,
    Args &&...args
  ) noexcept(std::is_nothrow_constructible_v<value_type, Args &&...>)
    : base_class{ &alloc, nullptr } {
    base_class::assign_value(
      std::construct_at(&m_value, std::forward<Args>(args)...)
    );
  }

  constexpr ~stack_owner() { reset(); }

  constexpr auto operator=(stack_owner const &) -> stack_owner & = delete;

  constexpr auto operator=(stack_owner &&other) noexcept -> stack_owner & {
    if (&other != this) {
      m_storage = std::move(other.m_value);
      base_class::operator=(std::move(other));
    }
    return *this;
  }

  constexpr void reset() {
    if (!base_class::is_alive()) { return; }

    base_class::m_data.state->value = false;
    base_class::m_data.value        = nullptr;

    if constexpr (!std::is_trivially_destructible_v<value_type>) {
      m_value.~value_type();
    }
    m_storage.fill(std::byte{});
  }

private:
  union {
    value_type m_value;
    alignas(value_type) storage_type m_storage{};
  };
};

} // namespace gzn::fnd
