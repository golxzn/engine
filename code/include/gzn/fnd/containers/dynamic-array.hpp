#pragma once

#include "gzn/fnd/containers/common.hpp"

namespace gzn::fnd {

/* TODO
 * [ ] Keep allocator copy is the worst ever stupid idea. Keep pointer like in
 *     STD sucks too. what about context?
 */

namespace constants {

u64 static constexpr minimum_grow_factor{ 1 };
u64 static constexpr default_grow_factor{ 2 };

} // namespace constants

template<class T>
struct dynamic_array_twicks {
  u64 static constexpr grow_factor{ constants::default_grow_factor };
};

template<
  class T,
  util::allocator_type Allocator = base_allocator,
  class Twicks                   = dynamic_array_twicks<T>>
class dynamic_array {
  gzn_static_assert(
    Twicks::grow_factor >= constants::minimum_grow_factor,
    "Grow factor should be more or equal to minimum_grow_factor{ 1 }"
  );

public:
  using value_type        = T;
  using pointer           = std::add_pointer_t<T>;
  using const_pointer     = std::add_pointer_t<std::add_const_t<T>>;
  using reference         = std::add_lvalue_reference_t<T>;
  using const_reference   = std::add_lvalue_reference_t<std::add_const_t<T>>;

  using iterator          = pointer;
  using const_iterator    = const_pointer;
  using size_type         = u64;

  using allocator_type    = Allocator;
  using allocator_pointer = std::add_pointer_t<Allocator>;

  explicit dynamic_array(
    allocator_type &allocator,
    size_type const reserve_size = 0
  )
    : m_allocator{ &allocator }
    , m_capacity{ reserve_size }
    , m_data{ containers::mem::allocate<value_type>(allocator, m_capacity) } {}

  explicit dynamic_array(
    allocator_type              &allocator,
    util::range_type auto const &range
  )
    : m_allocator{ &allocator }
    , m_capacity{ static_cast<size_type>(util::size(range)) }
    , m_size{ m_capacity }
    , m_data{ containers::mem::allocate_copy<value_type>(
        allocator,
        std::begin(range),
        std::end(range),
        m_capacity
      ) } {}

  explicit dynamic_array(
    allocator_type         &allocator,
    util::range_type auto &&range
  )
    : m_allocator{ &allocator }
    , m_capacity{ static_cast<size_type>(util::size(range)) }
    , m_size{ m_capacity }
    , m_data{ containers::mem::allocate_move<value_type>(
        allocator,
        std::begin(range),
        std::end(range),
        m_capacity
      ) } {}

  template<size_type Length>
  explicit dynamic_array(
    allocator_type      &allocator,
    c_array<T, Length> &&values
  )
    : m_allocator{ &allocator }
    , m_capacity{ Length }
    , m_size{ m_capacity }
    , m_data{ containers::mem::allocate_move<value_type>(
        allocator,
        values,
        values + m_capacity,
        m_capacity
      ) } {}

  dynamic_array(dynamic_array const &other)
    : m_allocator{ other.m_allocator }
    , m_capacity{ static_cast<size_type>(std::size(other)) }
    , m_size{ m_capacity }
    , m_data{ containers::mem::allocate_copy<value_type>(
        *m_allocator,
        std::begin(other),
        std::end(other),
        m_capacity
      ) } {}

  dynamic_array(dynamic_array &&other) noexcept
    : m_allocator{ other.m_allocator }
    , m_capacity{ std::exchange(other.m_capacity, size_type{}) }
    , m_size{ std::exchange(other.m_size, size_type{}) }
    , m_data{ std::exchange(other.m_data, nullptr) } {}

  ~dynamic_array() { reset(); }

  template<util::allocator_type OtherAlloc, class OtherTweaks>
  auto operator=(dynamic_array<T, OtherAlloc, OtherTweaks> const &other)
    -> dynamic_array & {
    reset();
    m_capacity = other.m_capacity;
    m_size     = other.m_size;
    m_data     = containers::mem::allocate_copy<value_type>(
      *m_allocator, std::begin(other), std::end(other), m_capacity
    );
    return *this;
  }

