#pragma once

#include "gzn/fnd/allocators/heap-allocator.hpp"
#include "gzn/fnd/containers/common.hpp"
#include "gzn/fnd/pointers.hpp"

namespace gzn::fnd {

using index_type = usize;
static constexpr auto INVALID_INDEX{
  (std::numeric_limits<index_type>::max)()
};

template<
  class T,
  util::allocator_type DataAlloc,
  util::allocator_type MetaAlloc = DataAlloc>
struct index_array_twicks {
  using value_type      = T;
  using size_type       = index_type;
  using data_alloc_type = DataAlloc;
  using meta_alloc_type = MetaAlloc;

  using data_alloc_ptr  = std::add_pointer_t<data_alloc_type>;
  using meta_alloc_ptr  = std::add_pointer_t<meta_alloc_type>;
  using pointer         = std::add_pointer_t<value_type>;
  using const_pointer   = std::add_pointer_t<std::add_const_t<T>>;
  using reference       = std::add_lvalue_reference_t<T>;
  using const_reference = std::add_lvalue_reference_t<std::add_const_t<T>>;

  index_type static constexpr min_capacity{ 0 };
  index_type static constexpr grow_constant{ 3 };

  struct metadata {
    size_type index{};
    size_type generation{};
  };

  /// @todo think about it. Yes, it makes life much easier, but for what price!
  [[nodiscard]]
  static auto get_default_data_allocator() -> data_alloc_ptr {
    thread_local static data_alloc_type alloc;
    return data_alloc_ptr{ &alloc };
  }

  [[nodiscard]]
  static auto get_default_meta_allocator() -> meta_alloc_ptr {
    thread_local static meta_alloc_type alloc;
    return meta_alloc_ptr{ &alloc };
  }
};

template<class T, class Twicks>
class index_array;

template<class T, class Twicks>
class handle {
public:
  using array_type = index_array<T, Twicks>;
  using value_type = typename Twicks::value_type;
  using metadata   = typename Twicks::metadata;
  using size_type  = typename Twicks::size_type;

  constexpr handle(size_type idx, not_null<array_type> array) noexcept;

  constexpr ~handle()                                           = default;

  constexpr handle(handle const &) noexcept                     = default;
  constexpr handle(handle &&) noexcept                          = default;

  constexpr auto operator=(handle const &) noexcept -> handle & = default;
  constexpr auto operator=(handle &&) noexcept -> handle &      = default;

  [[nodiscard]]
  constexpr auto is_valid() const noexcept -> bool;

  [[nodiscard]]
  constexpr auto value(this auto &&self) noexcept -> value_type &;

  [[nodiscard]]
  constexpr auto operator==(handle const &other) const noexcept -> bool;

  [[nodiscard]]
  constexpr auto operator!=(handle const &other) const noexcept -> bool;

  [[nodiscard]]
  constexpr operator bool() const noexcept {
    return is_valid();
  }

  [[nodiscard]]
  constexpr auto index() const noexcept -> size_type {
    return m_index;
  }

  [[nodiscard]]
  constexpr auto generation() const noexcept -> size_type {
    return m_generation;
  }

private:
  not_null<array_type> m_array;
  size_type            m_index;
  size_type            m_generation;

  friend class index_array<T, Twicks>;
};

template<class T, class Twicks = index_array_twicks<T, heap_allocator>>
class index_array {
public:
  using handle_type     = handle<T, Twicks>;
  using value_type      = typename Twicks::value_type;
  using data_alloc_type = typename Twicks::data_alloc_type;
  using meta_alloc_type = typename Twicks::meta_alloc_type;
  using size_type       = typename Twicks::size_type;
  using metadata        = typename Twicks::metadata;

  using data_alloc_ptr  = typename Twicks::data_alloc_ptr;
  using meta_alloc_ptr  = typename Twicks::meta_alloc_ptr;
  using pointer         = typename Twicks::pointer;
  using const_pointer   = typename Twicks::const_pointer;
  using reference       = typename Twicks::reference;
  using const_reference = typename Twicks::const_reference;

private:
  static constexpr bool T_is_trivial{ std::is_trivially_destructible_v<T> };

  using meta_ptr = std::add_pointer_t<metadata>;

  data_alloc_ptr m_data_alloc{ Twicks::get_default_meta_allocator() };
  meta_alloc_ptr m_meta_alloc{ Twicks::get_default_data_allocator() };
  size_type      m_capacity{ Twicks::min_capacity };
  size_type      m_size{};
  pointer        m_values{
    containers::mem::allocate<value_type>(d_alloc(), m_capacity)
  };
  meta_ptr m_metas{ make_sequence_info(m_alloc(), m_capacity) };

public:
  constexpr ~index_array() { clear(); }

  constexpr explicit index_array() = default;

#pragma region reserve constructor

  constexpr explicit index_array(
    data_alloc_ptr  data_alloc,
    meta_alloc_ptr  meta_alloc,
    size_type const reserve_count = 0u
  )
    : m_data_alloc{ data_alloc }
    , m_meta_alloc{ meta_alloc }
    , m_capacity{ (std::max)(reserve_count, Twicks::min_capacity) } {}

  constexpr explicit index_array(
    data_alloc_ptr  alloc         = Twicks::get_default_data_allocator(),
    size_type const reserve_count = 0u
  )
    requires std::is_same_v<data_alloc_type, meta_alloc_type>
    : index_array{ alloc, alloc, reserve_count } {}

#pragma endregion reserve constructor

#pragma region range constructor

  constexpr explicit index_array(
    data_alloc_ptr          data_alloc,
    meta_alloc_ptr          meta_alloc,
    util::range_type auto &&range
  )
    : m_data_alloc{ data_alloc }
    , m_meta_alloc{ meta_alloc }
    , m_capacity{ (std::max)(util::size(range), Twicks::min_capacity) }
    , m_size{ util::size(range) }
    , m_values{ containers::mem::allocate_move<value_type, pointer>(
        d_alloc(),
        std::begin(range),
        std::end(range),
        m_capacity
      ) }
    , m_metas{ make_sequence_info(m_alloc(), m_capacity) } {}

  constexpr explicit index_array(
    data_alloc_ptr          alloc,
    util::range_type auto &&range
  )
    requires std::is_same_v<data_alloc_type, meta_alloc_type>
    : index_array{ alloc, alloc, std::move(range) } {}

#pragma endregion range constructor

#pragma region cstyle array constructor

  template<size_type Length>
  constexpr explicit index_array(
    data_alloc_ptr    data_alloc,
    meta_alloc_ptr    meta_alloc,
    carr<T, Length> &&values
  )
    : m_data_alloc{ data_alloc }
    , m_meta_alloc{ meta_alloc }
    , m_capacity{ (std::max)(Length, Twicks::min_capacity) }
    , m_size{ Length }
    , m_values{ containers::mem::allocate_move<value_type>(
        d_alloc(),
        values,
        values + Length,
        m_capacity
      ) }
    , m_metas{ make_sequence_info(m_alloc(), m_capacity) } {}

  template<size_type Length>
  constexpr explicit index_array(
    data_alloc_ptr    alloc,
    carr<T, Length> &&values
  )
    requires std::is_same_v<data_alloc_type, meta_alloc_type>
    : index_array{ alloc, alloc, values } {}

#pragma endregion cstyle array constructor

#pragma region copy and move

  constexpr index_array(index_array const &other)
    : m_data_alloc{ other.m_data_alloc }
    , m_meta_alloc{ other.m_meta_alloc }
    , m_capacity{ other.m_capacity }
    , m_size{ other.m_size }
    , m_values{ containers::mem::allocate_copy<value_type>(
        d_alloc(),
        other.m_values,
        other.m_values + m_size,
        m_capacity
      ) }
    , m_metas{ containers::mem::allocate_copy<metadata>(
        m_alloc(),
        other.m_metas,
        other.m_metas + m_size,
        m_capacity
      ) } {}