  auto operator=(dynamic_array &&other) noexcept -> dynamic_array & {
    if (other != &this) {
      reset();
      m_allocator = other.m_allocator;
      m_capacity  = std::exchange(other.m_capacity, size_type{});
      m_size      = std::exchange(other.m_size, size_type{});
      m_data      = std::exchange(other.m_data, nullptr);
    }
    return *this;
  }

  [[nodiscard]]
  constexpr auto operator[](size_t const index) -> reference {
    gzn_assertion(index < m_size, "Index out of range!");
    return m_data[index];
  }

  [[nodiscard]]
  constexpr auto operator[](size_t const index) const -> const_reference {
    gzn_assertion(index < m_size, "Index out of range!");
    return m_data[index];
  }

  [[nodiscard]]
  constexpr auto at(size_t const index) noexcept -> pointer {
    return index < m_size ? m_data + index : nullptr;
  }

  [[nodiscard]]
  constexpr auto at(size_t const index) const noexcept -> const_pointer {
    return index < m_size ? m_data + index : nullptr;
  }

  [[nodiscard]]
  constexpr auto front() noexcept -> pointer {
    return m_data;
  }

  [[nodiscard]]
  constexpr auto front() const noexcept -> const_pointer {
    return m_data;
  }

  [[nodiscard]]
  constexpr auto back() noexcept -> pointer {
    return m_data ? m_data + m_size - 1 : nullptr;
  }

  [[nodiscard]]
  constexpr auto back() const noexcept -> const_pointer {
    return m_data ? m_data + m_size - 1 : nullptr;
  }

  auto push_back(value_type const &value) -> reference {
    if (m_capacity <= m_size) [[unlikely]] { grow(); }
    return m_data[m_size++] = value;
  }

  auto push_back(value_type &&value) -> reference {
    if (m_capacity <= m_size) [[unlikely]] { grow(); }
    return m_data[m_size++] = std::move(value);
  }

  template<class... Args>
    requires std::constructible_from<value_type, Args &&...>
  auto emplace_back(Args &&...args) -> reference {
    if (m_capacity <= m_size) [[unlikely]] { grow(); }
    auto raw{ new (m_data + m_size)
                value_type{ std::forward<Args>(args)... } };
    ++m_size;
    return *raw;
  }

  void pop_back() noexcept(std::is_nothrow_destructible_v<value_type>) {
    gzn_assertion(m_size != 0, "pop_back called with empty array");
    if (m_size != 0) {
      --m_size;
      if constexpr (!std::is_trivially_destructible_v<value_type>) {
        m_data[m_size].~value_type();
      }
    }
  }

  void fast_erase(iterator pos) {
    gzn_assertion(m_size != 0, "erase called with empty array");
    gzn_assertion(begin() <= pos, "'from' is out of range");
    gzn_assertion(end() > pos, "'to' is out of range");

    std::iter_swap(back(), pos);
    pop_back();
  }

  auto erase(iterator pos) -> iterator {
    gzn_assertion(begin() <= pos, "'from' is out of range");
    gzn_assertion(end() > pos, "'to' is out of range");
    if (empty()) [[unlikely]] { return end(); }

    auto const last{ end() };
    auto       cur{ pos };
    for (; cur + 1 != last; ++cur) { std::iter_swap(cur, cur + 1); }
    if constexpr (!std::is_trivially_destructible_v<value_type>) {
      cur->~value_type();
    }
    --m_size;
    return pos;
  }

  void clear() {
    if (m_capacity == 0) { return; }

    if constexpr (!std::is_trivially_destructible_v<value_type>) {
      auto const last{ end() };
      for (auto cur{ begin() }; cur != last; ++cur) { cur->~value_type(); }
    }
    m_size = 0;
    m_data = nullptr;
  }