  constexpr index_array(index_array &&other) noexcept
    : m_data_alloc{
      std::exchange(other.m_data_alloc, Twicks::get_default_data_allocator())
    }
    , m_meta_alloc{ std::exchange(
        other.m_meta_alloc,
        Twicks::get_default_meta_allocator()
      ) }
    , m_capacity{ std::exchange(other.m_capacity, Twicks::min_capacity) }
    , m_size{ std::exchange(other.m_size, 0u) }
    , m_values{ std::exchange(
        other.m_values,
        containers::mem::allocate<value_type>(
          other.d_alloc(),
          other.m_capacity
        )
      ) }
    , m_metas{ std::exchange(
        other.m_metas,
        make_sequence_info(other.m_alloc(), other.m_capacity)
      ) } {}

  constexpr auto operator=(index_array const &other) -> index_array & {
    if (this == &other) [[unlikely]] { return *this; }

    if (std::empty(*this)) {
      new (this) index_array{ other };
      return *this;
    }

    using namespace containers;

    auto const old_cap{ std::exchange(m_capacity, other.m_capacity) };
    reset();

    if (m_data_alloc != other.m_data_alloc) {
      mem::deallocate<value_type>(d_alloc(), m_values, old_cap);
      m_data_alloc = other.m_data_alloc;
      m_values     = mem::allocate<value_type>(d_alloc(), m_capacity);
    } else if (old_cap < m_capacity) {
      reserve_values(old_cap, m_capacity);
    }

    if (m_meta_alloc != other.m_meta_alloc) {
      mem::deallocate<metadata>(m_alloc(), m_metas, old_cap);
      m_meta_alloc = other.m_meta_alloc;
      m_metas      = mem::allocate<metadata>(m_alloc(), m_capacity);
    } else if (old_cap < m_capacity) {
      reserve_metas(old_cap, m_capacity);
    }

    m_size = other.m_size;
    std::copy_n(other.m_values, m_size, m_values);
    std::copy_n(other.m_metas, m_size, m_metas);

    return *this;
  }

  constexpr auto operator=(index_array &&other) -> index_array & {
    if (this == &other) [[unlikely]] { return *this; }

    clear();

    m_data_alloc = other.m_data_alloc;
    m_meta_alloc = other.m_meta_alloc;
    m_capacity   = std::exchange(other.m_capacity, 0u);
    m_size       = std::exchange(other.m_size, 0u);
    m_values     = std::exchange(other.m_values, nullptr);
    m_metas      = std::exchange(other.m_metas, nullptr);

    return *this;
  }

#pragma endregion copy and move

  [[nodiscard]]
  constexpr auto operator[](this auto &&self, size_type const idx)
    -> reference {
    gzn_assertion(idx >= self.m_capacity, "Index out of range");
    return self.m_values[self.m_metas[idx].index];
  }

  [[nodiscard]]
  constexpr auto at(this auto &&self, size_type const idx) -> pointer {
    gzn_assertion(idx >= self.m_capacity, "Index out of range");
    return idx < self.m_capacity ? self.m_values[self.m_metas[idx].index]
                                 : nullptr;
  }

  [[nodiscard]] constexpr auto front(this auto &&self) noexcept -> pointer {
    return self.m_values;
  }

  [[nodiscard]] constexpr auto back(this auto &&self) noexcept -> pointer {
    return self.m_values + self.m_size;
  }

  [[nodiscard]]
  constexpr auto push_back(value_type &&value) -> size_type {
    auto const [offset, index]{ acquire_slot() };
    new (m_values + offset) value_type{ std::move(value) };
    return index;
  }

  [[nodiscard]]
  constexpr auto push_back(value_type const &value) -> size_type {
    auto const [offset, index]{ acquire_slot() };
    new (m_values + offset) value_type{ std::move(value) };
    return index;
  }

  template<class... Args>
    requires std::constructible_from<value_type, Args...>
  [[nodiscard]] constexpr auto emplace_back(Args &&...args) -> size_type {
    auto const [offset, index]{ acquire_slot() };
    new (m_values + offset) value_type{ std::forward<Args>(args)... };
    return index;
  }