  void reset() {
    if (m_capacity == 0) { return; }

    if constexpr (!std::is_trivially_destructible_v<value_type>) {
      auto const last{ end() };
      for (auto cur{ begin() }; cur != last; ++cur) { cur->~value_type(); }
    }
    m_allocator->deallocate(m_data, m_capacity, alignof(value_type));
    m_capacity = 0;
    m_size     = 0;
    m_data     = nullptr;
  }

  void reserve(size_type const count) {
    if (m_capacity >= count) [[unlikely]] { return; }

    if (m_data) [[unlikely]] {
      auto new_values{ containers::mem::allocate_move<value_type>(
        *m_allocator, begin(), end(), count
      ) };
      m_allocator->deallocate(m_data, m_capacity, alignof(value_type));
      m_data = new_values;
    } else {
      m_data = containers::mem::allocate<value_type>(*m_allocator, count);
    }
    m_capacity = count;
  }

  void resize(size_t const count, value_type default_value = {}) {
    if (m_size == count) [[unlikely]] { return; }
    if (m_size < count) [[likely]] {
      auto const old_size{ m_size };
      reserve(count);

      auto const last{ m_data + m_capacity - 1 };
      auto       cur{ m_data + old_size };
      for (; cur != last; ++cur) { new (cur) value_type{ default_value }; }
      new (cur) value_type{ std::move(default_value) };
    } else if constexpr (!std::is_trivially_destructible_v<value_type>) {
      auto const last{ end() };
      for (auto cur{ m_data + count }; cur != last; ++cur) {
        cur->~value_type();
      }
    }

    m_size = count;
  }

  auto shrink_to_fit() -> size_type {
    if (m_size == m_capacity || m_capacity == 0) [[unlikely]] { return 0; }

    auto raw{ containers::mem::allocate_move<value_type>(
      *m_allocator, begin(), end(), m_size
    ) };
    m_allocator->deallocate(m_data, m_capacity, alignof(value_type));
    m_data     = raw;
    m_capacity = m_size;
    return m_size;
  }

  [[nodiscard]]
  constexpr auto data() noexcept -> pointer {
    return m_data;
  }

  [[nodiscard]]
  constexpr auto data() const noexcept -> const_pointer {
    return m_data;
  }

  [[nodiscard]]
  constexpr auto size() const noexcept -> size_type {
    return m_size;
  }

  [[nodiscard]]
  constexpr auto capacity() const noexcept -> size_type {
    return m_capacity;
  }

  [[nodiscard]]
  constexpr auto grow_factor() const noexcept -> size_type {
    return Twicks::grow_factor;
  }

  [[nodiscard]]
  constexpr auto size_in_bytes() const noexcept -> size_type {
    return m_size * sizeof(value_type);
  }

  [[nodiscard]]
  constexpr auto capacity_in_bytes() const noexcept -> size_type {
    return m_capacity * sizeof(value_type);
  }

  [[nodiscard]]
  constexpr auto empty() const noexcept -> bool {
    return m_size == 0;
  }

  [[nodiscard]]
  constexpr auto get_allocator() noexcept -> allocator_type & {
    return m_allocator;
  }

  [[nodiscard]]
  constexpr auto get_allocator() const noexcept -> allocator_type const & {
    return m_allocator;
  }

  [[nodiscard]]
  constexpr auto get_grown_capacity(size_type const capacity) const noexcept
    -> size_type {
    return 1 + capacity * grow_factor();
  }

  [[nodiscard]]
  constexpr auto begin() noexcept -> iterator {
    return m_data;
  }

  [[nodiscard]]
  constexpr auto end() noexcept -> iterator {
    return m_data + m_size;
  }

  [[nodiscard]]
  constexpr auto begin() const noexcept -> const_iterator {
    return m_data;
  }

  [[nodiscard]]
  constexpr auto end() const noexcept -> const_iterator {
    return m_data + m_size;
  }

private:
  allocator_pointer m_allocator{ nullptr };
  size_type         m_capacity{};
  size_type         m_size{};
  pointer           m_data{ nullptr };

  void grow() { reserve(get_grown_capacity(m_capacity)); }
};

} // namespace gzn::fnd