  constexpr void pop_back() noexcept(
    std::is_nothrow_destructible_v<value_type>
  ) {
    if (m_size == 0) [[unlikely]] {
      gzn_do_assertion("pop_back called with empty array");
      return;
    }
    --m_size;
    ++m_metas[m_size].generation;
    if constexpr (!T_is_trivial) { m_values[m_size].~value_type(); }
  }

  constexpr auto erase(
    size_type const idx
  ) noexcept(std::is_nothrow_destructible_v<value_type>) -> size_type {
    if (idx >= m_size) [[unlikely]] {
      gzn_do_assertion("erase: invalid index");
      return INVALID_INDEX;
    }

    --m_size;
    ++m_metas[idx].generation;

    auto const last_idx{ m_metas[m_size].index };
    std::swap(m_values[m_size], m_values[last_idx]);
    std::swap(m_metas[m_size], m_metas[last_idx]);
    return last_idx;
  }

  constexpr void reserve(size_type const new_cap) {
    if (m_capacity >= new_cap) [[unlikely]] { return; }

    auto const old_cap{ std::exchange(m_capacity, new_cap) };
    if (m_values) [[likely]] {
      reserve_values(old_cap, new_cap);
      reserve_metas(old_cap, new_cap);
    } else {
      m_values = containers::mem::allocate<value_type>(d_alloc(), new_cap);
      m_metas  = containers::mem::allocate<metadata>(m_alloc(), new_cap);
    }

    for (size_type idx{ old_cap }; idx < m_capacity; ++idx) {
      m_metas[idx] = {
        .index      = idx,
        .generation = 0u,
      };
    }
  }

  constexpr void reset() {
    if constexpr (T_is_trivial) {
      for (size_t idx{}; idx < m_size; ++idx) {
        m_values[m_size].~value_type();
      }
    }
    for (size_type idx; idx < m_capacity; ++idx) { ++m_metas[idx].generation; }
    m_size = 0u;
  }

  constexpr void clear() {
    if constexpr (T_is_trivial) {
      for (size_t idx{}; idx < m_size; ++idx) {
        m_values[m_size].~value_type();
      }
    }
    containers::mem::deallocate<value_type>(d_alloc(), m_values, m_capacity);
    containers::mem::deallocate<metadata>(m_alloc(), m_metas, m_capacity);

    m_size     = 0u;
    m_capacity = 0u;
    m_values   = nullptr;
    m_metas    = nullptr;
  }

  constexpr void shrink_to_fit() {
    if (m_size == m_capacity || m_capacity == 0u) [[unlikely]] { return; }

    auto values{ containers::mem::allocate_move<value_type>(
      d_alloc(), begin(), end(), m_size
    ) };
    auto metas{ containers::mem::allocate_move<metadata>(
      m_alloc(), m_metas, m_metas + m_size, m_size
    ) };
    containers::mem::deallocate<value_type>(d_alloc(), m_values, m_capacity);
    containers::mem::deallocate<metadata>(m_alloc(), m_metas, m_capacity);
    m_capacity = m_size;
    m_values   = values;
    m_metas    = metas;
  }

  [[nodiscard]]
  constexpr auto make_handle(size_type const idx) noexcept -> handle_type {
    return handle_type{ idx, not_null{ this } };
  }

  [[nodiscard]]
  constexpr auto generation(size_type const idx) const noexcept -> size_type {
    return idx < m_capacity ? m_metas[idx].generation : INVALID_INDEX;
  }

  [[nodiscard]]
  constexpr auto data(this auto &&self) noexcept -> pointer {
    return self.m_values;
  }

  [[nodiscard]]
  constexpr auto size() const noexcept -> size_type {
    return m_size;
  }

  [[nodiscard]]
  constexpr auto bytes_count() const noexcept -> size_type {
    return m_size * sizeof(value_type);
  }

  [[nodiscard]]
  constexpr auto capacity() const noexcept -> size_type {
    return m_capacity;
  }

  [[nodiscard]]
  constexpr auto capacity_bytes_count() const noexcept -> size_type {
    return m_capacity * sizeof(value_type);
  }

  [[nodiscard]]
  constexpr auto empty() const noexcept -> bool {
    return m_size == 0;
  }

  [[nodiscard]]
  constexpr auto is_valid_idx(size_type const idx) const noexcept -> bool {
    return idx < m_capacity;
  }

  [[nodiscard]]
  constexpr auto is_valid(
    size_type const idx,
    size_type const gen
  ) const noexcept -> bool {
    return is_valid_idx(idx) && m_metas[idx].generation == gen;
  }

  [[nodiscard]]
  constexpr auto begin(this auto &&self) noexcept -> pointer {
    return self.m_values;
  }

  [[nodiscard]]
  constexpr auto end(this auto &&self) noexcept -> pointer {
    return self.m_values + self.m_size;
  }

  [[nodiscard]]
  constexpr auto get_data_allocator(this auto &&self) {
    return self.m_data_alloc;
  }

  [[nodiscard]]
  constexpr auto get_meta_allocator(this auto &&self) {
    return self.m_meta_alloc;
  }

  gzn_inline constexpr void grow() { reserve(get_grown_capacity(m_capacity)); }

  [[nodiscard]]
  static constexpr auto grow_factor() noexcept -> size_type {
    return Twicks::grow_constant;
  }

  [[nodiscard]]
  static constexpr auto get_grown_capacity(size_type const capacity) noexcept
    -> size_type {
    return 1u + capacity * grow_factor();
  }

private:
  constexpr auto acquire_slot() -> std::pair<size_type, size_type> {
    if (m_size >= m_capacity) [[unlikely]] {
      grow();
      m_metas[m_size] = {
        .index      = m_size,
        .generation = 0u,
      };
    }

    auto const idx{ m_size++ };
    auto      &meta{ m_metas[idx] };
    ++meta.generation;
    return std::make_pair(meta.index, idx);
  }

  constexpr void reserve_values(
    size_type const old_cap,
    size_type const new_cap
  ) {
    using namespace containers;
    if (mem::grow<value_type>(d_alloc(), m_values, m_size, old_cap, new_cap))
      [[likely]] {
      return;
    }

    auto place{ mem::allocate<value_type>(d_alloc(), new_cap) };
    mem::deallocate<value_type>(d_alloc(), m_values, old_cap);
    m_values = place;
  }

  constexpr void reserve_metas(
    size_type const old_cap,
    size_type const new_cap
  ) {
    using namespace containers;
    if (mem::grow<metadata>(m_alloc(), m_metas, m_size, old_cap, new_cap))
      [[likely]] {
      return;
    }

    auto place{ mem::allocate<metadata>(m_alloc(), new_cap) };
    mem::deallocate<metadata>(m_alloc(), m_metas, old_cap);
    m_metas = place;
  }

  gzn_inline constexpr auto d_alloc() noexcept -> data_alloc_type & {
    return *m_data_alloc;
  }

  gzn_inline constexpr auto m_alloc() noexcept -> meta_alloc_type & {
    return *m_meta_alloc;
  }

  gzn_inline constexpr static auto make_sequence_info(
    meta_alloc_type &alloc,
    size_type const  count
  ) -> meta_ptr {
    auto data{ containers::mem::allocate<metadata>(alloc, count) };
    for (size_type idx{}; idx != count; ++idx) { data[idx].index = idx; }
    return data;
  }
};

template<class T, class Tw>
constexpr handle<T, Tw>::handle(
  size_type const      idx,
  not_null<array_type> array
) noexcept
  : m_array{ array }
  , m_index{ idx }
  , m_generation{ array->generation(idx) } {}

template<class T, class Tw>
constexpr auto handle<T, Tw>::is_valid() const noexcept -> bool {
  return m_array->is_valid(m_index, m_generation);
}

template<class T, class Tw>
constexpr auto handle<T, Tw>::value(this auto &&self) noexcept
  -> value_type & {
  return self.m_array->operator[](self.m_index);
}

template<class T, class Tw>
constexpr auto handle<T, Tw>::operator==(handle const &other) const noexcept
  -> bool {
  return m_index == other.m_index && m_array == other.m_array;
}

template<class T, class Tw>
constexpr auto handle<T, Tw>::operator!=(handle const &other) const noexcept
  -> bool {
  return !(*this == other);
}


} // namespace gzn::fnd
